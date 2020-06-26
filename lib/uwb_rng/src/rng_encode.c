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
#include <dpl/dpl_types.h>
#include <dpl/dpl_cputime.h>
#include <json/json.h>
#include <uwb_rng/uwb_rng.h>
#include <uwb_rng/rng_json.h>
#include <uwb_rng/rng_encode.h>

#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif

#ifdef __KERNEL__
int rng_encode_output(int idx, char *buf, size_t len);
#endif

#if MYNEWT_VAL(RNG_VERBOSE)
/*!
 * @fn rng_encode(struct uwb_rng_instance * rng)
 *
 * @brief JSON encoding of range
 * "{\"utime\": 0,\"seq\": 1,\"uid\": 0x1234,\"ouid\": 1234,\"raz\": [4.000000],\"rssi\": [81.000000,-81.1,1e-6],\"los\": [1.000000,null,null]}";
 * input parameters
 * @param rng     Pointer of struct uwb_rng_instance.
 * output parameters
 * returns void
 */
void
rng_encode(struct uwb_rng_instance * rng)
{
    int rc;

    twr_frame_t * frame = rng->frames[rng->idx_current];
    dpl_float64_t time_of_flight = uwb_rng_twr_to_tof(rng, rng->idx_current);
    frame->local.spherical.range = uwb_rng_tof_to_meters(time_of_flight);

    rng_json_t json = {
#if MYNEWT_VAL(UWB_WCS_ENABLED)
        .utime = uwb_wcs_read_systime_master64(rng->dev_inst),
#else
        .utime = dpl_cputime_ticks_to_usecs(dpl_cputime_get32()),
#endif
        .uid = frame->src_address,
        .ouid = frame->dst_address,
        .ppm = DPL_FLOAT64_NAN(),
        .sts = DPL_FLOAT64_NAN(),
        .idx =0
    };

    for (uint8_t i = 0;i< sizeof(json.raz)/sizeof(json.raz.array[0]);i++)
            json.raz.array[i] = json.braz.array[i] = json.los[i] = DPL_FLOAT64_NAN();

    switch(frame->code){
        case UWB_DATA_CODE_SS_TWR_EXT_FINAL:
        case UWB_DATA_CODE_DS_TWR_EXT_FINAL:
        for (uint8_t i = 0; i < sizeof(json.braz)/sizeof(json.braz.array[0]);i++){
            json.braz.array[i] = frame->remote.spherical.array[i];
        }
        /* Intentionally fall through */
        case UWB_DATA_CODE_SS_TWR_FINAL:
        case UWB_DATA_CODE_SS_TWR_ACK_FINAL:
        case UWB_DATA_CODE_DS_TWR_FINAL:
        for (uint8_t i = 0; i < sizeof(json.raz)/sizeof(json.raz.array[0]);i++){
            json.raz.array[i] = frame->local.spherical.array[i];
        }

        break;
        default: printf(",error: \"Unknown Frame Code\", %x\n", frame->code);
    }

#if MYNEWT_VAL(RNG_VERBOSE) > 1
    json.pd   = DPL_FLOAT64_FROM_F32(frame->local.pdoa);
    json.code = frame->code;
    if(rng->dev_inst->config.rxdiag_enable){
        for (uint8_t i=0;i< sizeof(json.rssi)/sizeof(json.rssi[0]);i++)
            json.rssi[i] = DPL_FLOAT64_FROM_F32(frame->local.vrssi[i]);
        dpl_float32_t fppl = frame->local.fppl;
        json.los[0] = DPL_FLOAT64_FROM_F32(uwb_estimate_los(rng->dev_inst, frame->local.rssi, fppl));
    }

    rc = rng_json_write(&json);
    assert(rc == 0);

#ifdef __KERNEL__
    /* This should probably not advance the pointer */
    size_t n = strlen(json.iobuf);
    json.iobuf[n]='\n';
    json.iobuf[n+1]='\0';
    rng_encode_output(rng->dev_inst->idx, json.iobuf, strlen(json.iobuf));
#else
    printf("%s\n",json.iobuf);
#endif

}
#endif

#endif
