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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <os/os.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include "bsp/bsp.h"

#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwb/uwb_ftypes.h>
#include <uwb_ccp/uwb_ccp.h>
#include <rtdoa/rtdoa.h>
#include <rtdoa_tag/rtdoa_tag.h>
#include <uwb_wcs/uwb_wcs.h>
#include <dsp/polyval.h>
#include <uwb_rng/slots.h>

#define WCS_DTU MYNEWT_VAL(WCS_DTU)

#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(RTDOA_TX_HOLDOFF),       // Send Time delay in usec.
    .rx_timeout_delay = MYNEWT_VAL(RTDOA_RX_TIMEOUT),       // Receive response timeout in usec
    .tx_guard_delay = MYNEWT_VAL(RTDOA_TX_GUARD_DELAY)
};

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface *);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

static struct uwb_mac_interface g_cbs = {
    .id = UWBEXT_RTDOA,
    .rx_complete_cb = rx_complete_cb,
    .rx_timeout_cb = rx_timeout_cb,
    .rx_error_cb = rx_error_cb,
    .reset_cb = reset_cb
};

/**
 * API to initialise the rng_ss package.
 *
 *
 * @return void
 */
void
rtdoa_tag_pkg_init(void)
{
    struct rtdoa_instance *rtdoa = 0;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %lu,\"msg\": \"rtdoa_tag_pkg_init\"}\n", os_cputime_ticks_to_usecs(os_cputime_get32()));
#endif

    struct uwb_dev *udev = uwb_dev_idx_lookup(0);
#if MYNEWT_VAL(UWB_DEVICE_0)
    g_cbs.inst_ptr = rtdoa = rtdoa_init(udev, &g_config, MYNEWT_VAL(RTDOA_NFRAMES));
    rtdoa_set_frames(rtdoa, MYNEWT_VAL(RTDOA_NFRAMES));
#endif
    uwb_mac_append_interface(udev, &g_cbs);

    /* Assume that the ccp has been added to the dev instance before rtdoa */
    rtdoa->ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_CCP);
}


/**
 * API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
rtdoa_tag_free(struct uwb_dev * inst)
{
    assert(inst);
    uwb_mac_remove_interface(inst, UWBEXT_RTDOA);
}


/**
 * API for receive error callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct rtdoa_instance * rtdoa = (struct rtdoa_instance *)cbs->inst_ptr;
    if(inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(os_sem_get_count(&rtdoa->sem) == 0){
        RTDOA_STATS_INC(rx_error);
        os_error_t err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
        return true;
    }
    return false;
}


/**
 * API for receive timeout callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct rtdoa_instance * rtdoa = (struct rtdoa_instance *)cbs->inst_ptr;
    if(os_sem_get_count(&rtdoa->sem) == 0) {
        RTDOA_STATS_INC(rx_timeout);
        os_error_t err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
    } else {
        return false;
    }
    return true;
}


/**
 * API for reset_cb of rtdoa interface
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return true on sucess
 */
static bool
reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct rtdoa_instance * rtdoa = (struct rtdoa_instance *)cbs->inst_ptr;
    if(os_sem_get_count(&rtdoa->sem) == 0){
        os_error_t err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
        RTDOA_STATS_INC(reset);
        return true;
    }
    return false;
}

/**
 * API for receive complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct rtdoa_instance * rtdoa = (struct rtdoa_instance *)cbs->inst_ptr;
    int64_t new_timeout;
    struct uwb_ccp_instance *ccp = rtdoa->ccp;
    struct uwb_wcs_instance * wcs = ccp->wcs;

    if(inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(os_sem_get_count(&rtdoa->sem) == 1){
        // unsolicited inbound
        RTDOA_STATS_INC(rx_unsolicited);
        return false;
    }

    rtdoa_request_frame_t * _frame = (rtdoa_request_frame_t * )inst->rxbuf;

    if (_frame->dst_address != inst->my_short_address &&
        _frame->dst_address != UWB_BROADCAST_ADDRESS) {
        return true;
    }

    RTDOA_STATS_INC(rx_complete);

    switch(_frame->code){
        case UWB_DATA_CODE_RTDOA_REQUEST:
        {
            // The initial packet in the rtdoa sequence either from the master or
            // one of the relaying nodes
            if (inst->frame_len < sizeof(rtdoa_request_frame_t)) {
                break;
            }
            RTDOA_STATS_INC(rtdoa_request);

            rtdoa_frame_t * frame = (rtdoa_frame_t *) rtdoa->frames[(++rtdoa->idx)%rtdoa->nframes];
            rtdoa->req_frame = frame;
            memcpy(frame->array, inst->rxbuf, sizeof(rtdoa_request_frame_t));
            memcpy(&frame->diag, inst->rxdiag, inst->rxdiag->rxd_len);

            /* Deliberately in local timeframe */
            frame->rx_timestamp = inst->rxtimestamp;

            /* Compensate for relays */
            uint32_t repeat_dly = 0;
            if (frame->rpt_count != 0) {
                RTDOA_STATS_INC(rx_relayed);
                repeat_dly = frame->rpt_count*rtdoa->config.tx_holdoff_delay;
                frame->rx_timestamp -= (repeat_dly << 16)*(1.0l - wcs->fractional_skew);
            }

            /* A good rtdoa_req packet has been received, stop the receiver */
            uwb_stop_rx(inst);
            /* Adjust timeout and delayed start to match when the responses will arrive */
            uint64_t dx_time = inst->rxtimestamp - repeat_dly;
            dx_time += (rtdoa_usecs_to_response(inst, (rtdoa_request_frame_t*)rtdoa->req_frame, 0, &rtdoa->config,
                            uwb_phy_frame_duration(inst, sizeof(rtdoa_response_frame_t))) << 16);

            /* Subtract the preamble time */
            dx_time -= uwb_phy_SHR_duration(inst);
            uwb_set_delay_start(inst, dx_time);
            if(uwb_start_rx(inst).start_rx_error){
                os_sem_release(&rtdoa->sem);
                RTDOA_STATS_INC(start_rx_error);
            }

            /* Set new timeout */
            new_timeout = ((int64_t)rtdoa->timeout - (int64_t)inst->rxtimestamp) >> 16;
            if (new_timeout < 1) new_timeout = 1;
            uwb_set_rx_timeout(inst, (uint16_t)new_timeout);
            /* Early return as we don't need to adjust timeout again */
            return true;
            break;
        }
        case UWB_DATA_CODE_RTDOA_RESP:
        {
            // The packets following the initial request from all nodes
            if (inst->frame_len < sizeof(rtdoa_response_frame_t)) {
                break;
            }
            RTDOA_STATS_INC(rtdoa_response);

            rtdoa_frame_t * frame = (rtdoa_frame_t *) rtdoa->frames[(++rtdoa->idx)%rtdoa->nframes];
            memcpy(frame->array, inst->rxbuf, sizeof(rtdoa_request_frame_t));
            memcpy(&frame->diag, inst->rxdiag, inst->rxdiag->rxd_len);
            frame->rx_timestamp = inst->rxtimestamp;
            break;
        }
        default:
            return false;
            break;
    }

    /* Adjust existing timeout instead of resetting it (faster) */
    new_timeout = ((int64_t)rtdoa->timeout - (int64_t)inst->rxtimestamp) >> 16;
    if (new_timeout < 1) new_timeout = 1;
    uwb_adj_rx_timeout(inst, new_timeout);
    return true;
}
