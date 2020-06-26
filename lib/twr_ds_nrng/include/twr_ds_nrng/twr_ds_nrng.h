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
 * @file twr_ds.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Range
 *
 * @details This is the rng base class which utilises the functions to enable/disable the configurations related to rng.
 *
 */

#ifndef _DW1000_TWR_DS_NRNG_H_
#define _DW1000_TWR_DS_NRNG_H_

#if MYNEWT_VAL(TWR_DS_NRNG_ENABLED)

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dw1000/dw1000_dev.h>
#include <uwb/uwb_ftypes.h>
#include <nranges/nranges.h>
void twr_ds_nrng_pkg_init(void);
void twr_ds_nrng_free(struct uwb_dev * inst);

#ifdef __cplusplus
}
#endif

#endif // TWR_DS_ENABLED
#endif //_DW1000_TWR_DS_H_
