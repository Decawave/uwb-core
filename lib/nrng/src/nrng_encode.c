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
#include <uwb_rng/uwb_rng.h>
#include <json/json.h>
#include <nrng/nrng_encode.h>
#include <nrng/nrng_json.h>

#if MYNEWT_VAL(NRNG_VERBOSE)

void
nrng_encode(struct nrng_instance * nrng, uint8_t seq_num, uint16_t base){

    uint32_t valid_mask = 0;
    nrng_frame_t * frame = nrng->frames[(base)%nrng->nframes];

    nrng_json_t json = {
        .utime =  os_cputime_ticks_to_usecs(os_cputime_get32()),
        .seq = seq_num,
        .uid = frame->src_address
    };
    // Workout which slots responded with a valid frames
    for (uint16_t i=0; i < 16; i++){
        if (nrng->slot_mask & 1UL << i){
            uint16_t idx = BitIndex(nrng->slot_mask, 1UL << i, SLOT_POSITION);
            nrng_frame_t * frame = nrng->frames[(base + idx)%nrng->nframes];
            if (frame->code == UWB_DATA_CODE_SS_TWR_NRNG_FINAL && frame->seq_num == seq_num){
                valid_mask |= 1UL << i;
            }
        }
    }
    // tdoa results are reference to slot 0, so reject it slot 0 did not respond. An alternative approach is needed @Niklas
    if (valid_mask == 0 || (valid_mask & 1) == 0)
       return;

    uint16_t j=0;
    for (uint16_t i=0; i < 16; i++){
        if (valid_mask & 1UL << i){
            uint16_t idx = BitIndex(nrng->slot_mask, 1UL << i, SLOT_POSITION);
            nrng_frame_t * frame = nrng->frames[(base + idx)%nrng->nframes];
            if (frame->code == UWB_DATA_CODE_SS_TWR_NRNG_FINAL && frame->seq_num == seq_num){
                json.rng[j] = (dpl_float64_t) uwb_rng_tof_to_meters(nrng_twr_to_tof_frames(nrng->dev_inst, frame, frame));
                json.ouid[j++] = frame->dst_address;
            }
        }
    }
    json.nsize = j;

    nrng_json_write(&json);
    printf("%s\n",json.iobuf);
}

#endif
