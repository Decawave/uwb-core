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
 * @file ccp.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief clock calibration packets
 *
 * @details This is the ccp base class which utilises the functions to enable/disable the configurations related to ccp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <dpl/dpl_os.h>
#include <dpl/dpl_cputime.h>

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <uwb_ccp/uwb_ccp.h>
#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif
#if MYNEWT_VAL(UWB_CCP_VERBOSE)
#include <uwb_ccp/ccp_json.h>
#endif

//#define DIAGMSG(s,u) printf(s,u)

#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

#if MYNEWT_VAL(UWB_CCP_STATS)
#include <stats/stats.h>
STATS_NAME_START(uwb_ccp_stat_section)
    STATS_NAME(uwb_ccp_stat_section, master_cnt)
    STATS_NAME(uwb_ccp_stat_section, slave_cnt)
    STATS_NAME(uwb_ccp_stat_section, send)
    STATS_NAME(uwb_ccp_stat_section, listen)
    STATS_NAME(uwb_ccp_stat_section, tx_complete)
    STATS_NAME(uwb_ccp_stat_section, rx_complete)
    STATS_NAME(uwb_ccp_stat_section, rx_relayed)
    STATS_NAME(uwb_ccp_stat_section, rx_start_error)
    STATS_NAME(uwb_ccp_stat_section, rx_unsolicited)
    STATS_NAME(uwb_ccp_stat_section, rx_other_frame)
    STATS_NAME(uwb_ccp_stat_section, txrx_error)
#if MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES) > 0
    STATS_NAME(uwb_ccp_stat_section, err_tolerated)
#endif
    STATS_NAME(uwb_ccp_stat_section, tx_start_error)
    STATS_NAME(uwb_ccp_stat_section, tx_relay_error)
    STATS_NAME(uwb_ccp_stat_section, tx_relay_ok)
    STATS_NAME(uwb_ccp_stat_section, irq_latency)
    STATS_NAME(uwb_ccp_stat_section, os_lat_behind)
    STATS_NAME(uwb_ccp_stat_section, os_lat_margin)
    STATS_NAME(uwb_ccp_stat_section, rx_timeout)
    STATS_NAME(uwb_ccp_stat_section, sem_timeout)
    STATS_NAME(uwb_ccp_stat_section, reset)
STATS_NAME_END(uwb_ccp_stat_section)

#define CCP_STATS_INC(__X) STATS_INC(ccp->stat, __X)
#define CCP_STATS_SET(__X, __N) {STATS_CLEAR(ccp->stat, __X);STATS_INCN(ccp->stat, __X, __N);}
#else
#define CCP_STATS_INC(__X) {}
#define CCP_STATS_SET(__X, __N) {}
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

static struct uwb_ccp_status ccp_send(struct uwb_ccp_instance * ccp, uwb_dev_modes_t mode);
static struct uwb_ccp_status ccp_listen(struct uwb_ccp_instance * ccp, uint64_t dx_time, uwb_dev_modes_t mode);

static void ccp_tasks_init(struct uwb_ccp_instance * inst);
static void ccp_timer_irq(void * arg);
static void ccp_master_timer_ev_cb(struct dpl_event *ev);
static void ccp_slave_timer_ev_cb(struct dpl_event *ev);
void ccp_encode(uint64_t epoch, uint64_t transmission_timestamp, uint64_t delta, uint8_t seq_num,  dpl_float64_t carrier_integrator);

#ifdef __KERNEL__
void ccp_chrdev_destroy(int idx);
int ccp_chrdev_create(int idx);

void ccp_sysfs_init(struct uwb_ccp_instance *ccp);
void ccp_sysfs_deinit(int idx);
int ccp_chrdev_output(int idx, char *buf, size_t len);
#endif  /* __KERNEL__ */

#if MYNEWT_VAL(UWB_CCP_VERBOSE)
static void ccp_postprocess(struct dpl_event * ev);
#endif

/**
 * @fn ccp_timer_init(struct uwb_ccp_instance *ccp, uwb_ccp_role_t role)
 * @brief API to initiate timer for ccp.
 *
 * @param inst  Pointer to struct uwb_ccp_instance.
 * @param role  uwb_ccp_role_t for CCP_ROLE_MASTER, CCP_ROLE_SLAVE, CCP_ROLE_RELAY roles.
 * @return void
 */
static void
ccp_timer_init(struct uwb_ccp_instance *ccp, uwb_ccp_role_t role)
{
    ccp->status.timer_enabled = true;

    dpl_cputime_timer_init(&ccp->timer, ccp_timer_irq, (void *) ccp);

    /* Only reinitialise timer_event if not done */
    if (dpl_event_get_arg(&ccp->timer_event) != (void *) ccp) {
        if (role == CCP_ROLE_MASTER){
            dpl_event_init(&ccp->timer_event, ccp_master_timer_ev_cb, (void *) ccp);
        } else {
            dpl_event_init(&ccp->timer_event, ccp_slave_timer_ev_cb, (void *) ccp);
        }
    }
    dpl_cputime_timer_relative(&ccp->timer, 0);
}

/**
 * @fn ccp_timer_irq(void * arg)
 * @brief ccp_timer_event is in the interrupt context and schedules and tasks on the ccp event queue.
 *
 * @param arg   Pointer to struct uwb_ccp_instance.
 * @return void
 */
static void
ccp_timer_irq(void * arg){
    assert(arg);
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)arg;
    dpl_eventq_put(&ccp->eventq, &ccp->timer_event);
}

/**
 * @fn ccp_master_timer_ev_cb(struct os_event *ev)
 * @brief The OS scheduler is not accurate enough for the timing requirement of an RTLS system.
 * Instead, the OS is used to schedule events in advance of the actual event.
 * The UWB delay start mechanism then takes care of the actual event. This removes the non-deterministic
 * latencies of the OS implementation.
 *
 * @param ev  Pointer to os_events.
 * @return void
 */
static void
ccp_master_timer_ev_cb(struct dpl_event *ev)
{
    int rc;
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));

    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *) dpl_event_get_arg(ev);

    if (!ccp->status.enabled) {
        return;
    }

    CCP_STATS_INC(master_cnt);
    ccp->status.timer_restarted = 0;

    if (ccp_send(ccp, UWB_BLOCKING).start_tx_error) {
        /* CCP failed to send, probably because os_latency wasn't enough
         * margin to get in to prep the frame for sending.
         * NOTE that os_epoch etc will be updated in ccp_send if it fails
         * to send so timer update below will still point to next beacon time */
        if (!ccp->status.enabled) {
            goto disabled;
        }
    }

    if (!ccp->status.timer_restarted && ccp->status.enabled) {
        rc = dpl_cputime_timer_start(&ccp->timer, ccp->os_epoch
            - dpl_cputime_usecs_to_ticks(MYNEWT_VAL(OS_LATENCY))
            + dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(ccp->period))
        );
        if (rc == 0) ccp->status.timer_restarted = 1;
    }
disabled:
    return;
}

/**
 * @fn ccp_slave_timer_ev_cb(struct dpl_event *ev)
 * @brief The OS scheduler is not accurate enough for the timing requirement of an RTLS system.
 * Instead, the OS is used to schedule events in advance of the actual event.
 * The UWB delay start mechanism then takes care of the actual event. This removes the non-deterministic
 * latencies of the OS implementation.
 *
 * @param ev  Pointer to dpl_events.
 * @return void
 */
static void
ccp_slave_timer_ev_cb(struct dpl_event *ev)
{
    int rc;
    uint16_t timeout;
    uint64_t dx_time = 0;
    uint32_t timer_expiry;
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));

    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *) dpl_event_get_arg(ev);
    struct uwb_dev * inst = ccp->dev_inst;

    /* Precalc / update blink frame duration */
    ccp->blink_frame_duration = uwb_phy_frame_duration(inst, sizeof(uwb_ccp_blink_frame_t));

    if (!ccp->status.enabled) {
        return;
    }

    /* Sync lost since earlier, just set a long rx timeout and
     * keep listening */
    if (ccp->status.rx_timeout_error) {
        uwb_set_rx_timeout(inst, MYNEWT_VAL(UWB_CCP_LONG_RX_TO));
        ccp_listen(ccp, 0, UWB_BLOCKING);
        goto reset_timer;
    }

    CCP_STATS_INC(slave_cnt);

    /* Calculate when to expect the ccp packet */
    dx_time = ccp->local_epoch;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
    if (ccp->wcs) {
        struct uwb_wcs_instance * wcs = ccp->wcs;
        dx_time += DPL_FLOAT64_F64_TO_U64(DPL_FLOAT64_MUL(DPL_FLOAT64_U64_TO_F64((uint64_t)ccp->period << 16),
                                                          wcs->normalized_skew));
    }
#else
    dx_time += ((uint64_t)ccp->period << 16);
#endif
    dx_time -= ((uint64_t)ceilf(uwb_usecs_to_dwt_usecs(uwb_phy_SHR_duration(inst) +
                                                       inst->config.rx.timeToRxStable)) << 16);

    timeout = ccp->blink_frame_duration + MYNEWT_VAL(XTALT_GUARD);
#if MYNEWT_VAL(UWB_CCP_MAX_CASCADE_RPTS) != 0
    /* Adjust timeout if we're using cascading ccp in anchors */
    timeout += (ccp->config.tx_holdoff_dly + ccp->blink_frame_duration) * MYNEWT_VAL(UWB_CCP_MAX_CASCADE_RPTS);
#endif
    uwb_set_rx_timeout(inst, timeout);

    ccp_listen(ccp, dx_time, UWB_BLOCKING);
    if(ccp->status.start_rx_error){
        /* Sync lost, set a long rx timeout */
        uwb_set_rx_timeout(inst, MYNEWT_VAL(UWB_CCP_LONG_RX_TO));
        ccp_listen(ccp, 0, UWB_BLOCKING);
    }

reset_timer:
    /* Check if we're still enabled */
    if (!ccp->status.enabled) {
        return;
    }

    /* Ensure timer isn't active */
    dpl_cputime_timer_stop(&ccp->timer);

    if (ccp->status.rx_timeout_error &&
        ccp->missed_frames > MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES)) {
        /* No ccp received, reschedule immediately */
        rc = dpl_cputime_timer_relative(&ccp->timer, 0);
    } else {
        /* ccp received ok, or close enough - reschedule for next ccp event */
        ccp->status.rx_timeout_error = 0;
        timer_expiry = ccp->os_epoch + dpl_cputime_usecs_to_ticks(
            - MYNEWT_VAL(OS_LATENCY)
            + (uint32_t)uwb_dwt_usecs_to_usecs(ccp->period)
            - ccp->blink_frame_duration - inst->config.rx.timeToRxStable);
        rc = dpl_cputime_timer_start(&ccp->timer, timer_expiry);
    }
    if (rc == 0) ccp->status.timer_restarted = 1;
}

/**
 * @fn ccp_task(void *arg)
 * @brief The ccp event queue being run to process timer events.
 *
 * @param arg  Pointer to struct ccp_instance.
 * @return void
 */

static void *
ccp_task(void *arg)
{
    struct uwb_ccp_instance * inst = arg;
    while (1) {
        dpl_eventq_run(&inst->eventq);
    }
    return NULL;
}

/**
 * @fn ccp_tasks_init(struct uwb_ccp_instance * inst)
 * @brief ccp eventq is used.
 *
 * @param inst Pointer to struct uwb_ccp_instance
 * @return void
 */
static void
ccp_tasks_init(struct uwb_ccp_instance * inst)
{
    /* Check if the tasks are already initiated */
    if (!dpl_eventq_inited(&inst->eventq))
    {
        /* Use a dedicate event queue for ccp events */
        dpl_eventq_init(&inst->eventq);
        dpl_task_init(&inst->task_str, "ccp",
                      ccp_task,
                      (void *) inst,
                      inst->task_prio, DPL_WAIT_FOREVER,
                      inst->task_stack,
                      MYNEWT_VAL(UWB_CCP_TASK_STACK_SZ));
    }
}

/**
 * @fn uwb_ccp_set_tof_comp_cb(struct uwb_ccp_instance * inst, uwb_ccp_tof_compensation_cb_t tof_comp_cb)
 * @brief Sets the CB that estimate the tof in dw units to the node with euid provided as
 * paramater. Used to compensate for the tof from the clock source.
 *
 * @param inst        Pointer to struct uwb_ccp_instance
 * @param tof_comp_cb tof compensation callback
 *
 * @return void
 */
void
uwb_ccp_set_tof_comp_cb(struct uwb_ccp_instance * inst, uwb_ccp_tof_compensation_cb_t tof_comp_cb)
{
    inst->tof_comp_cb = tof_comp_cb;
}

/**
 * @fn uwb_ccp_init(struct uwb_dev * inst, uint16_t nframes)
 * @brief Precise timing is achieved by adding a fixed period to the transmission time of the previous frame.
 * This approach solves the non-deterministic latencies caused by the OS. The OS, however, is used to schedule
 * the next transmission event, but the UWB tranceiver controls the actual next transmission time using the uwb_set_delay_start.
 * This function allocates all the required resources. In the case of a large scale deployment multiple instances
 * can be uses to track multiple clock domains.
 *
 * @param inst     Pointer to struct uwb_dev* dev
 * @param nframes  Nominally set to 2 frames for the simple use case. But depending on the interpolation
 * algorithm this should be set accordingly. For example, four frames are required or bicubic interpolation.
 *
 * @return struct uwb_ccp_instance *
 */
struct uwb_ccp_instance *
uwb_ccp_init(struct uwb_dev* dev, uint16_t nframes)
{
    int i;
    assert(dev);
    assert(nframes > 1);

    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_CCP);
    if (ccp == NULL) {
        ccp = (struct uwb_ccp_instance *) calloc(1, sizeof(struct uwb_ccp_instance) + nframes * sizeof(uwb_ccp_frame_t *));
        assert(ccp);
        ccp->status.selfmalloc = 1;
        ccp->nframes = nframes;
        uwb_ccp_frame_t ccp_default = {
            .fctrl = FCNTL_IEEE_BLINK_CCP_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 64-bit addressing).
            .seq_num = 0xFF,
            .rpt_count = 0,
            .rpt_max = MYNEWT_VAL(UWB_CCP_MAX_CASCADE_RPTS)
        };

        for (i = 0; i < ccp->nframes; i++){
            ccp->frames[i] = (uwb_ccp_frame_t *) calloc(1, sizeof(uwb_ccp_frame_t));
            assert(ccp->frames[i]);
            memcpy(ccp->frames[i], &ccp_default, sizeof(uwb_ccp_frame_t));
            ccp->frames[i]->seq_num = 0;
        }

        ccp->dev_inst = dev;
        /* TODO: fixme below */
        ccp->task_prio = dev->task_prio - 0x4;
    }else{
        assert(ccp->nframes == nframes);
    }
    ccp->period = MYNEWT_VAL(UWB_CCP_PERIOD);
    ccp->config = (struct uwb_ccp_config){
        .postprocess = false,
        .tx_holdoff_dly = MYNEWT_VAL(UWB_CCP_RPT_HOLDOFF_DLY),
    };

    dpl_error_t err = dpl_sem_init(&ccp->sem, 0x1);
    assert(err == DPL_OK);

#if MYNEWT_VAL(UWB_CCP_VERBOSE)
    uwb_ccp_set_postprocess(ccp, &ccp_postprocess);    // Using default process
#endif

    ccp->cbs = (struct uwb_mac_interface){
        .id = UWBEXT_CCP,
        .inst_ptr = (void*)ccp,
        .tx_complete_cb = tx_complete_cb,
        .rx_complete_cb = rx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
        .rx_error_cb = error_cb,
        .tx_error_cb = error_cb,
        .reset_cb = reset_cb
    };
    uwb_mac_append_interface(dev, &ccp->cbs);

    ccp_tasks_init(ccp);
    ccp->status.initialized = 1;

#if MYNEWT_VAL(UWB_CCP_STATS)
    int rc = stats_init(
                STATS_HDR(ccp->stat),
                STATS_SIZE_INIT_PARMS(ccp->stat, STATS_SIZE_32),
                STATS_NAME_INIT_PARMS(uwb_ccp_stat_section)
            );
    assert(rc == 0);

#if  MYNEWT_VAL(UWB_DEVICE_0) && !MYNEWT_VAL(UWB_DEVICE_1)
    rc = stats_register("ccp", STATS_HDR(ccp->stat));
#elif  MYNEWT_VAL(UWB_DEVICE_0) && MYNEWT_VAL(UWB_DEVICE_1)
    if (dev->idx == 0)
        rc |= stats_register("ccp0", STATS_HDR(ccp->stat));
    else
        rc |= stats_register("ccp1", STATS_HDR(ccp->stat));
#endif
    assert(rc == 0);
#endif

    return ccp;
}

/**
 * @fn uwb_ccp_free(struct uwb_ccp_instance * inst)
 * @brief Deconstructor
 *
 * @param inst   Pointer to struct uwb_ccp_instance.
 * @return void
 */
void
uwb_ccp_free(struct uwb_ccp_instance * inst)
{
    int i;
    assert(inst);
    inst->status.enabled = 0;
    dpl_sem_release(&inst->sem);
    uwb_mac_remove_interface(inst->dev_inst, inst->cbs.id);

    /* Make sure timer and eventq is stopped */
    dpl_cputime_timer_stop(&inst->timer);
    dpl_eventq_deinit(&inst->eventq);

    if (inst->status.selfmalloc) {
        for (i = 0; i < inst->nframes; i++) {
            free(inst->frames[i]);
        }
        free(inst);
    } else {
        inst->status.initialized = 0;
    }
}

/**
 * @fn uwb_ccp_set_postprocess(struct ccp_instance * inst, dpl_event_fn * postprocess)
 * @brief API that overrides the default post-processing behaviors, replacing the JSON stream with an alternative
 * or an advanced timescale processing algorithm.
 *
 * @param inst              Pointer to struct ccp_instance * ccp.
 * @param postprocess       Pointer to dpl_event_fn.
 * @return void
 */
void
uwb_ccp_set_postprocess(struct uwb_ccp_instance * ccp, dpl_event_fn * postprocess)
{
    dpl_event_init(&ccp->postprocess_event, postprocess, (void *) ccp);
    ccp->config.postprocess = true;
}

#if MYNEWT_VAL(UWB_CCP_VERBOSE)
/**
 * @fn ccp_postprocess(struct dpl_event * ev)
 * @brief API that serves as a place holder for timescale processing and by default is creates json string for the event.
 *
 * @param ev   pointer to dpl_events.
 * @return void
 */
static void
ccp_postprocess(struct dpl_event * ev)
{
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));

    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *) dpl_event_get_arg(ev);
    uwb_ccp_frame_t * previous_frame = ccp->frames[(uint16_t)(ccp->idx-1)%ccp->nframes];
    uwb_ccp_frame_t * frame = ccp->frames[(ccp->idx)%ccp->nframes];
    uint64_t delta = 0;

    if (ccp->config.role == CCP_ROLE_MASTER){
        delta = (frame->transmission_timestamp.timestamp - previous_frame->transmission_timestamp.timestamp);
    } else {
        delta = (frame->reception_timestamp - previous_frame->reception_timestamp);
    }
    delta = delta & ((uint64_t)1<<63)?delta & 0xFFFFFFFFFF :delta;

    dpl_float64_t carrier_integrator = uwb_calc_clock_offset_ratio(ccp->dev_inst, frame->carrier_integrator, UWB_CR_CARRIER_INTEGRATOR);
    ccp_json_t json = {
        .utime = ccp->master_epoch.timestamp,
        .seq = frame->seq_num,
        .ccp = {DPL_FLOAT64_U64_TO_F64(frame->transmission_timestamp.timestamp),
                DPL_FLOAT64_U64_TO_F64(delta)},
        .ppm = DPL_FLOAT64_MUL(carrier_integrator, DPL_FLOAT64_INIT(1e6l))
    };
    ccp_json_write_uint64(&json);
#ifdef __KERNEL__
    size_t n = strlen(json.iobuf);
    json.iobuf[n]='\n';
    json.iobuf[n+1]='\0';
    ccp_chrdev_output(ccp->dev_inst->idx, json.iobuf, n+1);
#else
#ifdef UWB_CCP_VERBOSE
    printf("%s\n",json.iobuf);
#endif
#endif
}
#endif

static void
adjust_for_epoch_to_rm(struct uwb_ccp_instance * ccp, uint16_t epoch_to_rm_us)
{
    ccp->master_epoch.timestamp -= ((uint64_t)epoch_to_rm_us << 16);
    ccp->local_epoch -= ((uint64_t)epoch_to_rm_us << 16);
    ccp->os_epoch -= dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(epoch_to_rm_us));
}

#if MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES) > 0
static void
issue_superframe(struct uwb_ccp_instance * ccp)
{
    struct uwb_dev * inst = ccp->dev_inst;
    struct uwb_mac_interface * lcbs = NULL;

    if (ccp->status.valid &&
        ccp->missed_frames <= MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES)
        ) {
        /* Tolerating a (few) missed ccp-frame. Update time */
        ccp->os_epoch += dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(ccp->period));
        ccp->master_epoch.timestamp += ((uint64_t)ccp->period)<<16;
        ccp->local_epoch += ((uint64_t)ccp->period)<<16;
        CCP_STATS_INC(err_tolerated);

        /* Call all available superframe callbacks */
        if(!(SLIST_EMPTY(&inst->interface_cbs))) {
            SLIST_FOREACH(lcbs, &inst->interface_cbs, next) {
                if (lcbs != NULL && lcbs->superframe_cb) {
                    if(lcbs->superframe_cb((struct uwb_dev*)inst, lcbs)) continue;
                }
            }
        }
    }
}
#endif

/**
 * @fn rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Precise timing is achieved using the reception_timestamp and tracking intervals along with
 * the correction factor. For timescale processing, a postprocessing  callback is placed in the eventq.
 *
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return void
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *)cbs->inst_ptr;

    if (ccp->config.role == CCP_ROLE_MASTER) {
        return true;
    }

    if (inst->fctrl_array[0] != FCNTL_IEEE_BLINK_CCP_64){
        if(dpl_sem_get_count(&ccp->sem) == 0){
            /* We're hunting for a ccp but received something else,
             * set a long timeout and keep listening */
            uwb_adj_rx_timeout(inst, MYNEWT_VAL(UWB_CCP_LONG_RX_TO));
            CCP_STATS_INC(rx_other_frame);
            return true;
        }
        return false;
    }

    if(dpl_sem_get_count(&ccp->sem) != 0){
        //unsolicited inbound
        CCP_STATS_INC(rx_unsolicited);
        return false;
    }

    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"ccp:rx_complete_cb\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));

    uwb_ccp_frame_t * frame = ccp->frames[(ccp->idx+1)%ccp->nframes];  // speculative frame advance

    if (inst->frame_len >= sizeof(uwb_ccp_blink_frame_t) && inst->frame_len <= sizeof(frame->array))
        memcpy(frame->array, inst->rxbuf, sizeof(uwb_ccp_blink_frame_t));
    else
        return true;

    if (inst->status.lde_error)
        return true;

#if MYNEWT_VAL(UWB_CCP_TEST_CASCADE_RPTS)
    if (frame->rpt_count < MYNEWT_VAL(UWB_CCP_TEST_CASCADE_RPTS)) {
        return true;
    }
#endif
    /* A good ccp packet has been received, stop the receiver */
    uwb_stop_rx(inst); //Prevent timeout event

    ccp->idx++; // confirmed frame advance
    ccp->seq_num = frame->seq_num;
    ccp->missed_frames = 0;

    /* Read os_time and correct for interrupt latency */
    uint32_t delta_0 = 0xffffffffU&(uwb_read_systime_lo32(inst) - (uint32_t)(inst->rxtimestamp&0xFFFFFFFFUL));
    ccp->os_epoch = dpl_cputime_get32();
    uint32_t delta_1 = 0xffffffffU&(uwb_read_systime_lo32(inst) - (uint32_t)(inst->rxtimestamp&0xFFFFFFFFUL));
    uint32_t delta = (delta_0>>1) + (delta_1>>1);
    ccp->os_epoch -= dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(delta>>16));
    CCP_STATS_SET(irq_latency, uwb_dwt_usecs_to_usecs(delta>>16));

    CCP_STATS_INC(rx_complete);
    ccp->status.rx_timeout_error = 0;

    if (frame->transmission_timestamp.timestamp < ccp->master_epoch.timestamp ||
        frame->euid != ccp->master_euid) {
        ccp->master_euid = frame->euid;
        ccp->status.valid = (MYNEWT_VAL(UWB_CCP_VALID_THRESHOLD)==0);
        ccp->status.valid_count = 0;
    }else{
        if(ccp->status.valid_count < MYNEWT_VAL(UWB_CCP_VALID_THRESHOLD)-1){
            ccp->status.valid_count++;
        }
        ccp->status.valid |= (ccp->status.valid_count == MYNEWT_VAL(UWB_CCP_VALID_THRESHOLD)-1);
    }

    ccp->master_epoch.timestamp = frame->transmission_timestamp.timestamp;
    ccp->local_epoch = frame->reception_timestamp = inst->rxtimestamp;
    ccp->period = (frame->transmission_interval >> 16);

    /* Adjust for delay between epoch and rmarker */
    adjust_for_epoch_to_rm(ccp, frame->epoch_to_rm_us);

    frame->carrier_integrator = inst->carrier_integrator;
    if (inst->config.rxttcko_enable) {
        frame->rxttcko = inst->rxttcko;
    } else {
        frame->rxttcko = 0;
    }

    /* Compensate for time of flight */
    if (ccp->tof_comp_cb) {
        uint32_t tof_comp = ccp->tof_comp_cb(frame->short_address);
        tof_comp = uwb_ccp_skew_compensation_ui64(ccp, (uint64_t)tof_comp);
        ccp->local_epoch -= tof_comp;
        frame->reception_timestamp = ccp->local_epoch;
    }

    /* Compensate if not receiving the master ccp packet directly */
    if (frame->rpt_count != 0) {
        CCP_STATS_INC(rx_relayed);
        /* Assume ccp intervals are a multiple of 0x10000 dwt usec -> 0x100000000 dwunits */
        uint64_t master_interval = ((frame->transmission_interval/0x100000000UL+1)*0x100000000UL);
        ccp->period = master_interval>>16;
        uint64_t repeat_dly = master_interval - frame->transmission_interval;
        ccp->master_epoch.timestamp = (ccp->master_epoch.timestamp - repeat_dly);
        repeat_dly = uwb_ccp_skew_compensation_ui64(ccp, repeat_dly);
        ccp->local_epoch = (ccp->local_epoch - repeat_dly) & 0x0FFFFFFFFFFUL;
        frame->reception_timestamp = ccp->local_epoch;
        /* master_interval and transmission_interval are expressed as dwt_usecs */
        ccp->os_epoch -= dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs((repeat_dly >> 16)));
        /* Carrier integrator is only valid if direct from the master */
        frame->carrier_integrator = 0;
        frame->rxttcko = 0;
    }

    /* Cascade relay of ccp packet */
    if (ccp->config.role == CCP_ROLE_RELAY && ccp->status.valid && frame->rpt_count < frame->rpt_max) {
        uwb_ccp_frame_t tx_frame;
        memcpy(tx_frame.array, frame->array, sizeof(uwb_ccp_frame_t));

        /* Only replace the short id, retain the euid to know which master this originates from */
        tx_frame.short_address = inst->my_short_address;
        tx_frame.rpt_count++;
        uint64_t tx_timestamp = frame->reception_timestamp;
        tx_timestamp += tx_frame.rpt_count*((uint64_t)ccp->config.tx_holdoff_dly<<16);

        /* Shift frames so as to reduce risk of frames corrupting each other */
        tx_timestamp += (inst->slot_id%4)*(((uint64_t)ccp->blink_frame_duration)<<16);
        tx_timestamp &= 0x0FFFFFFFE00UL;
        uwb_set_delay_start(inst, tx_timestamp);

        /* Need to add antenna delay */
        tx_timestamp += inst->tx_antenna_delay;

        /* Calculate the transmission time of our packet in the masters reference */
        uint64_t tx_delay = (tx_timestamp - frame->reception_timestamp);
        tx_delay = uwb_ccp_skew_compensation_ui64(ccp, tx_delay);
        tx_frame.transmission_timestamp.timestamp += tx_delay;

        /* Adjust the transmission interval so listening units can calculate the
         * original master's timestamp */
        tx_frame.transmission_interval = frame->transmission_interval - tx_delay;

        uwb_write_tx(inst, tx_frame.array, 0, sizeof(uwb_ccp_blink_frame_t));
        uwb_write_tx_fctrl(inst, sizeof(uwb_ccp_blink_frame_t), 0);
        ccp->status.start_tx_error = uwb_start_tx(inst).start_tx_error;
        if (ccp->status.start_tx_error){
            CCP_STATS_INC(tx_relay_error);
        } else {
            CCP_STATS_INC(tx_relay_ok);
        }
    }

    /* Call all available superframe callbacks */
    struct uwb_mac_interface * lcbs = NULL;
    if(!(SLIST_EMPTY(&inst->interface_cbs))) {
        SLIST_FOREACH(lcbs, &inst->interface_cbs, next) {
            if (lcbs != NULL && lcbs->superframe_cb) {
                if(lcbs->superframe_cb((struct uwb_dev*)inst, lcbs)) continue;
            }
        }
    }

    if (ccp->config.postprocess && ccp->status.valid) {
        dpl_eventq_put(dpl_eventq_dflt_get(), &ccp->postprocess_event);
    }

    dpl_sem_release(&ccp->sem);
    return false;
}

/**
 * @fn tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Precise timing is achieved by adding a fixed period to the transmission time of the previous frame. This static
 * function is called on successful transmission of a CCP packet, and this advances the frame index point. Circular addressing is used
 * for the frame addressing. The next dpl_event is scheduled to occur in (MYNEWT_VAL(UWB_CCP_PERIOD) - MYNEWT_VAL(UWB_CCP_OS_LATENCY)) usec
 * from now. This provided a context switch guard zone. The assumption is that the underlying OS will have sufficient time start
 * the call uwb_set_delay_start within ccp_blink.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return bool
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    int rc;
    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&ccp->sem) == 1)
        return false;

    CCP_STATS_INC(tx_complete);
    if (ccp->config.role != CCP_ROLE_MASTER)
        return false;

    uwb_ccp_frame_t * frame = ccp->frames[(++ccp->idx)%ccp->nframes];

    /* Read os_time and correct for interrupt latency */
    uint32_t delta_0 = 0xffffffffU&(uwb_read_systime_lo32(inst) - frame->transmission_timestamp.lo);
    ccp->os_epoch = dpl_cputime_get32();
    uint32_t delta_1 = 0xffffffffU&(uwb_read_systime_lo32(inst) - frame->transmission_timestamp.lo);
    uint32_t delta = (delta_0>>1) + (delta_1>>1);
    ccp->os_epoch -= dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(delta>>16));
    CCP_STATS_SET(irq_latency, uwb_dwt_usecs_to_usecs(delta>>16));

    ccp->local_epoch = frame->transmission_timestamp.lo;
    ccp->master_epoch = frame->transmission_timestamp;
    ccp->period = (frame->transmission_interval >> 16);

    /* Adjust for delay between epoch and rmarker */
    adjust_for_epoch_to_rm(ccp, frame->epoch_to_rm_us);

    if (ccp->status.timer_enabled){
        rc = dpl_cputime_timer_start(&ccp->timer, ccp->os_epoch
            - dpl_cputime_usecs_to_ticks(MYNEWT_VAL(OS_LATENCY))
            + dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(ccp->period))
        );
        if (rc == 0) ccp->status.timer_restarted = 1;
    }
    ccp->status.valid |= ccp->idx > 1;

    /* Call all available superframe callbacks */
    struct uwb_mac_interface * lcbs = NULL;
    if(!(SLIST_EMPTY(&inst->interface_cbs))) {
        SLIST_FOREACH(lcbs, &inst->interface_cbs, next) {
            if (lcbs != NULL && lcbs->superframe_cb) {
                if(lcbs->superframe_cb((struct uwb_dev*)inst, lcbs)) continue;
            }
        }
    }

    if (ccp->config.postprocess && ccp->status.valid)
        dpl_eventq_put(dpl_eventq_dflt_get(), &ccp->postprocess_event);

    if(dpl_sem_get_count(&ccp->sem) == 0){
        dpl_error_t err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);
    }

    return false;
}

/**
 * @fn error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for rx_error_cb of ccp.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return void
 */
static bool
error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&ccp->sem) == 1)
        return false;

    CCP_STATS_INC(txrx_error);
    if(dpl_sem_get_count(&ccp->sem) == 0) {
        if (ccp->config.role != CCP_ROLE_MASTER) {
            ccp->status.rx_error = 1;
            ccp->missed_frames++;
#if MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES) > 0
            issue_superframe(ccp);
#endif
        }
        dpl_error_t err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);
    }
    return true;
}

/**
 * @fn rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for rx_timeout_cb of ccp.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return void
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&ccp->sem) == 1)
        return false;

    if (dpl_sem_get_count(&ccp->sem) == 0){
        ccp->status.rx_timeout_error = 1;
        ccp->missed_frames++;
#if MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES) > 0
        issue_superframe(ccp);
#endif
        DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"ccp:rx_timeout_cb\"}\n",
                dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
        CCP_STATS_INC(rx_timeout);
        dpl_error_t err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);
    }
    return true;
}

/**
 * @fn reset_cb(struct uwb_dev*, struct uwb_mac_interface*)
 * @brief API for reset_cb of ccp.
 *
 * @param inst   Pointer to struct uwb_dev *inst.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return void
 */
static bool
reset_cb(struct uwb_dev *inst, struct uwb_mac_interface * cbs)
{
    struct uwb_ccp_instance * ccp = (struct uwb_ccp_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&ccp->sem) == 0){
        DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"uwb_ccp_reset_cb\"}\n",
                dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
        dpl_error_t err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);
        CCP_STATS_INC(reset);
        return true;
    }
    return false;   // CCP is an observer and should not return true
}

/**
 * @fn ccp_send(struct uwb_ccp_instance *ccp, uwb_ccp_modes_t mode)
 * @brief Start clock calibration packets (CCP) blinks with a pulse repetition period of MYNEWT_VAL(UWB_CCP_PERIOD).
 * Precise timing is achieved by adding a fixed period to the transmission time of the previous frame.
 * This removes the need to explicitly read the syst uwb_ccp_stop(ccp); ime register and the assiciated non-deterministic latencies.
 * This function is static function for internl use. It will force a Half Period Delay Warning is called at
 * out of sequence.
 *
 * @param inst   Pointer to struct uwb_ccp_instance *ccp.
 * @param mode   uwb_dev_modes_t for UWB_BLOCKING, UWB_NON_BLOCKING modes.
 *
 * @return struct ccp_status
 */
static struct uwb_ccp_status
ccp_send(struct uwb_ccp_instance *ccp, uwb_dev_modes_t mode)
{
    assert(ccp);
    struct uwb_dev * inst = ccp->dev_inst;
    CCP_STATS_INC(send);
    uwb_phy_forcetrxoff(inst);
    dpl_error_t err = dpl_sem_pend(&ccp->sem, DPL_TIMEOUT_NEVER);
    assert(err == DPL_OK);

    uwb_ccp_frame_t * previous_frame = ccp->frames[(uint16_t)(ccp->idx)%ccp->nframes];
    uwb_ccp_frame_t * frame = ccp->frames[(ccp->idx+1)%ccp->nframes];
    frame->rpt_count = 0;
    frame->rpt_max = MYNEWT_VAL(UWB_CCP_MAX_CASCADE_RPTS);
    frame->epoch_to_rm_us = uwb_phy_SHR_duration(inst);

    uint64_t timestamp = previous_frame->transmission_timestamp.timestamp
                        + ((uint64_t)ccp->period << 16);

    timestamp = timestamp & 0xFFFFFFFFFFFFFE00ULL; /* Mask off the last 9 bits */
    uwb_set_delay_start(inst, timestamp);
    timestamp += inst->tx_antenna_delay;
    frame->transmission_timestamp.timestamp = timestamp;

    frame->seq_num = ++ccp->seq_num;
    frame->euid = inst->euid;
    frame->short_address = inst->my_short_address;
    frame->transmission_interval = ((uint64_t)ccp->period << 16);

    uwb_write_tx(inst, frame->array, 0, sizeof(uwb_ccp_blink_frame_t));
    uwb_write_tx_fctrl(inst, sizeof(uwb_ccp_blink_frame_t), 0);
    uwb_set_wait4resp(inst, false);
    ccp->status.start_tx_error = uwb_start_tx(inst).start_tx_error;
    if (ccp->status.start_tx_error) {
        uint64_t systime = uwb_read_systime(inst);
        uint64_t late_us = ((systime - timestamp)&UWB_DTU_40BMASK) >> 16;
        CCP_STATS_INC(tx_start_error);
        previous_frame->transmission_timestamp.timestamp = (frame->transmission_timestamp.timestamp
                        + ((uint64_t)ccp->period << 16));
        ccp->idx++;

        /* Handle missed transmission and update epochs as tx_complete would have done otherwise */
        ccp->os_epoch += dpl_cputime_usecs_to_ticks(uwb_dwt_usecs_to_usecs(ccp->period - late_us));
        ccp->os_epoch -= dpl_cputime_usecs_to_ticks(MYNEWT_VAL(OS_LATENCY));
        ccp->master_epoch.timestamp += ((uint64_t)ccp->period)<<16;
        ccp->local_epoch += ((uint64_t)ccp->period)<<16;
#if MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES) > 0
        /* Call all available superframe callbacks */
        struct uwb_mac_interface * lcbs = NULL;
        if(!(SLIST_EMPTY(&inst->interface_cbs))) {
            SLIST_FOREACH(lcbs, &inst->interface_cbs, next) {
                if (lcbs != NULL && lcbs->superframe_cb) {
                    if(lcbs->superframe_cb((struct uwb_dev*)inst, lcbs)) continue;
                }
            }
        }
#endif
        err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);

    }else if(mode == UWB_BLOCKING){
#if MYNEWT_VAL(UWB_CCP_STATS)
        uint32_t margin = 0xffffffffU&(frame->transmission_timestamp.lo - uwb_read_systime_lo32(inst));
        CCP_STATS_SET(os_lat_margin, uwb_dwt_usecs_to_usecs(margin>>16));
#endif
        err = dpl_sem_pend(&ccp->sem, DPL_TIMEOUT_NEVER); // Wait for completion of transactions
        assert(err == DPL_OK);
        err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);
    }
    return ccp->status;
}

/*!
 * @fn ccp_listen(struct ccp_instance *ccp, uint64_t dx_time, uwb_ccp_modes_t mode)
 *
 * @brief Explicit entry function for reveicing a ccp frame.
 *
 * input parameters
 * @param inst - struct ccp_instance *ccp *
 * @param dx_time - if > 0 the absolute time to start listening
 * @param mode - uwb_dev_modes_t for UWB_BLOCKING, UWB_NONBLOCKING modes.
 *
 * output parameters
 *
 * returns struct uwb_ccp_status_t
 */
static struct uwb_ccp_status
ccp_listen(struct uwb_ccp_instance *ccp, uint64_t dx_time, uwb_dev_modes_t mode)
{
    struct uwb_dev * inst = ccp->dev_inst;
    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"uwb_ccp_listen\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    uwb_phy_forcetrxoff(inst);
    dpl_error_t err = dpl_sem_pend(&ccp->sem,  DPL_TIMEOUT_NEVER);
    assert(err == DPL_OK);

    CCP_STATS_INC(listen);

    if (dx_time) {
        uwb_set_delay_start(inst, dx_time);
    }

    ccp->status.rx_timeout_error = 0;
    ccp->status.start_rx_error = uwb_start_rx(inst).start_rx_error;
    if (ccp->status.start_rx_error) {
#if MYNEWT_VAL(UWB_CCP_STATS)
        uint32_t behind = 0xffffffffU&(uwb_read_systime_lo32(inst) - dx_time);
        CCP_STATS_SET(os_lat_behind, uwb_dwt_usecs_to_usecs(behind>>16));
#endif
        /*  */
        CCP_STATS_INC(rx_start_error);
        err = dpl_sem_release(&ccp->sem);
        assert(err == DPL_OK);
    }else if(mode == UWB_BLOCKING){
#if MYNEWT_VAL(UWB_CCP_STATS)
        if (dx_time) {
            uint32_t margin = 0xffffffffU&(dx_time - uwb_read_systime_lo32(inst));
            CCP_STATS_SET(os_lat_margin, uwb_dwt_usecs_to_usecs(margin>>16));
        }
#endif
        /* Wait for completion of transactions */
        err = dpl_sem_pend(&ccp->sem, dpl_time_ms_to_ticks32(4*MYNEWT_VAL(UWB_CCP_LONG_RX_TO)/1000));
        if (err==DPL_TIMEOUT) {
            CCP_STATS_INC(sem_timeout);
        }
        if(dpl_sem_get_count(&ccp->sem) == 0){
            err = dpl_sem_release(&ccp->sem);
            assert(err == DPL_OK);
        }
    }
    return ccp->status;
}

/**
 * @fn ccp_start(struct uwb_ccp_instance, uwb_ccp_role_t role)
 * @brief API to start clock calibration packets (CCP) blinks.
 * With a pulse repetition period of MYNEWT_VAL(UWB_CCP_PERIOD).
 *
 * @param inst   Pointer to struct uwb_ccp_instance.
 * @param role   struct uwb_ccp_role of CCP_ROLE_MASTER, CCP_ROLE_SLAVE, CCP_ROLE_RELAY.
 *
 * @return void
 */
void
uwb_ccp_start(struct uwb_ccp_instance *ccp, uwb_ccp_role_t role)
{
    struct uwb_dev * inst = ccp->dev_inst;
    uint16_t epoch_to_rm = uwb_phy_SHR_duration(inst);

    // Initialise frame timestamp to current time
    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"uwb_ccp_start\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    assert(ccp);
    ccp->idx = 0x0;
    ccp->status.valid = false;
    ccp->master_euid = 0x0;
    uwb_ccp_frame_t * frame = ccp->frames[(ccp->idx)%ccp->nframes];
    ccp->config.role = role;
    ccp->status.enabled = 1;

    /* Setup CCP to send/listen for the first packet ASAP */
    ccp->os_epoch = dpl_cputime_get32() - epoch_to_rm;
    uint64_t ts = (uwb_read_systime(inst) - (((uint64_t)ccp->period)<<16))&UWB_DTU_40BMASK;
    ts += ((uint64_t)ccp->config.tx_holdoff_dly + 2 * MYNEWT_VAL(OS_LATENCY))<<16;

    if (ccp->config.role == CCP_ROLE_MASTER){
        ccp->local_epoch = frame->transmission_timestamp.lo = ts;
        frame->transmission_timestamp.hi = 0;
    } else {
        ccp->local_epoch = frame->reception_timestamp = ts;
    }
    ccp->local_epoch -= epoch_to_rm;
    ccp->local_epoch &= UWB_DTU_40BMASK;

    ccp_timer_init(ccp, role);
}
EXPORT_SYMBOL(uwb_ccp_start);

/**
 * @fn ccp_stop(struct ccp_instance * inst)
 * @brief API to stop clock calibration packets (CCP) blinks / receives.
 *
 * @param inst   Pointer to struct uwb_ccp_instance.
 * @return void
 */
void
uwb_ccp_stop(struct uwb_ccp_instance *ccp)
{
    assert(ccp);
    DIAGMSG("{\"utime\": %"PRIu32",\"msg\": \"uwb_ccp_stop\"}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    ccp->status.enabled = 0;
    dpl_cputime_timer_stop(&ccp->timer);
    if(dpl_sem_get_count(&ccp->sem) == 0){
        uwb_phy_forcetrxoff(ccp->dev_inst);
        if(dpl_sem_get_count(&ccp->sem) == 0){
            dpl_error_t err = dpl_sem_release(&ccp->sem);
            assert(err == DPL_OK);
        }
    }
}
EXPORT_SYMBOL(uwb_ccp_stop);

uint64_t uwb_ccp_skew_compensation_ui64(struct uwb_ccp_instance *ccp, uint64_t value)
{
#if MYNEWT_VAL(UWB_WCS_ENABLED)
    struct uwb_wcs_instance * wcs = ccp->wcs;
    if (!wcs) return value;
    // value *= (1.0l - fractional_skew);
    value = DPL_FLOAT64_F64_TO_U64(DPL_FLOAT64_MUL( DPL_FLOAT64_U64_TO_F64(value), DPL_FLOAT64_SUB( DPL_FLOAT64_INIT(1.0l),wcs->fractional_skew)));
#endif
    return value;
}

dpl_float64_t uwb_ccp_skew_compensation_f64(struct uwb_ccp_instance *ccp,  dpl_float64_t value)
{
#if MYNEWT_VAL(UWB_WCS_ENABLED)
    struct uwb_wcs_instance * wcs = ccp->wcs;
    if (!wcs) return value;
    // value *= (1.0l - fractional_skew);
    value = DPL_FLOAT64_MUL(value, DPL_FLOAT64_SUB( DPL_FLOAT64_INIT(1.0l),wcs->fractional_skew));
#endif
    return value;
}


/**
 * @fn uwb_ccp_pkg_init(void)
 * @brief API to initialise the package, only one ccp service required in the system.
 *
 * @return void
 */
void
uwb_ccp_pkg_init(void)
{
    int i;
    struct uwb_dev *udev;
    struct uwb_ccp_instance * ccp __attribute__((unused));
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"uwb_ccp_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        ccp = uwb_ccp_init(udev, 2);
#ifdef __KERNEL__
        ccp_sysfs_init(ccp);
        ccp_chrdev_create(udev->idx);
        pr_info("uwbccp: To start service: echo 1 > /sys/kernel/uwbcore/uwbccp%d/role; echo 1 > /sys/kernel/uwbcore/uwbccp%d/enable; cat /dev/uwbccp%d\n",
                udev->idx, udev->idx, udev->idx);
#endif  /* __KERNEL__ */
    }

}

/**
 * @fn uwb_ccp_pkg_down(void)
 * @brief Uninitialise ccp
 *
 * @return void
 */
int
uwb_ccp_pkg_down(int reason)
{
    int i;
    struct uwb_dev *udev;
    struct uwb_ccp_instance * ccp;

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_CCP);
        if (!ccp) {
            continue;
        }
        if (ccp->status.enabled) {
            uwb_ccp_stop(ccp);
        }
#if __KERNEL__
        ccp_chrdev_destroy(udev->idx);
        ccp_sysfs_deinit(udev->idx);
#endif
        uwb_ccp_free(ccp);
    }

    return 0;
}
