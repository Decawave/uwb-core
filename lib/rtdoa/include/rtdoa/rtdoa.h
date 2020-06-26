/**
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

#ifndef _RTDOA_H_
#define _RTDOA_H_

#include <stdlib.h>
#include <stdint.h>

#include <hal/hal_spi.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <euclid/triad.h>
#include <stats/stats.h>

#if MYNEWT_VAL(UWB_RNG_ENABLED)
#include <uwb_rng/uwb_rng.h>
#include <uwb_rng/slots.h>
#endif

#if MYNEWT_VAL(RTDOA_STATS)
    STATS_SECT_START(rtdoa_stat_section)
    STATS_SECT_ENTRY(rtdoa_request)
    STATS_SECT_ENTRY(rtdoa_response)
    STATS_SECT_ENTRY(rtdoa_listen)
    STATS_SECT_ENTRY(rx_complete)
    STATS_SECT_ENTRY(rx_error)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(rx_relayed)
    STATS_SECT_ENTRY(tx_relay_error)
    STATS_SECT_ENTRY(tx_relay_ok)
    STATS_SECT_ENTRY(start_rx_error)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(start_tx_error)
    STATS_SECT_ENTRY(reset)
STATS_SECT_END

#define RTDOA_STATS_INC(__X) STATS_INC(rtdoa->stat, __X)
#else
#define RTDOA_STATS_INC(__X) {}
#endif

//! N-Ranges request frame
typedef union {
    struct _rtdoa_request_frame_t{
        struct _ieee_rng_request_frame_t;
        uint64_t tx_timestamp;
        uint8_t slot_modulus;       //!< How many slots to send in
        uint8_t rpt_count;          //!< Repeat level
        uint8_t rpt_max;            //!< Repeat max level
        // Add Location and variance of location here, triad?
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _rtdoa_request_frame_t)];
} rtdoa_request_frame_t;

//! RTDoA response frame
typedef union {
    struct _rtdoa_response_frame_t{
       struct _ieee_rng_request_frame_t;
        uint64_t tx_timestamp;
        uint8_t slot_id;          //!< slot_idx of transmitting anchor
        // Anchor Location??
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _rtdoa_response_frame_t)]; //!< Array of size nrng response frame
} rtdoa_response_frame_t;

//! RTDoA ext response frame format
typedef union {
    struct _rtdoa_frame_t{
        struct _rtdoa_request_frame_t;
        uint64_t rx_timestamp;
        union {
            struct uwb_dev_rxdiag diag;
            uint8_t diag_storage[MYNEWT_VAL(UWB_DEV_RXDIAG_MAXLEN)];
        };
    } __attribute__((__packed__, aligned(1)));
    uint8_t array[sizeof(struct _rtdoa_frame_t)];
} rtdoa_frame_t;


struct rtdoa_instance{
    struct uwb_dev * dev_inst;                  //!< Pointer to struct uwb_dev
    struct uwb_ccp_instance * ccp;
#if MYNEWT_VAL(RTDOA_STATS)
    STATS_SECT_DECL(rtdoa_stat_section) stat;   //!< Stats instance
#endif
    uint16_t resp_count;
    uint64_t delay;
    uint64_t timeout;
    uint8_t seq_num;
    uint16_t nframes;
    struct os_sem sem;                          //!< Structure of semaphores
    struct uwb_mac_interface cbs;               //!< MAC Layer Callbacks
    uwb_rng_status_t status;
    uwb_rng_control_t control;
    struct uwb_rng_config config;
    uint16_t idx;
    rtdoa_frame_t * req_frame;
    rtdoa_frame_t * frames[];
};

#ifdef __cplusplus
extern "C" {
#endif

struct rtdoa_instance * rtdoa_init(struct uwb_dev * inst, struct uwb_rng_config * config, uint16_t nframes);
float rtdoa_tdoa_between_frames(struct rtdoa_instance *rtdoa, rtdoa_frame_t *req_frame, rtdoa_frame_t *resp_frame);

void rtdoa_set_frames(struct rtdoa_instance *rtdoa, uint16_t nframes);
struct uwb_dev_status rtdoa_config(struct rtdoa_instance *rtdoa, struct uwb_rng_config * config);

struct uwb_dev_status rtdoa_listen(struct rtdoa_instance * rtdoa, uwb_dev_modes_t mode, uint64_t delay, uint16_t timeout);
uint32_t rtdoa_usecs_to_response(struct uwb_dev * inst, rtdoa_request_frame_t * req,
                                 uint16_t nslots, struct uwb_rng_config * config, uint32_t duration);
uint64_t rtdoa_local_to_master64(struct uwb_dev * inst, uint64_t dtu_time, rtdoa_frame_t *req_frame);

#ifdef __cplusplus
}
#endif
#endif /* _RTDOA_H_ */
