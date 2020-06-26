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
#include <uwb_wcs/uwb_wcs.h>
#include <rtdoa/rtdoa.h>
#include <rtdoa_node/rtdoa_node.h>
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

static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

static struct uwb_mac_interface g_cbs = {
    .id = UWBEXT_RTDOA,
    .inst_ptr = 0,
    .tx_complete_cb = tx_complete_cb,
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
void rtdoa_node_pkg_init(void)
{
    struct rtdoa_instance *rtdoa = 0;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %lu,\"msg\": \"rtdoa_node_pkg_init\"}\n", os_cputime_ticks_to_usecs(os_cputime_get32()));
#endif
#if MYNEWT_VAL(UWB_DEVICE_0)
    g_cbs.inst_ptr = rtdoa = rtdoa_init(uwb_dev_idx_lookup(0), &g_config, MYNEWT_VAL(RTDOA_NFRAMES));
    rtdoa_set_frames(rtdoa, MYNEWT_VAL(RTDOA_NFRAMES));
#endif
    uwb_mac_append_interface(uwb_dev_idx_lookup(0), &g_cbs);

    /* Assume that the ccp has been added to the dev instance before rtdoa */
    rtdoa->ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_CCP);
}


/**
 * API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
rtdoa_node_free(struct uwb_dev * inst)
{
    assert(inst);
    uwb_mac_remove_interface(inst, UWBEXT_RTDOA);
}

struct uwb_dev_status
rtdoa_request(struct rtdoa_instance *rtdoa, uint64_t delay)
{
    assert(rtdoa);
    struct uwb_dev * inst = rtdoa->dev_inst;
    assert(inst);
    /* This function executes on the device that initiates the rtdoa sequence */
    os_error_t err = os_sem_pend(&rtdoa->sem,  OS_TIMEOUT_NEVER);
    assert(err == OS_OK);
    RTDOA_STATS_INC(rtdoa_request);

    rtdoa->req_frame = rtdoa->frames[rtdoa->idx%rtdoa->nframes];
    rtdoa_request_frame_t * frame = (rtdoa_request_frame_t *) rtdoa->req_frame;

    frame->seq_num = ++rtdoa->seq_num;
    frame->code = UWB_DATA_CODE_RTDOA_REQUEST;
    frame->src_address = inst->my_short_address;
    frame->dst_address = UWB_BROADCAST_ADDRESS;
    frame->slot_modulus = MYNEWT_VAL(RTDOA_NNODES);
    frame->rpt_count = 0;
    frame->rpt_max = MYNEWT_VAL(RTDOA_MAX_CASCADE_RPTS);

    uwb_set_delay_start(inst, delay);

    /* Calculate tx_timestamp, really use wcs here?!?! */
    struct uwb_ccp_instance *ccp = rtdoa->ccp;
    struct uwb_wcs_instance * wcs = ccp->wcs;
    frame->tx_timestamp = (uwb_wcs_local_to_master64(wcs, delay) & 0xFFFFFFFFFFFFFE00ULL);
    frame->tx_timestamp += inst->tx_antenna_delay;
    /* Also set the local rx_timestamp to allow us to also transmit in the next part */
    rtdoa->req_frame->rx_timestamp = frame->tx_timestamp;

    uwb_write_tx(inst, frame->array, 0, sizeof(rtdoa_request_frame_t));
    uwb_write_tx_fctrl(inst, sizeof(rtdoa_request_frame_t), 0);
    uwb_set_wait4resp(inst, false);

    if (uwb_start_tx(inst).start_tx_error) {
        RTDOA_STATS_INC(start_tx_error);
        if (os_sem_get_count(&rtdoa->sem) == 0) {
            err = os_sem_release(&rtdoa->sem);
            assert(err == OS_OK);
        }
    } else {
        err = os_sem_pend(&rtdoa->sem, OS_TIMEOUT_NEVER); // Wait for completion of transactions
        assert(err == OS_OK);
        err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
    }
    return inst->status;
}


static struct uwb_dev_status
tx_rtdoa_response(struct rtdoa_instance * rtdoa)
{
    assert(rtdoa);
    /* This function executes on the device that responds to a rtdoa request */

    struct uwb_dev * inst = rtdoa->dev_inst;
    struct uwb_rng_config * config = &rtdoa->config;

    /* Rx-timestamp will be compensated for relay */
    uint64_t dx_time = rtdoa->req_frame->rx_timestamp;
    /* usecs to dwt usecs? */
    uint8_t slot_idx = inst->slot_id % rtdoa->req_frame->slot_modulus + 1;
    dx_time += (rtdoa_usecs_to_response(inst, (rtdoa_request_frame_t*)rtdoa->req_frame, slot_idx, config,
                                        uwb_phy_frame_duration(inst, sizeof(rtdoa_response_frame_t))) << 16);

    RTDOA_STATS_INC(rtdoa_response);

    rtdoa_response_frame_t * frame = (rtdoa_response_frame_t *) rtdoa->frames[++rtdoa->idx%rtdoa->nframes];

    frame->seq_num = ++rtdoa->seq_num;
    frame->code = UWB_DATA_CODE_RTDOA_RESP;
    frame->src_address = inst->my_short_address;
    frame->dst_address = UWB_BROADCAST_ADDRESS;
    frame->slot_id = inst->slot_id%rtdoa->req_frame->slot_modulus + 1;

    uwb_set_delay_start(inst, (dx_time&0x000000FFFFFFFE00ULL));

    /* Calculate tx-time */
    frame->tx_timestamp = rtdoa_local_to_master64(inst, (dx_time&0x000000FFFFFFFE00ULL) +
                                                  inst->tx_antenna_delay,
                                                  rtdoa->req_frame);

    uwb_write_tx(inst, frame->array, 0, sizeof(rtdoa_response_frame_t));
    uwb_write_tx_fctrl(inst, sizeof(rtdoa_response_frame_t), 0);
    uwb_set_wait4resp(inst, false);

    if (uwb_start_tx(inst).start_tx_error) {
        RTDOA_STATS_INC(start_tx_error);
        if (os_sem_get_count(&rtdoa->sem) == 0) {
            os_error_t err = os_sem_release(&rtdoa->sem);
            assert(err == OS_OK);
        }
    }

    return inst->status;
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
    if(os_sem_get_count(&rtdoa->sem) == 1) {
        return false;
    }

    if(os_sem_get_count(&rtdoa->sem) == 0){
        os_error_t err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
        RTDOA_STATS_INC(rx_timeout);
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
    else
        return false;
}

/**
 * API for transmit complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct rtdoa_instance * rtdoa = (struct rtdoa_instance *)cbs->inst_ptr;
    assert(rtdoa);

    /* If we sent the request or a repeat of the request, send response now */
    rtdoa_request_frame_t * frame = (rtdoa_request_frame_t *) rtdoa->frames[rtdoa->idx%rtdoa->nframes];
    if (frame->code == UWB_DATA_CODE_RTDOA_REQUEST) {
        /* Transmit response packet */
        tx_rtdoa_response(rtdoa);
        // printf("dx_time %d %llx d:%llx\n", slot_idx, dx_time, dx_time - rtdoa->req_frame->rx_timestamp);
        return true;
    }

    if(os_sem_get_count(&rtdoa->sem) == 0){
        os_error_t err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
        return true;
    } else {
        return false;
    }
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
    struct uwb_ccp_instance * ccp = rtdoa->ccp;
    struct uwb_wcs_instance * wcs = ccp->wcs;

    if(inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(os_sem_get_count(&rtdoa->sem) == 1){
        // unsolicited inbound
        RTDOA_STATS_INC(rx_unsolicited);
        return false;
    }

    //struct uwb_rng_config * config = &rtdoa->config;
    rtdoa_request_frame_t * _frame = (rtdoa_request_frame_t * )inst->rxbuf;

    if (_frame->dst_address != inst->my_short_address && _frame->dst_address != UWB_BROADCAST_ADDRESS)
        return true;

    RTDOA_STATS_INC(rx_complete);

    switch(_frame->code){
        case UWB_DATA_CODE_RTDOA_REQUEST:
            {
                // This code executes on the device that is responding to a request
                if (inst->frame_len < sizeof(rtdoa_request_frame_t))
                    break;

                rtdoa_frame_t * frame = (rtdoa_frame_t *) rtdoa->frames[(++rtdoa->idx)%rtdoa->nframes];
                rtdoa->req_frame = frame;
                memcpy(frame->array, inst->rxbuf, sizeof(rtdoa_request_frame_t));

                /* Deliberately in local timeframe */
                frame->rx_timestamp = inst->rxtimestamp;

                /* Compensate for time of flight using the ccp function */
                if (ccp->tof_comp_cb) {
                    frame->rx_timestamp -= ccp->tof_comp_cb(frame->src_address)*(1.0l - wcs->fractional_skew);
                }

                /* Compensate for relays, TODO: Compensate more accurately as
                 * tx time is locked down to 125MHz clock */
                if (frame->rpt_count != 0) {
                    RTDOA_STATS_INC(rx_relayed);
                    uint32_t repeat_dly = frame->rpt_count*rtdoa->config.tx_holdoff_delay;
                    frame->rx_timestamp -= ((uint64_t)repeat_dly << 16)*(1.0l - wcs->fractional_skew);
                }

                /* Send a cascade relay if this is an ok relay */
                if (frame->rpt_count < frame->rpt_max) {
                    rtdoa_request_frame_t tx_frame;
                    memcpy(tx_frame.array, frame->array, sizeof(tx_frame));
                    tx_frame.src_address = inst->my_short_address;
                    tx_frame.rpt_count++;
                    uint64_t tx_timestamp = inst->rxtimestamp + tx_frame.rpt_count*((uint64_t)ccp->config.tx_holdoff_dly<<16);
                    tx_timestamp &= 0x0FFFFFFFE00UL;
                    uwb_set_delay_start(inst, tx_timestamp);
                    tx_timestamp += inst->tx_antenna_delay;

                    tx_frame.tx_timestamp = rtdoa_local_to_master64(inst, tx_timestamp, rtdoa->req_frame);
                    uwb_write_tx_fctrl(inst, sizeof(tx_frame), 0);
                    uwb_write_tx(inst, tx_frame.array, 0, sizeof(tx_frame));
                    if (uwb_start_tx(inst).start_tx_error) {
                        RTDOA_STATS_INC(tx_relay_error);
                        if(os_sem_get_count(&rtdoa->sem) == 0){
                            os_sem_release(&rtdoa->sem);
                        }
                    } else {
                        RTDOA_STATS_INC(tx_relay_ok);
                    }
                } else {
                    /* Queue up response message directly if not relaying request */
                    tx_rtdoa_response(rtdoa);
                }

                break;
            }
        default:
            return false;
            break;
        }
    return true;
}
