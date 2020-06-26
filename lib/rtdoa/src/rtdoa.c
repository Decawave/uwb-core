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
#include <os/os.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include "bsp/bsp.h"
#include <math.h>

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <rtdoa/rtdoa.h>
#include <uwb_rng/uwb_rng.h>
#include <uwb_wcs/uwb_wcs.h>
#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif
#include <uwb_rng/slots.h>

#if MYNEWT_VAL(RTDOA_STATS)
STATS_NAME_START(rtdoa_stat_section)
    STATS_NAME(rtdoa_stat_section, rtdoa_request)
    STATS_NAME(rtdoa_stat_section, rtdoa_response)
    STATS_NAME(rtdoa_stat_section, rtdoa_listen)
    STATS_NAME(rtdoa_stat_section, rx_complete)
    STATS_NAME(rtdoa_stat_section, rx_error)
    STATS_NAME(rtdoa_stat_section, rx_timeout)
    STATS_NAME(rtdoa_stat_section, rx_relayed)
    STATS_NAME(rtdoa_stat_section, tx_relay_error)
    STATS_NAME(rtdoa_stat_section, tx_relay_ok)
    STATS_NAME(rtdoa_stat_section, start_rx_error)
    STATS_NAME(rtdoa_stat_section, rx_unsolicited)
    STATS_NAME(rtdoa_stat_section, start_tx_error)
    STATS_NAME(rtdoa_stat_section, reset)
STATS_NAME_END(rtdoa_stat_section)
#endif

struct rtdoa_instance *
rtdoa_init(struct uwb_dev * inst, struct uwb_rng_config * config, uint16_t nframes)
{
    assert(inst);

    struct rtdoa_instance * rtdoa = (struct rtdoa_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_RTDOA);
    if (rtdoa == NULL ) {
        rtdoa = (struct rtdoa_instance*) malloc(sizeof(struct rtdoa_instance) + nframes * sizeof(rtdoa_frame_t * ));
        assert(rtdoa);
        memset(rtdoa, 0, sizeof(struct rtdoa_instance));
        rtdoa->status.selfmalloc = 1;
    }
    os_error_t err = os_sem_init(&rtdoa->sem, 0x1);
    assert(err == OS_OK);

    rtdoa->dev_inst = inst;
    rtdoa->nframes = nframes;
    rtdoa->idx = 0xFFFF;
    rtdoa->seq_num = 0;

    if (config != NULL ){
        rtdoa_config(rtdoa, config);
    }

#if MYNEWT_VAL(RTDOA_STATS)
    int rc = stats_init(
                    STATS_HDR(rtdoa->stat),
                    STATS_SIZE_INIT_PARMS(rtdoa->stat, STATS_SIZE_32),
                    STATS_NAME_INIT_PARMS(rtdoa_stat_section)
            );

#if  MYNEWT_VAL(UWB_DEVICE_0) && !MYNEWT_VAL(UWB_DEVICE_1)
    rc |= stats_register("rtdoa", STATS_HDR(rtdoa->stat));
#elif  MYNEWT_VAL(UWB_DEVICE_0) && MYNEWT_VAL(UWB_DEVICE_1)
    if (inst == hal_dw1000_inst(0)) {
        rc |= stats_register("rtdoa0", STATS_HDR(rtdoa->stat));
    } else{
        rc |= stats_register("rtdoa1", STATS_HDR(rtdoa->stat));
    }
#endif
    assert(rc == 0);
#endif
    return rtdoa;
}

/**
 * API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
rtdoa_free(struct rtdoa_instance * inst)
{
    assert(inst);
    uwb_mac_remove_interface(inst->dev_inst, inst->cbs.id);

    if (inst->status.selfmalloc){
        for(int i =0; i< inst->nframes; i++){
            free(inst->frames[i]);
            inst->frames[i] = NULL;
        }
        free(inst);
    }
    else
        inst->status.initialized = 0;
}

void
rtdoa_set_frames(struct rtdoa_instance *rtdoa, uint16_t nframes)
{
    assert(nframes <= rtdoa->nframes);
    rtdoa_frame_t default_frame = {
        .PANID = 0xDECA,
        .fctrl = FCNTL_IEEE_RANGE_16,
        .code = UWB_DATA_CODE_RTDOA_INVALID
    };
    for (uint16_t i = 0; i < nframes; i++){
        rtdoa->frames[i] = (rtdoa_frame_t * ) malloc(sizeof(rtdoa_frame_t));
        assert(rtdoa->frames[i]);
        memcpy(rtdoa->frames[i], &default_frame, sizeof(rtdoa_frame_t));
    }
}


/**
 * API to assign the config parameters
 *
 * @param inst    struct rtdoa_instance
 * @param config  Pointer to struct uwb_rng_config.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
rtdoa_config(struct rtdoa_instance *rtdoa, struct uwb_rng_config * config){
    assert(config);
    memcpy(&rtdoa->config, config, sizeof(struct uwb_rng_config));
    return rtdoa->dev_inst->status;
}


/**
 * Help function to calculate the delay between cascading requests
 *
 * @param inst Pointer to struct uwb_dev
 * @param slot_id 0 for master, and increasing
 * @return void
 */
uint32_t
rtdoa_usecs_to_response(struct uwb_dev * inst, rtdoa_request_frame_t * req,
                        uint16_t nslots, struct uwb_rng_config * config, uint32_t duration)
{
    uint32_t ret = 0;
    /* Repeat part */
    ret += (req->rpt_max - req->rpt_count+1)*config->tx_holdoff_delay;

    /* Response part */
    ret += nslots * ( duration + (uint32_t)config->tx_guard_delay );
    return ret;
}

/**
 * Function for transforming local rtdoa timestamps into the reference frame timestamp
 * domain.
 *
 * @param inst Pointer to struct uwb_dev
 * @param slot_id 0 for master, and increasing
 * @return void
 */
uint64_t
rtdoa_local_to_master64(struct uwb_dev * inst, uint64_t dtu_time, rtdoa_frame_t *req_frame)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance * wcs = ccp->wcs;

    double delta = ((dtu_time & 0x0FFFFFFFFFFUL) - (req_frame->rx_timestamp&0x0FFFFFFFFFFUL)) & 0x0FFFFFFFFFFUL;
    uint64_t req_lo40 = (req_frame->tx_timestamp & 0x0FFFFFFFFFFUL);
    if (wcs->status.valid) {
        /* No need to take special care of 40bit overflow as the timescale forward returns
         * a double value that can exceed the 40bit. */
        req_lo40 += (uint64_t) round((1.0l - wcs->fractional_skew) * delta);
    } else {
        req_lo40 += delta;
    }
    return (req_frame->tx_timestamp & 0xFFFFFF0000000000UL) + req_lo40;
}

/**
 * API to listen as a slave node
 *
 * @param inst          Pointer to struct uwb_dev.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
rtdoa_listen(struct rtdoa_instance * rtdoa, uwb_dev_modes_t mode,
             uint64_t delay, uint16_t timeout)
{
    os_error_t err = os_sem_pend(&rtdoa->sem,  OS_TIMEOUT_NEVER);
    assert(err == OS_OK);

    /* Setup start time and overall timeout */
    uwb_set_delay_start(rtdoa->dev_inst, delay);
    uwb_set_rx_timeout(rtdoa->dev_inst, timeout);
    rtdoa->timeout = (delay + (((uint64_t)timeout)<<16))&0xFFFFFFFFFFUL;

    RTDOA_STATS_INC(rtdoa_listen);
    if(uwb_start_rx(rtdoa->dev_inst).start_rx_error){
        err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
        RTDOA_STATS_INC(start_rx_error);
    }
    if (mode == UWB_BLOCKING){
        err = os_sem_pend(&rtdoa->sem, OS_TIMEOUT_NEVER); // Wait for completion of transactions
        assert(err == OS_OK);
        err = os_sem_release(&rtdoa->sem);
        assert(err == OS_OK);
    }
    return rtdoa->dev_inst->status;
}


float
rtdoa_tdoa_between_frames(struct rtdoa_instance *rtdoa,
                          rtdoa_frame_t *req_frame, rtdoa_frame_t *resp_frame)
{
    int64_t tof;
    float diff_m = nanf("");
    if (req_frame == NULL) {
        return diff_m;
    }
    switch(resp_frame->code){
        case UWB_DATA_CODE_RTDOA_RESP: {
            /* Invalidate this frame to avoid it being used more than once */
            resp_frame->code = UWB_DATA_CODE_TWR_INVALID;

            if (resp_frame->tx_timestamp < req_frame->tx_timestamp) {
                break;
            }
            /* rxts stored in frames as local timestamp, recalc into reference frame domain */
            uint64_t rx_ts = rtdoa_local_to_master64(rtdoa->dev_inst, resp_frame->rx_timestamp, rtdoa->req_frame);
            tof = (int64_t)rx_ts - (int64_t)resp_frame->tx_timestamp;
            diff_m = uwb_rng_tof_to_meters(tof);

            //printf("r[%x-%x]: %llx %llx %16llx %llx\n", req_frame->src_address, resp_frame->src_address,
            //       rx_ts, resp_frame->tx_timestamp, tof, req_frame->tx_timestamp);
            //printf("r[%x-%x]: %6lld %6ldmm\n", req_frame->src_address, resp_frame->src_address, tof, (int32_t)(diff_m*1000.0f + 0.5f));
            break;
        }
        default: break;
    }
    return diff_m;
}
