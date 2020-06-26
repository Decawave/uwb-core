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
 * @details Api for handling pan requests
 *
 */

#ifndef _UWB_PAN_REQ_H_
#define _UWB_PAN_REQ_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>

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
} __attribute__((packed));

typedef bool (*uwb_pan_request_cb_func_t)(uint64_t euid, struct pan_req_resp *request, struct pan_req_resp *response);

#ifdef __cplusplus
}
#endif
#endif /* _UWB_PAN_REQ_H_ */
