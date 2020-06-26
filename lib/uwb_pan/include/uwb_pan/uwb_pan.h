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
#include "uwb_pan_req.h"

//! Roles available for PAN
typedef enum _uwb_pan_role_t{
    UWB_PAN_ROLE_INVALID = 0,                   //!< Pan master mode
    UWB_PAN_ROLE_MASTER,                        //!< Pan master mode
    UWB_PAN_ROLE_SLAVE,                         //!< Pan slave mode
    UWB_PAN_ROLE_RELAY                          //!< Pan relay mode
}uwb_pan_role_t;

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

//! Union of response frame format
typedef union{
//! Structure containing pan response frame format
    struct _pan_frame_t{
        //! Structure of IEEE blink frame
        struct _ieee_blink_frame_t;
        uint8_t rpt_count:4;                 //!< Repeat level
        uint8_t rpt_max:4;                   //!< Repeat max level
        uint16_t code;                       //!< Package type code
        struct pan_req_resp req;
    }__attribute__((__packed__, aligned(1)));
    uint8_t array[sizeof(struct _pan_frame_t)];
}pan_frame_t;

//! Pan status parameters
typedef struct _uwb_pan_status_t{
    uint16_t selfmalloc:1;                 //!< Internal flag for memory garbage collection
    uint16_t initialized:1;                //!< Instance allocated
    uint16_t valid:1;                      //!< Set for valid parameters
    uint16_t start_tx_error:1;             //!< Set for start transmit error
    uint16_t lease_expired:1;              //!< Set when lease has expired
}uwb_pan_status_t;

//! Pan configure parameters
typedef struct _uwb_pan_config_t{
    uint32_t rx_holdoff_delay;        //!< Delay between frames, in UWB usec.
    uint16_t rx_timeout_period;       //!< Receive response timeout, in UWB usec.
    uint32_t tx_holdoff_delay:28;     //!< Delay between frames, in UWB usec.
    uint32_t role:4;                  //!< uwb_pan_role_t
    uint16_t lease_time;              //!< Lease time in seconds
    uint16_t network_role;            //!< Network application role (Anchor/Tag...)
}uwb_pan_config_t;

//! Pan control parameters
typedef struct _uwb_pan_control_t{
    uint16_t postprocess:1;           //!< Pan postprocess
    uint16_t request_cb:1;            //!< Pan request callback
}uwb_pan_control_t;

//! Pan instance parameters
struct uwb_pan_instance{
    struct uwb_dev * dev_inst;                   //!< pointer to struct uwb_dev
    struct uwb_mac_interface cbs;                //!< MAC Layer Callbacks
    struct dpl_sem sem;                          //!< Structure containing os semaphores
    uwb_pan_status_t status;                     //!< Pan status parameters
    uwb_pan_control_t control;                   //!< Pan control parameters
    uwb_pan_request_cb_func_t request_cb;        //!< Request handling callback
    struct dpl_event postprocess_event;          //!< Structure of postprocess event
    struct dpl_callout pan_lease_callout_expiry; //!< Structure of lease_callout_expiry
    uwb_pan_config_t * config;                   //!< Pan config parameters
    uint16_t nframes;                            //!< Number of buffers defined to store the data
    uint16_t idx;                                //!< Indicates number of DW1000 instances
    pan_frame_t * frames[];                      //!< Buffers to pan frames
};

struct uwb_pan_instance * uwb_pan_init(struct uwb_dev * inst,  uwb_pan_config_t * config, uint16_t nframes);
void uwb_pan_free(struct uwb_pan_instance *pan);
void uwb_pan_set_postprocess(struct uwb_pan_instance *pan, dpl_event_fn * postprocess);
void uwb_pan_set_request_cb(struct uwb_pan_instance *pan, uwb_pan_request_cb_func_t callback);

void uwb_pan_start(struct uwb_pan_instance * pan, uwb_pan_role_t role, network_role_t network_role);
struct uwb_dev_status uwb_pan_listen(struct uwb_pan_instance * pan, uwb_dev_modes_t mode);
uwb_pan_status_t uwb_pan_blink(struct uwb_pan_instance * pan, uint16_t role, uwb_dev_modes_t mode, uint64_t delay);
uwb_pan_status_t uwb_pan_reset(struct uwb_pan_instance * pan, uint64_t delay);
uint32_t uwb_pan_lease_remaining(struct uwb_pan_instance * pan);

void uwb_pan_slot_timer_cb(struct dpl_event * ev);

#ifdef __cplusplus
}
#endif
#endif /* _UWB_PAN_H_ */
