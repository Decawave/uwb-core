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
 * @file rng_json.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date  Apr 10 2020
 * @brief
 *
 */

#ifndef _RNG_JSON_H_
#define _RNG_JSON_H_

#include <json/json.h>
#include <json/json_util.h>
#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <euclid/triad.h>

typedef struct rng_json{
    /* json_decoder must be first element in the structure */
    struct json_decoder decoder;
    struct json_encoder encoder;
    json_status_t status;
    uint64_t utime;
    uint64_t seq;
    uint64_t code;
    uint64_t uid;
    uint64_t ouid;
    triad_t raz;
    triad_t braz;
    union {
        dpl_float64_t rssi[3];
        uint64_t rssi_uint64[3];
    };
    uint64_t count;
    union {
        dpl_float64_t los[3];
        uint64_t los_uint64[3];
    };
    union {
        dpl_float64_t pd;
        uint64_t pd_uint64;
    };
    union {
        dpl_float64_t ppm;
        uint64_t ppm_uint64;
    };
    dpl_float64_t sts;
    char iobuf[MYNEWT_VAL(UWB_RNG_JSON_BUFSIZE)];
    uint16_t idx;
}rng_json_t;

rng_json_t * rng_json_init(rng_json_t * json);
void rng_json_free(rng_json_t * json);
#ifdef FLOAT_SUPPORT
int rng_json_read(rng_json_t * json, char * line);
#endif
int rng_json_write(rng_json_t * json);

#endif
