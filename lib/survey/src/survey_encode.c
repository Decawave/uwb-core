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
#include <nrng/nrng_json.h>
#include <survey/survey_encode.h>

#if MYNEWT_VAL(SURVEY_VERBOSE)

/**
 * API for verbose JSON logging of survey resultss
 *
 * @param survey survey_instance_t point
 * @param seq_num survey
 * @return none.
 */
void
survey_encode(survey_instance_t * survey, uint16_t seq, uint16_t idx){

    uint32_t utime = os_cputime_ticks_to_usecs(os_cputime_get32());
    survey_nrngs_t * nrngs = survey->nrngs[idx%survey->nframes];

    uint16_t mask = 0;
    // Workout which node responded to the request
    for (uint16_t i=0; i < survey->nnodes; i++){
        if (nrngs->nrng[i].mask){
                mask |= 1UL << i;
        }
    }

    survey->status.empty = NumberOfBits(mask) == 0;
    if (survey->status.empty)
       return;

    for (uint16_t i=0; i < survey->nnodes; i++){
        if (nrngs->nrng[i].mask){
            nrng_json_t json={
                .utime = utime,
                .seq = seq,
                .nsize = NumberOfBits(nrngs->nrng[i].mask)
                };
            for (uint16_t j=0; j < json.nsize; j++){
                json.rng[j] = nrngs->nrng[i].rng[j];
                json.ouid[j] = nrngs->nrng[i].uid[j];
            }
            nrng_json_write(&json);
            printf("%s\n", json.iobuf);
        }
    }
}

#endif
