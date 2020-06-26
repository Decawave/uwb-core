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
 * @file uwb_math.h
 * @athor Szymon Czapracki
 * @date 2020
 * @brief Ranging math functions
 *
 */

#ifndef __UWB_RNGMATH_H_
#define __UWB_RNGMATH_H_

#include <stdlib.h>
#include <stdint.h>
#include "dpl/dpl.h"
#include "syscfg/syscfg.h"
#include "euclid/triad.h"

#ifdef __cplusplus
extern "C" {
#endif

dpl_float64_t uwb_rng_tof_to_meters(dpl_float64_t ToF);
dpl_float64_t calc_tof_ss(uint32_t response_timestamp,
                                uint32_t request_timestamp,
                                uint64_t transmission_timestamp,
                                uint64_t reception_timestamp,
                                dpl_float64_t skew);
dpl_float64_t calc_tof_ds(uint32_t first_response_timestamp,
                                uint32_t first_request_timestamp,
                                uint64_t first_transmission_timestamp,
                                uint64_t first_reception_timestamp,
                                uint32_t response_timestamp,
                                uint32_t request_timestamp,
                                uint64_t transmission_timestamp,
                                uint64_t reception_timestamp);
uint32_t calc_tof_sym_ss(uint32_t response_timestamp,
                                uint32_t request_timestamp,
                                uint64_t transmission_timestamp,
                                uint64_t reception_timestamp);
uint32_t calc_tof_sym_ds(uint32_t first_response_timestamp,
                                uint32_t first_request_timestamp,
                                uint64_t first_transmission_timestamp,
                                uint64_t first_reception_timestamp,
                                uint32_t response_timestamp,
                                uint32_t request_timestamp,
                                uint64_t transmission_timestamp,
                                uint64_t reception_timestamp);
float uwb_rng_path_loss(float Pt, float G, float fc, float R);

#ifdef __cplusplus

}
#endif

#endif /* _UWB_MATH_H_ */
