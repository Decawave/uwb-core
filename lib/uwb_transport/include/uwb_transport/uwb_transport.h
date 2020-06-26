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
 * @file uwb_transport.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2019
 * @brief
 *
 * @details uwb transport layer
 *
 */

#ifndef _UWB_TRANSPORT_H_
#define _UWB_TRANSPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <dpl/dpl.h>
#include <dpl/dpl_mbuf.h>

#if MYNEWT_VAL(UWB_TRANSPORT_STATS)
#include <stats/stats.h>
STATS_SECT_START(uwb_transport_stat_section)
    STATS_SECT_ENTRY(tx_bytes)
    STATS_SECT_ENTRY(tx_packets)
    STATS_SECT_ENTRY(tx_err)
    STATS_SECT_ENTRY(rx_bytes)
    STATS_SECT_ENTRY(rx_packets)
    STATS_SECT_ENTRY(rx_acks)
    STATS_SECT_ENTRY(rx_err)
    STATS_SECT_ENTRY(rx_dup)
    STATS_SECT_ENTRY(rx_start_err)
    STATS_SECT_ENTRY(om_writes)
#if MYNEWT_VAL(UWB_TRANSPORT_STATS_BITRATE)
    STATS_SECT_ENTRY(tx_bitrate)
    STATS_SECT_ENTRY(rx_bitrate)
#endif
STATS_SECT_END
#endif

#define UWB_TRANSPORT_FCTRL UWB_FCTRL_STD_DATA_FRAME


typedef struct _uwb_transport_user_header
{
    uint16_t tsp_code;                                //!< Extension ID
    uint16_t uid;                                     //!< Source short unique identifier
    struct _uwb_transport_instance * uwb_transport;   //!< Pointer to instance owning this interface
}__attribute__((__packed__,aligned(1))) uwb_transport_user_header_t;

//! IEEE 802.15.4 standard data frame.
typedef union {
    struct _uwb_transport_frame{
        struct _ieee_std_frame_t;   //!< Standard IEEE data frame
        uint16_t tsp_code;          //!< Transport code ID
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _uwb_transport_frame)];  //!< Array of size standard frame
} uwb_transport_frame_header_t;

#define UWB_TRANSPORT_MTU_STD (128 - sizeof(uwb_transport_frame_header_t) - 2/*CRC*/)
#define UWB_TRANSPORT_MTU_EXT (512 - sizeof(uwb_transport_frame_header_t) - 2/*CRC*/)

typedef struct _uwb_transport_extension {
    uint16_t tsp_code;                                //!< Extension ID
    struct _uwb_transport_instance * uwb_transport;   //!< Pointer to instance owning this interface
    bool (* receive_cb) (struct uwb_dev * inst, uint16_t uid, struct dpl_mbuf * mbuf);    //!< Callback to handle incoming pkt
    bool (* transmit_cb) (struct uwb_dev * inst);    //!< Callback to signify successful tx, queue can handle more packets
    SLIST_ENTRY(_uwb_transport_extension) next;      //!< pointer to next entry in extension list
}uwb_transport_extension_t;

//! uwb_transport config parameters
typedef struct _uwb_transport_config_t{
    uint16_t os_msys_mpool:1;       //!< Using os_msys_mpool as default
    uint16_t dflt_eventq:1;         //!< Using default eventq
    uint16_t request_acks:1;        //!< Whether to enable acks
}uwb_transport_config_t;

//! uwb_transport config parameters
typedef struct _uwb_transport_status_t{
    uint16_t has_init:1;
    uint16_t awaiting_ack_tx:1;     //!< Awaiting ACK tx complete
}uwb_transport_status_t;

//! uwb_transport instance parameters
typedef struct _uwb_transport_instance {
    struct uwb_dev * dev_inst;
    uwb_transport_status_t status;
    uwb_transport_config_t config;
    uint8_t frame_seq_num;
    int ack_seq_num;

    uint32_t last_frame_time;
    uwb_transport_frame_header_t last_frame;
    uint16_t tx_buffer_len;
    uint16_t tx_buffer_offset;
    struct dpl_sem sem;
    struct dpl_sem write_tx_lock;
    struct dpl_sem ack_sem;
    struct dpl_mqueue tx_q;
    struct dpl_mqueue rx_q;
    struct dpl_mbuf_pool * omp;
    struct dpl_eventq * oeq;
    SLIST_HEAD(,_uwb_transport_extension) extension_list;
#if MYNEWT_VAL(UWB_TRANSPORT_STATS)
    STATS_SECT_DECL(uwb_transport_stat_section) stat;
#if MYNEWT_VAL(UWB_TRANSPORT_STATS_BITRATE)
    float tx_bits_per_second;
    uint32_t tx_br_last;
    float rx_bits_per_second;
    uint32_t rx_br_last;
#endif
#endif
    char device_name[12];
}uwb_transport_instance_t;

typedef enum _uwb_transport_codes_t{
    UWB_TRANSPORT_CMD_STATE_SEND = 1,
    UWB_TRANSPORT_CMD_STATE_RSP,
    UWB_TRANSPORT_CMD_STATE_INVALID
}uwb_transport_codes_t;

uint16_t uwb_transport_mtu(struct dpl_mbuf *m, int idx);
void uwb_transport_free(struct _uwb_transport_instance *uwb_transport);
uwb_transport_instance_t* uwb_transport_init(struct uwb_dev *inst);
struct uwb_dev_status uwb_transport_write_tx(struct _uwb_transport_instance *uwb_transport, struct dpl_mbuf *om, uint16_t idx);

struct uwb_dev_status uwb_transport_start_tx(struct _uwb_transport_instance *uwb_transport, struct dpl_mbuf *om, uint64_t dx_tim);
struct dpl_mbuf* uwb_transport_new_mbuf(struct _uwb_transport_instance *uwb_transport);

/* Extension interface */
struct _uwb_transport_extension * uwb_transport_get_extension(struct _uwb_transport_instance *uwb_transport, uint16_t tsp_code);
void uwb_transport_append_extension(struct _uwb_transport_instance *uwb_transport, struct _uwb_transport_extension  *extension);
void uwb_transport_remove_extension(struct _uwb_transport_instance *uwb_transport, uint16_t tsp_code);

/* Sychronous model */
struct uwb_dev_status uwb_transport_listen(struct _uwb_transport_instance *uwb_transport, uwb_dev_modes_t mode, uint64_t dx_time, uint64_t timeout);
int uwb_transport_dequeue_tx(struct _uwb_transport_instance *uwb_transport, uint64_t dx_time, uint64_t timeout);
    int uwb_transport_enqueue_tx(struct _uwb_transport_instance *uwb_transport, uint16_t dst_addr, uint16_t tsp_code, uint16_t retries, struct dpl_mbuf *om);

#ifdef __cplusplus
}
#endif

#endif /* _UWB_TRANSPORT_H_ */
