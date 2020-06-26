/**
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <uwb/uwb.h>
#include <uwb_rng/uwb_rng.h>
#include <syscfg/syscfg.h>
#include <sysinit/sysinit.h>
#include <dpl/dpl.h>

#if MYNEWT_VAL(DW1000_DEVICE_0)
#include <dw1000/dw1000_hal.h>
#endif
#if MYNEWT_VAL(TDMA_ENABLED)
#include <tdma/tdma.h>
#endif
#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

#include "uwbcfg/uwbcfg.h"
static bool g_uwb_config_needs_update = false;
static int
uwb_config_updated()
{
    g_uwb_config_needs_update = true;
    return 0;
}

static struct uwbcfg_cbs uwb_cb = {
    .uc_update = uwb_config_updated
};

int ev_fd;
struct dpl_sem g_spi0_sem;
pthread_t t1;
static const struct dw1000_dev_cfg dw1000_0_cfg = {
    .spi_sem = &g_spi0_sem,
    .spi_num = 0,
};

void dw1000_interrupt_ev_cb(struct dpl_event *ev);
static bool error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static void slot_complete_cb(struct dpl_event * ev);

/*!
 * @fn slot_cb(struct dpl_event * ev)
 *
 * @brief In this example slot_cb is used to initiate a range request. The slot_cb is scheduled
 * MYNEWT_VAL(OS_LATENCY) in advance of the transmit epoch and a delayed start request is issued in advance of
 * the required epoch. The transmission timing is controlled precisely by the DW1000 with the transmission time
 * defined by the value of the dw_time variable. If the OS_LATENCY value is set too small the range request
 * function will report a start_tx_error. In a synchronized network, the node device switches the transceiver
 * to receiver mode for the same epoch; and will either receive the inbound frame or timeout after the frame
 * duration as elapsed. This ensures that the transceiver is in receive mode for the minimum time required.
 *
 * input parameters
 * @param inst - struct dpl_event *
 *
 * output parameters
 *
 * returns none
 */
static void
slot_cb(struct dpl_event *ev){
    assert(ev);
    tdma_slot_t * slot = (tdma_slot_t *) dpl_event_get_arg(ev);
    tdma_instance_t * tdma = slot->parent;
    uint16_t idx = slot->idx;
    struct uwb_rng_instance *rng = (struct uwb_rng_instance*)slot->arg;

    uint64_t dx_time = tdma_tx_slot_start(tdma, idx) & 0xFFFFFFFFFE00UL;

    if (g_uwb_config_needs_update) {
        uwb_mac_config(tdma->dev_inst, NULL);
        uwb_txrf_config(tdma->dev_inst, &tdma->dev_inst->config.txrf);
        g_uwb_config_needs_update = false;
    }

    /* Range with the clock master by default */
    struct uwb_ccp_instance *ccp = tdma->ccp;
    uint16_t node_address = ccp->frames[ccp->idx%ccp->nframes]->short_address;

    /* Select single-sided or double sided twr every second slot */
    int mode = UWB_DATA_CODE_SS_TWR_ACK;
#if 0
    if ((slot->idx&7)==1) {
        mode = UWB_DATA_CODE_SS_TWR;
    }
    if ((slot->idx&7)==2) {
        mode = UWB_DATA_CODE_SS_TWR_EXT;
    }
    if ((slot->idx&7)==3) {
        mode = UWB_DATA_CODE_DS_TWR;
    }
    if ((slot->idx&7)==4) {
        mode = UWB_DATA_CODE_DS_TWR_EXT;
    }
#endif
   // printf("sl\n");
    rng->control.delay_start_enabled = 0;
    uwb_rng_request_delay_start(rng, node_address, dx_time, mode);
//    printf("%llu\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
}

/*!
 * @fn complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 *
 * @brief This callback is part of the  struct uwb_mac_interface extension interface and invoked of the completion of a range request
 * in the context of this example. The struct uwb_mac_interface is in the interrupt context and is used to schedule events an event queue.
 * Processing should be kept to a minimum giving the interrupt context. All algorithms activities should be deferred to a thread on an event queue.
 * The callback should return true if and only if it can determine if it is the sole recipient of this event.
 *
 * NOTE: The MAC extension interface is a link-list of callbacks, subsequent callbacks on the list will be not be called in the
 * event of returning true.
 *
 * @param inst  - struct uwb_dev *
 * @param cbs   - struct uwb_mac_interface *
 *
 * output parameters
 *
 * returns bool
 */
/* The timer callout */
static struct dpl_event slot_event = {0};
static uint16_t g_idx_latest;

static bool
complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED)) {
        return false;
    }
    struct uwb_rng_instance* rng = (struct uwb_rng_instance*)cbs->inst_ptr;
    g_idx_latest = (rng->idx)%rng->nframes; // Store valid frame pointer
    if (!dpl_event_is_queued(&slot_event)) {
        dpl_event_init(&slot_event, slot_complete_cb, rng);
        dpl_eventq_put(dpl_eventq_dflt_get(), &slot_event);
    }
    return true;
}

/*!
 * @fn slot_complete_cb(struct os_event * ev)
 *
 * @brief In the example this function represents the event context processing of the received range request.
 * In this case, a JSON string is constructed and written to stdio. See the ./apps/matlab or ./apps/python folders for examples on
 * how to parse and render these results.
 *
 * input parameters
 * @param inst - struct os_event *
 * output parameters
 * returns none
 */
static void
slot_complete_cb(struct dpl_event * ev){
    assert(ev != NULL);
}

/*!
 * @fn error_cb(struct os_event *ev)
 *
 * @brief This callback is in the interrupt context and is called on error event.
 * In this example just log event.
 * Note: interrupt context so overlapping IO is possible
 * input parameters
 * @param inst - struct uwb_dev * inst
 *
 * output parameters
 *
 * returns none
 */
static bool
error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED)) {
        return false;
    }

    uint32_t utime = os_cputime_ticks_to_usecs(os_cputime_get32());
    if (inst->status.start_rx_error)
        printf("{\"utime\": %lu,\"msg\": \"start_rx_error,%s:%d\"}\n",utime, __FILE__, __LINE__);
    if (inst->status.start_tx_error)
        printf("{\"utime\": %lu,\"msg\": \"start_tx_error,%s:%d\"}\n",utime, __FILE__, __LINE__);
    if (inst->status.rx_error)
        printf("{\"utime\": %lu,\"msg\": \"rx_error,%s:%d\"}\n",utime, __FILE__, __LINE__);

    return true;
}

int main(int argc, char **argv){
    int rc;

    sysinit();

    struct uwb_dev *udev = uwb_dev_idx_lookup(0);
    struct uwb_rng_instance* rng = (struct uwb_rng_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_RNG);
    assert(rng);

    struct uwb_mac_interface cbs = {
        .id = UWBEXT_APP0,
        .inst_ptr = rng,
        .tx_error_cb = error_cb,
        .rx_error_cb = error_cb,
        .complete_cb = complete_cb
    };
    uwb_mac_append_interface(udev, &cbs);

    uwbcfg_register(&uwb_cb);

    tdma_instance_t * tdma = (tdma_instance_t*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_TDMA);
    assert(tdma);
    uwb_ccp_start(tdma->ccp, CCP_ROLE_SLAVE);

    uint32_t utime = os_cputime_ticks_to_usecs(os_cputime_get32());
    printf("{\"utime\": %lu,\"exec\": \"%s\"}\n",utime,__FILE__);
    printf("{\"device_id\"=\"%lX\"",udev->device_id);
    printf(",\"panid=\"%X\"",udev->pan_id);
    printf(",\"addr\"=\"%X\"",udev->uid);
    printf(",\"part_id\"=\"%lX\"",(uint32_t)(udev->euid&0xffffffff));
    printf(",\"lot_id\"=\"%lX\"}\n",(uint32_t)(udev->euid>>32));
    printf("{\"utime\": %lu,\"msg\": \"frame_duration = %d usec\"}\n",utime, uwb_phy_frame_duration(udev, sizeof(twr_frame_final_t)));
    printf("{\"utime\": %lu,\"msg\": \"SHR_duration = %d usec\"}\n",utime, uwb_phy_SHR_duration(udev));
    printf("{\"utime\": %lu,\"msg\": \"holdoff = %d usec\"}\n",utime,(uint16_t)ceilf(uwb_dwt_usecs_to_usecs(rng->config.tx_holdoff_delay)));
#if MYNEWT_VAL(DW1000_DEVICE_0)
    // Using DW GPIO5 and GPIO6 to study timing.
    dw1000_dev_instance_t * inst = hal_dw1000_inst(0);
    dw1000_gpio5_config_ext_txe( inst);
    dw1000_gpio6_config_ext_rxe( inst);
#endif
    /* Slot 0:ccp, 1+ twr */
    for (uint16_t i = 1; i < MYNEWT_VAL(TDMA_NSLOTS)-1; i++)
        tdma_assign_slot(tdma, slot_cb, i, (void*)rng);
#if MYNEWT_VAL(RNG_VERBOSE) > 1
    udev->config.rxdiag_enable = 1;
#endif

    while (1) {
        dpl_eventq_run(dpl_eventq_dflt_get());
    }
    assert(0);
    return rc;
}
