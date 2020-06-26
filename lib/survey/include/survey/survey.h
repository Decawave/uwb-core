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
 * @file survey.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2019
 *
 * @brief automatic site survey
 * @details The site survey process involves constructing a matrix of (n * n -1) ranges between n node.
 * For this, we designate a slot in the superframe that performs a nrng_requst to all other nodes.
 * We use the ccp->seq number to determine what node make use of this slot.
 *
 */

#ifndef _SURVEY_H_
#define _SURVEY_H_

#include <stdlib.h>
#include <stdint.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <uwb_rng/slots.h>
#include <stats/stats.h>

typedef struct survey_nrng{
    uint16_t mask;              //!< slot bitmask, the bit position in the mask decodes as the node slot_id
    dpl_float32_t rng[MYNEWT_VAL(SURVEY_NNODES)];      //!< Ranging corresponding to above slot mask. When broadcasting the survey results we one transmit sucessful range requests.
    uint16_t uid[MYNEWT_VAL(SURVEY_NNODES)];
}survey_nrng_t;

typedef struct survey_nrngs{
    uint16_t mask;              //!< slot bitmask, the bit position in the mask decodes as the node slot_id
    survey_nrng_t nrng[MYNEWT_VAL(SURVEY_NNODES)];     //!< Ranging corresponding to above slot mask. When broadcasting the survey results we one transmit sucessful range requests.
}survey_nrngs_t;

//! N-Ranges request frame
typedef union {
    struct _survey_broadcast_frame_t{
        struct _ieee_rng_request_frame_t;
        uint16_t slot_id;
        uint16_t cell_id;
        struct survey_nrng;
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _survey_broadcast_frame_t)];
}survey_broadcast_frame_t;

STATS_SECT_START(survey_stat_section)
    STATS_SECT_ENTRY(request)
    STATS_SECT_ENTRY(listen)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(start_tx_error)
    STATS_SECT_ENTRY(start_rx_error)
    STATS_SECT_ENTRY(broadcaster)
    STATS_SECT_ENTRY(receiver)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(reset)
STATS_SECT_END

//! Status parameters of ccp.
typedef struct _survey_status_t{
    uint16_t selfmalloc:1;            //!< Internal flag for memory garbage collection
    uint16_t initialized:1;           //!< Instance allocated
    uint16_t valid:1;                 //!< Set for valid parameters
    uint16_t start_rx_error:1;
    uint16_t start_tx_error:1;
    uint16_t empty:1;
    uint16_t update:1;
}survey_status_t;

//! config parameters.
typedef struct _survey_config_t{
    uint32_t rx_timeout_delay;          //!< Relay nodes holdoff
}survey_config_t;

//! survey instance parameters.
typedef struct _survey_instance_t{
    struct uwb_dev * dev_inst;                  //!< Pointer to struct uwb_dev
    struct uwb_ccp_instance * ccp;              //!< Pointer to uwb_ccp_instance
    struct nrng_instance * nrng;                //!< Pointer to _struct nrng_instance
    STATS_SECT_DECL(survey_stat_section) stat;  //!< Stats instance
    struct uwb_mac_interface cbs;               //!< MAC Layer Callbacks
    void (* survey_complete_cb) (struct dpl_event *ev); //!< Optional Callback for post processing
    struct dpl_sem sem;                          //!< Structure containing os semaphores
    survey_status_t status;                     //!< Survey status parameters
    survey_config_t config;                     //!< Survey control parameters
    uint8_t seq_num;
    uint16_t nnodes;                            //!< Number of buffers defined to store the data
    survey_broadcast_frame_t * frame;           //!< Frame to broadcast results back between nodes
    uint16_t nframes;                           //!< nrngs[] is cicrular buffer of size nframes
    uint16_t idx;                               //!< idx is cicrular buffer of size nframes
    survey_nrngs_t * nrngs[];                   //!< Array containing survey results, indexed by slot_id
}survey_instance_t;


survey_instance_t * survey_init(struct uwb_dev * inst, uint16_t nnodes, uint16_t nframes);
void survey_free(survey_instance_t * inst);
void survey_slot_range_cb(struct dpl_event *ev);
void survey_slot_broadcast_cb(struct dpl_event *ev);
survey_status_t survey_receiver(survey_instance_t * survey, uint64_t dx_time);

#ifdef __cplusplus
}
#endif

#endif /* _SURVEY_H_ */
