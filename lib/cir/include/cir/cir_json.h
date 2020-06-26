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
 * @file cir_json.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date  May 5 2020
 * @brief
 *
 */

#ifndef _CIR_JSON_H_
#define _CIR_JSON_H_

#include <json/json.h>
#include <json/json_util.h>
#include <dpl/dpl.h>
#include <dpl/dpl_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cir_json{
    /* json_decoder must be first element in the structure */
    struct json_decoder decoder;
    struct json_encoder encoder;
    json_status_t status;
    char* type;
    uint64_t utime;
    uint64_t raw_ts;
    uint64_t resampler_delay;
    dpl_float64_t fp_power;
    dpl_float64_t angle;
    dpl_float64_t fp_idx;
    uint64_t accumulator_count;
    /* */
    uint64_t cir_offset;
    uint64_t cir_count;
    int64_t real[MYNEWT_VAL(CIR_MAX_SIZE)];
    int64_t imag[MYNEWT_VAL(CIR_MAX_SIZE)];
    /* */
    char iobuf[MYNEWT_VAL(CIR_VERBOSE_BUFFER_SIZE)];
    uint16_t idx;
} cir_json_t;

cir_json_t * cir_json_init(struct cir_json * json);
void cir_json_free(struct cir_json * json);
int cir_json_write(struct cir_json * json);

#ifdef __KERNEL__
int cir_chrdev_output(char *buf, size_t len);
#endif  /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif
