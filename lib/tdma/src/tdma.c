/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file dw1000_tdma.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief TDMA
 *
 * @details This is the base class of tdma which initialises tdma instance, assigns slots for each node and does ranging continuously based on
 * addresses.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "sysinit/sysinit.h"
#include <dpl/dpl.h>
#include <dpl/queue.h>
#include <dpl/dpl_cputime.h>

#include <uwb/uwb.h>
#include <tdma/tdma.h>

#if __KERNEL__
#include <linux/math64.h>
#endif

#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif
#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif

#if MYNEWT_VAL(TDMA_SANITY_INTERVAL) > 0
#include <os/os_sanity.h>
#endif

#include <stats/stats.h>

#if MYNEWT_VAL(TDMA_STATS)
STATS_NAME_START(tdma_stat_section)
    STATS_NAME(tdma_stat_section, slot_timer_cnt)
    STATS_NAME(tdma_stat_section, superframe_cnt)
    STATS_NAME(tdma_stat_section, superframe_miss)
    STATS_NAME(tdma_stat_section, dropped_slots)
STATS_NAME_END(tdma_stat_section)

#define TDMA_STATS_INC(__X) STATS_INC(tdma->stat, __X)
#else
#define TDMA_STATS_INC(__X) {}
#endif

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

static void tdma_superframe_slot_cb(struct dpl_event * ev);
static void slot_timer_cb(void * arg);
static bool superframe_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

#ifdef TDMA_TASKS_ENABLE
static void tdma_tasks_init(struct _tdma_instance_t * inst);
static void * tdma_task(void *arg);
#endif

/**
 * @fn tdma_init(struct uwb_dev * dev, uint16_t nslots)
 * @brief API to initialise the tdma instance. Sets the clkcal postprocess and
 * assings the slot callback function for slot0.
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param nslots   Total slots to be allocated between two frames.
 *
 * @return tdma_instance_t*
 */
tdma_instance_t *
tdma_init(struct uwb_dev *dev, uint16_t nslots)
{
    assert(dev);
    tdma_instance_t * tdma = (tdma_instance_t*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_TDMA);

    if (tdma == NULL) {
        tdma = (tdma_instance_t *) calloc(1, sizeof(struct _tdma_instance_t) + nslots * sizeof(struct _tdma_slot_t *));
        assert(tdma);
        tdma->status.selfmalloc = 1;
        dpl_error_t err = dpl_mutex_init(&tdma->mutex);
        assert(err == DPL_OK);
        tdma->nslots = nslots;
        tdma->dev_inst = dev;
#ifdef TDMA_TASKS_ENABLE
        tdma->task_prio = dev->task_prio + 0x6;
#endif
    }

    tdma->cbs = (struct uwb_mac_interface){
        .id = UWBEXT_TDMA,
        .inst_ptr = (void*)tdma,
        .superframe_cb = superframe_cb
    };
    uwb_mac_append_interface(dev, &tdma->cbs);

    tdma->ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_CCP);
    assert(tdma->ccp);

#if MYNEWT_VAL(TDMA_STATS)
    int rc = stats_init(
                STATS_HDR(tdma->stat),
                STATS_SIZE_INIT_PARMS(tdma->stat, STATS_SIZE_32),
                STATS_NAME_INIT_PARMS(tdma_stat_section)
            );
    assert(rc == 0);

#if  MYNEWT_VAL(UWB_DEVICE_0) && !MYNEWT_VAL(UWB_DEVICE_1)
    rc = stats_register("tdma", STATS_HDR(tdma->stat));
#elif  MYNEWT_VAL(UWB_DEVICE_0) && MYNEWT_VAL(UWB_DEVICE_1)
    if (dev->idx == 0)
        rc |= stats_register("tdma0", STATS_HDR(tdma->stat));
    else
        rc |= stats_register("tdma1", STATS_HDR(tdma->stat));
#endif
    assert(rc == 0);
#endif

    tdma->superframe_slot.parent = tdma;
    tdma->superframe_slot.idx = 0;
    dpl_cputime_timer_init(&tdma->superframe_slot.timer, slot_timer_cb, (void *) &tdma->superframe_slot);
    dpl_event_init(&tdma->superframe_slot.event, tdma_superframe_slot_cb, (void *) &tdma->superframe_slot);
    tdma->status.initialized = true;

    tdma->os_epoch = dpl_cputime_get32();

#ifdef TDMA_TASKS_ENABLE
    tdma_tasks_init(tdma);
#endif
    return tdma;
}

/**
 * @fn tdma_free(tdma_instance_t * inst)
 * @brief API to free memory allocated for tdma slots.
 *
 * @param inst  Pointer to tdma_instance_t.
 *
 * @return void
 */
void
tdma_free(tdma_instance_t * inst)
{
    assert(inst);
    tdma_stop(inst);
    uwb_mac_remove_interface(inst->dev_inst, inst->cbs.id);
    if (inst->status.selfmalloc)
        free(inst);
    else
        inst->status.initialized = 0;
}


#ifdef TDMA_TASKS_ENABLE

/**
 * @fn sanity_feeding_cb(struct dpl_event * ev)
 * @brief API to feed the sanity watchdog
 *
 * @return void
 */
#if MYNEWT_VAL(TDMA_SANITY_INTERVAL) > 0
static void
sanity_feeding_cb(struct dpl_event * ev)
{
    assert(ev != NULL);
    tdma_instance_t * tdma = (tdma_instance_t * )dpl_event_get_arg(ev);
    assert(tdma);

    os_sanity_task_checkin(0);
    dpl_callout_reset(&tdma->sanity_cb, DPL_TICKS_PER_SEC);
}
#endif

/**
 * @fn tdma_tasks_init(struct _tdma_instance_t * inst)
 * @brief API to initialise a higher priority task for the tdma slot tasks.
 *
 * @param inst  Pointer to  _tdma_instance_t.
 *
 * @return void
 */
static void
tdma_tasks_init(struct _tdma_instance_t * inst)
{
    /* Check if the tasks are already initiated */
    if (!dpl_eventq_inited(&inst->eventq))
    {
        /* Use a dedicate event queue for tdma events */
        dpl_eventq_init(&inst->eventq);
        dpl_task_init(&inst->task_str, "tdma",
                      tdma_task,
                      (void *) inst,
                      inst->task_prio,
#if MYNEWT_VAL(TDMA_SANITY_INTERVAL) > 0
                      DPL_TICKS_PER_SEC * MYNEWT_VAL(TDMA_SANITY_INTERVAL),
#else
                      DPL_WAIT_FOREVER,
#endif
                      inst->task_stack,
                      MYNEWT_VAL(TDMA_TASK_STACK_SZ));
    }
#if MYNEWT_VAL(TDMA_SANITY_INTERVAL) > 0
    dpl_callout_init(&inst->sanity_cb, &inst->eventq, sanity_feeding_cb, (void *) inst);
    dpl_callout_reset(&inst->sanity_cb, DPL_TICKS_PER_SEC);
#endif
}

/**
 * @fn tdma_task(void *arg)
 * @brief API for task function of tdma.
 *
 * @param arg   Pointer to an argument of void type.
 *
 * @return void
 */
static void *
tdma_task(void *arg)
{
#if MYNEWT_VAL(TDMA_MAX_SLOT_DELAY_US) > 0
    uint32_t ticks, delay;
    tdma_slot_t * slot;
#endif
    struct dpl_event *ev;
    tdma_instance_t * tdma = arg;

    while (1) {
        ev = dpl_eventq_get(&tdma->eventq);
#if MYNEWT_VAL(TDMA_MAX_SLOT_DELAY_US) == 0
        dpl_event_run(ev);

#else
#if MYNEWT_VAL(TDMA_SANITY_INTERVAL) > 0
        if (ev == dpl_callout_get_event(&tdma->sanity_cb)) {
            dpl_event_run(ev);
            continue;
        }
#endif
        /* Assume all other events are tdma_slots */
        slot = (tdma_slot_t *) dpl_event_get_arg(ev);
        ticks = dpl_cputime_get32();
        delay = dpl_cputime_ticks_to_usecs(ticks - slot->cputime_slot_start);

        /* Ignore slots if we're too late in to run them */
        if (slot->idx!=0 && delay > MYNEWT_VAL(TDMA_MAX_SLOT_DELAY_US)) {
            TDMA_STATS_INC(dropped_slots);
        } else{
            dpl_event_run(ev);
        }
#endif
    }
    return NULL;
}
#endif

/**
 * @fn superframe_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return bool based on the totality of the handling which is false this implementation.
 */
static bool
superframe_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    tdma_instance_t * tdma = (tdma_instance_t*)cbs->inst_ptr;
    struct uwb_ccp_instance *ccp = tdma->ccp;
    if (!tdma) {
        return false;
    }

    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"tdma:rx_complete_cb\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    if (tdma->status.initialized) {
        tdma->os_epoch = ccp->os_epoch;
#ifdef TDMA_TASKS_ENABLE
        dpl_eventq_put(&tdma->eventq, &tdma->superframe_slot.event);
#else
        dpl_eventq_put(&inst->eventq, &tdma->superframe_slot.event);
#endif
    }
    return false;
}

/**
 * @fn tdma_assign_slot(struct _tdma_instance_t * inst, void (* call_back )(struct dpl_event *), uint16_t idx, void * arg)
 * @brief API to intialise slot instance for the slot.Also initialise a timer and assigns callback for each slot.
 *
 * @param inst       Pointer to _tdma_instance_t.
 * @param call_back  Callback for the particular slot.
 * @param idx        Slot number.
 * @param arg        Argument to the callback.
 *
 * @return void
 */
void
tdma_assign_slot(struct _tdma_instance_t * inst, void (* call_back )(struct dpl_event *), uint16_t idx, void * arg)
{
    assert(idx < inst->nslots);

    if (inst->status.initialized == false)
       return;

    if (inst->slot[idx] == NULL){
        inst->slot[idx] = (tdma_slot_t  *) calloc(1, sizeof(struct _tdma_slot_t));
        assert(inst->slot[idx]);
    }else{
        memset(inst->slot[idx], 0, sizeof(struct _tdma_slot_t));
    }

    inst->slot[idx]->idx = idx;
    inst->slot[idx]->parent = inst;
    inst->slot[idx]->arg = arg;

    dpl_event_init(&inst->slot[idx]->event, call_back, (void *) inst->slot[idx]);
    dpl_cputime_timer_init(&inst->slot[idx]->timer, slot_timer_cb, (void *) inst->slot[idx]);
}
EXPORT_SYMBOL(tdma_assign_slot);


/**
 * @fn tdma_release_slot(struct _tdma_instance_t * inst, uint16_t idx)
 * @brief API to free the slot instance.
 *
 * @param inst   Pointer to _tdma_instance_t.
 * @param idx    Slot number.
 *
 * @return void
 */
void
tdma_release_slot(struct _tdma_instance_t * inst, uint16_t idx)
{
    assert(idx < inst->nslots);
    if (inst->slot[idx]) {
        dpl_cputime_timer_stop(&inst->slot[idx]->timer);
        free(inst->slot[idx]);
        inst->slot[idx] =  NULL;
    }
}
EXPORT_SYMBOL(tdma_release_slot);

/**
 * @fn tdma_superframe_slot_cb(struct dpl_event * ev)
 * @brief This event is generated by ccp/clkcal complete event. This event defines the start of an superframe epoch.
 * The event also schedules a tdma_superframe_timer_cb which turns on the receiver in advance of the next superframe epoch.
 *
 * @param ev   Pointer to dpl_event.
 *
 * @return void
 */
static void
tdma_superframe_slot_cb(struct dpl_event * ev)
{
    uint16_t i;
    uint32_t slot_period_us;
    struct _tdma_slot_t *slot;
    tdma_instance_t * tdma;
    struct uwb_ccp_instance * ccp;
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev) != NULL);

    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"tdma_superframe_slot_cb\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    slot = (struct _tdma_slot_t *) dpl_event_get_arg(ev);
    tdma = slot->parent;
    ccp = tdma->ccp;

    TDMA_STATS_INC(superframe_cnt);

    /* Make sure all timers are stopped */
    dpl_cputime_timer_stop(&slot->timer);
    for (i = 0; i < tdma->nslots; i++) {
        if (tdma->slot[i]){
            dpl_cputime_timer_stop(&tdma->slot[i]->timer);
        }
    }
#if __KERNEL__
    slot_period_us = uwb_dwt_usecs_to_usecs(div64_s64(ccp->period, tdma->nslots));
#else
    slot_period_us = uwb_dwt_usecs_to_usecs(ccp->period / tdma->nslots);
#endif
    for (i = 0; i < tdma->nslots; i++) {
        if (tdma->slot[i]){
            tdma->slot[i]->cputime_slot_start = tdma->os_epoch
                + dpl_cputime_usecs_to_ticks((uint32_t) (i * slot_period_us) - MYNEWT_VAL(OS_LATENCY));
            hal_timer_start_at(&tdma->slot[i]->timer, tdma->slot[i]->cputime_slot_start);
        }
    }

    /* Next superframe slot estimate */
    slot->cputime_slot_start = tdma->os_epoch
        + dpl_cputime_usecs_to_ticks(
            (uint32_t)uwb_dwt_usecs_to_usecs(ccp->period) + slot_period_us);
    hal_timer_start_at(&slot->timer, slot->cputime_slot_start);
}

/**
 * @fn slot_timer_cb(void * arg)
 * @brief API for each slot. This function then puts
 * the callback provided by the user in the tdma event queue.
 *
 * @param arg    A void type argument.
 *
 * @return void
 */
static void
slot_timer_cb(void * arg)
{
    assert(arg);

    tdma_slot_t * slot = (tdma_slot_t *) arg;
    /* No point in continuing if this slot is NULL */
    if (slot == NULL) {
        return;
    }
    tdma_instance_t * tdma = slot->parent;

    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"slot_timer_cb\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));

    TDMA_STATS_INC(slot_timer_cnt);

    if (slot == &tdma->superframe_slot) {
        /* Superframe must have been missed by ccp */
        TDMA_STATS_INC(superframe_miss);
        return;
    }

#ifdef TDMA_TASKS_ENABLE
    dpl_eventq_put(&tdma->eventq, &slot->event);
#else
    dpl_eventq_put(&tdma->dev_inst->eventq, &slot->event);
#endif
}

/**
 * @fn tdma_stop(struct _tdma_instance_t * tdma)
 * @brief API to stop tdma operation. Releases each slot and stops all cputimer callbacks
 *
 * @param tdma      Pointer to _tdma_instance_t.
 *
 * @return void
 */
void
tdma_stop(struct _tdma_instance_t * tdma)
{
    uint16_t i;
    for (i = 0; i < tdma->nslots; i++) {
        if (tdma->slot[i]){
            dpl_cputime_timer_stop(&tdma->slot[i]->timer);
            tdma_release_slot(tdma, i);
        }
    }
}


/**
 * Function for calculating the start of the slot for a rx operation
 *
 * @param inst       Pointer to struct struct _tdma_instance_t
 * @param idx        Slot index
 *
 * @return dx_time   The time for a rx operation to start
 */
uint64_t
tdma_rx_slot_start(struct _tdma_instance_t * tdma, dpl_float32_t idx)
{
    uint64_t dx_time, slot_offset;
    dpl_float64_t slot_period;
    struct uwb_ccp_instance * ccp = tdma->ccp;
    uint64_t rx_stable = tdma->dev_inst->config.rx.timeToRxStable;
#if __KERNEL__
    slot_period = DPL_FLOAT64_U64_TO_F64(div64_s64(((uint64_t)ccp->period << 16), tdma->nslots));
#else
    slot_period = DPL_FLOAT64_U64_TO_F64((((uint64_t)ccp->period << 16) / tdma->nslots));
#endif
    slot_offset = DPL_FLOAT64_INT(DPL_FLOAT64_MUL(DPL_FLOAT64_FROM_F32(idx), slot_period));
    /* Compensate for the time it takes to turn on the receiver */
    slot_offset -= (rx_stable << 16);

#if MYNEWT_VAL(UWB_WCS_ENABLED)
    {
        struct uwb_wcs_instance * wcs = ccp->wcs;
        dx_time = ccp->local_epoch + (uint64_t) uwb_wcs_dtu_time_adjust(wcs, slot_offset);
    }
#else
    dx_time = ccp->local_epoch + slot_offset;
#endif
    return dx_time;
}
EXPORT_SYMBOL(tdma_rx_slot_start);

/**
 * Function for calculating the start of the slot for a tx operation.
 * taking into account that the preamble needs to be sent before the
 * RMARKER, which marks the time of the frame, is sent.
 *
 * @param inst       Pointer to struct _tdma_instance_t
 * @param idx        Slot index
 *
 * @return dx_time   The time for a rx operation to start (dtu)
 */
uint64_t
tdma_tx_slot_start(struct _tdma_instance_t * tdma, dpl_float32_t idx)
{
    uint64_t dx_time = tdma_rx_slot_start(tdma, idx);
#ifndef __KERNEL__
    dx_time = (dx_time + ((uint64_t)ceilf(uwb_usecs_to_dwt_usecs(uwb_phy_SHR_duration(tdma->dev_inst))) << 16));
#else
    dx_time = (dx_time + ((uint64_t)(uwb_usecs_to_dwt_usecs(uwb_phy_SHR_duration(tdma->dev_inst))) << 16));
#endif
    return dx_time;
}
EXPORT_SYMBOL(tdma_tx_slot_start);

/**
 * @fn tdma_pkg_init(void)
 * @ brief API to initialise the package, only one ccp service required in the system.
 *
 * @return void
 */
void
tdma_pkg_init(void)
{
    int i;
    struct uwb_dev *udev;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"tdma_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        tdma_init(udev, MYNEWT_VAL(TDMA_NSLOTS));
    }
}

/**
 * @fn tdma_pkg_down(int reason)
 * @ brief API to uninitialise the package
 *
 * @return void
 */
int
tdma_pkg_down(int reason)
{
    int i;
    struct uwb_dev *udev;
    struct _tdma_instance_t * tdma;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"tdma_pkg_down\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        tdma = (struct _tdma_instance_t*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_TDMA);
        if (!tdma) {
            continue;
        }
        tdma_free(tdma);
    }
    return 0;
}
