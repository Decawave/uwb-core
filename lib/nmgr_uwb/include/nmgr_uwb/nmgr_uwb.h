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
 * @file nmgr_uwb.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2019
 * @brief
 *
 * @details Newtmgr over uwb
 *
 */

#ifndef _NMGR_UWB_H_
#define _NMGR_UWB_H_


#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <hal/hal_spi.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>

#define NMGR_UWB_MTU_STD (128 -  sizeof(struct _ieee_std_frame_t) - sizeof(uint16_t) - 2/*CRC*/)
#define NMGR_UWB_MTU_EXT (1023 - sizeof(struct _ieee_std_frame_t) - sizeof(uint16_t) - 2/*CRC*/)
#define NMGR_UWB_FCTRL UWB_FCTRL_STD_DATA_FRAME
#define NMGR_UWB_FCTRL_WACK (NMGR_UWB_FCTRL|UWB_FCTRL_ACK_REQUESTED)

//! IEEE 802.15.4 standard data frame.
typedef union {
//! Structure of standard frame
    struct _nmgr_uwb_header{
        uint16_t fctrl;             //!< Frame control
        uint8_t seq_num;            //!< Sequence number, incremented for each new frame
        uint16_t PANID;             //!< PANID
        uint16_t dst_address;       //!< Destination address
        uint16_t src_address;       //!< Source address
        uint16_t code;              //!< Response code for the request
        uint8_t  rpt_count;         //!< Repeat level
        uint8_t  rpt_max;           //!< Repeat max level
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _nmgr_uwb_header)];  //!< Array of size standard frame
} nmgr_uwb_frame_header_t;

typedef struct _nmgr_uwb_instance_t {
    struct uwb_dev* dev_inst;
    uint8_t frame_seq_num;
    struct dpl_sem sem;
    struct os_mqueue tx_q;
} nmgr_uwb_instance_t;

uint16_t nmgr_uwb_mtu(struct os_mbuf *m, int idx);
nmgr_uwb_instance_t* nmgr_uwb_init(struct uwb_dev* inst);
int nmgr_uwb_tx(struct _nmgr_uwb_instance_t *nmgruwb, uint16_t dst_addr, uint16_t code, struct os_mbuf *m, uint64_t dx_time);

/* Sychronous model */
struct uwb_dev_status nmgr_uwb_listen(struct _nmgr_uwb_instance_t *nmgruwb, uwb_dev_modes_t mode, uint64_t delay, uint16_t timeout);
int uwb_nmgr_process_tx_queue(struct _nmgr_uwb_instance_t *nmgruwb, uint64_t dx_time);
int uwb_nmgr_queue_tx(struct _nmgr_uwb_instance_t *nmgruwb, uint16_t dst_addr, uint16_t code, struct os_mbuf *om);
#ifdef __cplusplus
}
#endif

#endif /* _NMGR_UWB_H_ */
