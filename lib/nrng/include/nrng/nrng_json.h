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
 * @file nrng_json.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date  Apr 10 2020
 * @brief
 *
 */

#ifndef _NRNG_JSON_H_
#define _NRNG_JSON_H_

#include <json/json.h>
#include <json/json_util.h>
#include <dpl/dpl_types.h>
#include <uwb/uwb.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nrng_json{
    /* json_decoder must be first element in the structure */
    struct json_decoder decoder;
    struct json_encoder encoder;
    json_status_t status;
    uint64_t utime;
    uint64_t seq;
    uint64_t uid;
    uint64_t ouid[MYNEWT_VAL(NRNG_NNODES)];
    dpl_float64_t rng[MYNEWT_VAL(NRNG_NNODES)];
    uint64_t nsize;
    char iobuf[PAGE_SIZE];
    uint16_t idx;
}nrng_json_t;

int nrng_json_read(nrng_json_t * json, char * line);
int nrng_json_write(nrng_json_t * json);

#ifdef __cplusplus
}
#endif
#endif
