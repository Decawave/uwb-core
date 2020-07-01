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

#ifndef _UWB_RNG_JSON_TEST_H
#define _UWB_RNG_JSON_TEST_H

#include <stdio.h>

#include "os/mynewt.h"
#include "testutil/testutil.h"
#include "uwb_rng/rng_encode.h"
#include "uwb_rng/rng_json.h"

#define JSON_BIGBUF_SIZE    192
extern char *bigbuf;
extern int buf_index;

bool compare_json_rng_structs(rng_json_t *json_exp, rng_json_t *json_in);
bool compare_los(dpl_float64_t *los_exp, dpl_float64_t *rssi_exp);
bool compare_rssi(dpl_float64_t *rssi_exp, dpl_float64_t *rssi_in);
bool compare_raz(triad_t *raz_exp, triad_t *raz_in);
bool epsilon_same_float(float a, float b);
bool epsilon_same_double(double a, double b);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif /* _UWB_RNG_JSON_TEST_H */
