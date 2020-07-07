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
 * @file uwb_pan.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Personal Area Network
 *
 * @details This is the pan base class which utilises the functions to allocate/deallocate the resources on pan_master,sets callbacks, enables  * blink_requests.
 *
 */

#ifndef _UWB_PAN_H_
#define _UWB_PAN_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <pan_utils/pan_utils.h>

//! Network Roles
typedef enum {
    NETWORK_ROLE_INVALID = 0x0,   /*!< Invalid role */
    NETWORK_ROLE_ANCHOR,          /*!< Network Anchor */
    NETWORK_ROLE_TAG              /*!< Network Tag */
} network_role_t;


//! Pan package codes.
typedef enum _uwb_pan_modes_t{
    DWT_PAN_INVALID = 0,             //!< Invalid Pan message
    DWT_PAN_REQ,                     //!< Pan request
    DWT_PAN_RESP,                    //!< Pan response
    DWT_PAN_RESET,                   //!< Pan reset, in case of master restart
}uwb_pan_code_t;

struct uwb_pan_instance * uwb_pan_init(struct uwb_dev * inst,  struct uwb_pan_config_t * config, uint16_t nframes);
void uwb_pan_free(struct uwb_pan_instance *pan);
void uwb_pan_set_postprocess(struct uwb_pan_instance *pan, dpl_event_fn * postprocess);
void uwb_pan_set_request_cb(struct uwb_pan_instance *pan, uwb_pan_request_cb_func_t callback);

void uwb_pan_start(struct uwb_pan_instance * pan, enum uwb_pan_role_t role, network_role_t network_role);
struct uwb_dev_status uwb_pan_listen(struct uwb_pan_instance * pan, uwb_dev_modes_t mode);
struct uwb_pan_status_t uwb_pan_blink(struct uwb_pan_instance * pan, uint16_t role, uwb_dev_modes_t mode, uint64_t delay);
struct uwb_pan_status_t uwb_pan_reset(struct uwb_pan_instance * pan, uint64_t delay);
uint32_t uwb_pan_lease_remaining(struct uwb_pan_instance * pan);

void uwb_pan_slot_timer_cb(struct dpl_event * ev);

#ifdef __cplusplus
}
#endif
#endif /* _UWB_PAN_H_ */
