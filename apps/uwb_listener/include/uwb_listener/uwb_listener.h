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
 * @file uwb_listener.h
 * @athor Niklas Casaril
 * @date 2020
 * @brief Listener
 *
 * @details Listener / Sniffer module
 *
 */

#ifndef __UWB_LISTENER_H_
#define __UWB_LISTENER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>

//! Listener status parameters
typedef struct _uwb_rng_status_t{
    uint16_t selfmalloc:1;           //!< Internal flag for memory garbage collection
    uint16_t initialized:1;          //!< Instance allocated
}uwb_listener_status_t;

//! Structure of listener instance
struct uwb_listener_instance {
    struct uwb_dev * dev_inst;       //!< Structure of uwb_dev
    uwb_listener_status_t status;
};

struct uwb_listener_instance * uwb_listener_init(struct uwb_dev * dev);
void uwb_listener_free(struct uwb_listener_instance * rng);
void listener_encode(struct uwb_dev * inst);

#ifdef __cplusplus
}
#endif

#endif /* _UWB_LISTENER_H_ */
