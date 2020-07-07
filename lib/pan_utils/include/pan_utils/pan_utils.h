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
 * @file pan_utils.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2020
 * @brief Personal Area Network utilities
 *
 */

#ifndef _PAN_UTILS_H_
#define _PAN_UTILS_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dpl/dpl.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <stats/stats.h>

STATS_SECT_START(pan_stat_section)
    STATS_SECT_ENTRY(pan_request)
    STATS_SECT_ENTRY(pan_listen)
    STATS_SECT_ENTRY(pan_reset)
    STATS_SECT_ENTRY(relay_tx)
    STATS_SECT_ENTRY(lease_expiry)
    STATS_SECT_ENTRY(tx_complete)
    STATS_SECT_ENTRY(rx_complete)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(rx_other_frame)
    STATS_SECT_ENTRY(rx_error)
    STATS_SECT_ENTRY(tx_error)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(reset)
STATS_SECT_END

extern STATS_SECT_DECL(pan_stat_section) g_stat; //!< Stats instance

enum uwb_pan_role_t {
    UWB_PAN_ROLE_INVALID = 0,                   //!< Pan master mode
    UWB_PAN_ROLE_MASTER,                        //!< Pan master mode
    UWB_PAN_ROLE_SLAVE,                         //!< Pan slave mode
    UWB_PAN_ROLE_RELAY                          //!< Pan relay mode
};

/* Copy of bootutil/image: image_version structure */
struct pan_image_version {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
};

struct pan_req_resp {
    uint16_t role;                       //!< Requested role in network
    uint16_t lease_time;                 //!< Requested lease time in seconds
    union {
        struct pan_image_version fw_ver; //!< Firmware version running
        struct {
            uint16_t pan_id;             //!< Assigned pan_id
            uint16_t short_address;      //!< Assigned device_id
            uint16_t slot_id;            //!< Assigned slot_id
            uint16_t padding;
        };
    };
};

typedef bool (*uwb_pan_request_cb_func_t)(uint64_t euid, struct pan_req_resp *request, struct pan_req_resp *response);

//! Union of response frame format
union pan_frame_t {
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
};

//! Pan status parameters
struct uwb_pan_status_t {
    uint16_t selfmalloc:1;                 //!< Internal flag for memory garbage collection
    uint16_t initialized:1;                //!< Instance allocated
    uint16_t valid:1;                      //!< Set for valid parameters
    uint16_t start_tx_error:1;             //!< Set for start transmit error
    uint16_t lease_expired:1;              //!< Set when lease has expired
};

//! Pan configure parameters
struct uwb_pan_config_t {
    uint32_t rx_holdoff_delay;        //!< Delay between frames, in UWB usec.
    uint16_t rx_timeout_period;       //!< Receive response timeout, in UWB usec.
    uint32_t tx_holdoff_delay:28;     //!< Delay between frames, in UWB usec.
    uint32_t role:4;                  //!< uwb_pan_role_t
    uint16_t lease_time;              //!< Lease time in seconds
    uint16_t network_role;            //!< Network application role (Anchor/Tag...)
};

//! Pan control parameters
struct uwb_pan_control_t {
    uint16_t postprocess:1;           //!< Pan postprocess
    uint16_t request_cb:1;            //!< Pan request callback
};

//! Pan instance parameters
struct uwb_pan_instance {
    struct uwb_dev * dev_inst;                   //!< pointer to struct uwb_dev
    struct uwb_mac_interface cbs;                //!< MAC Layer Callbacks
    struct dpl_sem sem;                          //!< Structure containing os semaphores
    struct uwb_pan_status_t status;              //!< Pan status parameters
    struct uwb_pan_control_t control;            //!< Pan control parameters
    uwb_pan_request_cb_func_t request_cb;        //!< Request handling callback
    struct dpl_event postprocess_event;          //!< Structure of postprocess event
    struct dpl_callout pan_lease_callout_expiry; //!< Structure of lease_callout_expiry
    struct uwb_pan_config_t * config;                   //!< Pan config parameters
    uint16_t nframes;                            //!< Number of buffers defined to store the data
    uint16_t idx;                                //!< Indicates number of DW1000 instances
    union pan_frame_t * frames[];                      //!< Buffers to pan frames
};

void uwb_pan_set_request_cb(struct uwb_pan_instance *pan,
                                        uwb_pan_request_cb_func_t callback);
void uwb_pan_set_postprocess(struct uwb_pan_instance *pan, dpl_event_fn * cb);

#ifdef __cplusplus
}
#endif
#endif /* _UWB_PAN_UTILS */
