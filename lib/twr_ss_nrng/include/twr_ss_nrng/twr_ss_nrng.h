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
 * @file twr_ss_nrng.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Range
 *
 * @details This is the rng base class which utilises the functions to enable/disable the configurations related to rng.
 *
 */

#ifndef _TWR_SS_NRNG_H_
#define _TWR_SS_NRNG_H_

#if MYNEWT_VAL(TWR_SS_NRNG_ENABLED)

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <nrng/nrng.h>

void twr_ss_nrng_pkg_init(void);
void twr_ss_nrng_free(struct uwb_dev * inst);

#ifdef __cplusplus
}
#endif

#endif // TWR_SS_NRNG_ENABLED
#endif //_TWR_SS_NRNG_H_
