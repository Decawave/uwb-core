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
 * @file dw1000_rng.h
 * @athor paul kettle
 * @date 2018
 * @brief Range
 *
 * @details This is the rng base class which utilises the functions to enable/disable the configurations related to rng.
 *
 */

#ifndef _DW1000_NMGR_CMDS_H_
#define _DW1000_NMGR_CMDS_H_


#include <stdlib.h>
#include <stdint.h>
#include <mgmt/mgmt.h>
#include <nmgr_uwb/nmgr_uwb.h>

typedef union{
    struct _nmgr_uwb_frame_t{
        struct _nmgr_uwb_header;
        union _payload{
            struct nmgr_hdr hdr;
            uint8_t payload[NMGR_UWB_MTU_EXT];
        };
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _nmgr_uwb_frame_t)];
}nmgr_uwb_frame_t;

#ifdef __cplusplus
extern "C" {
#endif
struct os_eventq* nmgr_cmds_get_eventq();

#ifdef __cplusplus
}
#endif

#endif /* _DW1000_RNG_H_ */
