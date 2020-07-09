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
 * @file uwb_rng.h
 * @athor paul kettle
 * @date 2018
 * @brief Range
 *
 * @details This is the rng base class which utilises the functions to enable/disable the configurations related to rng.
 *
 */

#ifndef __UWB_RNG_H_
#define __UWB_RNG_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <euclid/triad.h>
#include <stats/stats.h>
#include <uwb_rng/slots.h>
#include <euclid/triad.h>
#include <syscfg/syscfg.h>

#if MYNEWT_VAL(RNG_STATS)
STATS_SECT_START(rng_stat_section)
    STATS_SECT_ENTRY(rng_request)
    STATS_SECT_ENTRY(rng_listen)
    STATS_SECT_ENTRY(tx_complete)
    STATS_SECT_ENTRY(rx_complete)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(rx_other_frame)
    STATS_SECT_ENTRY(rx_error)
    STATS_SECT_ENTRY(tx_error)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(complete_cb)
    STATS_SECT_ENTRY(reset)
    STATS_SECT_ENTRY(superframe_reset)
STATS_SECT_END
#endif

//! Range configuration parameters.
struct uwb_rng_config{
   uint32_t rx_holdoff_delay;        //!< Delay between frames, in UWB usec.
   uint32_t tx_holdoff_delay;        //!< Delay between frames, in UWB usec.
   uint32_t tx_guard_delay;          //!< Delay between frames from subsequent nodes, in UWB sec.
   uint16_t rx_timeout_delay;        //!< Receive response timeout, in UWB usec.
   uint16_t bias_correction:1;       //!< Enable range bias correction polynomial
   uint16_t fctrl_req_ack:1;         //!< Enable ack request
};

//! Range control parameters.
typedef struct _uwb_rng_control_t{
    uint16_t delay_start_enabled:1;  //!< Set for enabling delayed start
    uint16_t complete_after_tx:1;    //!< Set by ranging state machine to say that exchange is complete after next tx
}uwb_rng_control_t;

//! Range status parameters
typedef struct _uwb_rng_status_t{
    uint16_t selfmalloc:1;           //!< Internal flag for memory garbage collection
    uint16_t initialized:1;          //!< Instance allocated
    uint16_t mac_error:1;            //!< Error caused due to frame filtering
    uint16_t invalid_code_error:1;   //!< Error due to invalid code
    uint16_t tx_ack_expected:1;      //!< Acknowledge expected
    uint16_t rx_ack_expected:1;      //!< Acknowledge expected
}uwb_rng_status_t;

//! Range holdoff calculation results
struct uwb_rng_txd{
    uint64_t response_tx_delay;
    uint64_t response_timestamp;
};

//!  TWR final frame format
typedef struct _twr_frame_final_t{
    struct _ieee_rng_response_frame_t;
    uint32_t request_timestamp;     //!< Request transmission timestamp
    uint32_t response_timestamp;    //!< Response reception timestamp
    int32_t carrier_integrator; //!< carrier integrator value
} __attribute__((__packed__, aligned(1))) twr_frame_final_t;

//! TWR data format
typedef struct _twr_data_t{
    triad_t spherical;                 //!< Measurement triad spherical coordinates
    triad_t spherical_variance;        //!< Measurement variance triad
    triad_t cartesian;                 //!< Position triad local coordinates
    //triad_t cartesian_variance;      //!< Position estimated variance triad
    dpl_float32_t rssi;                //!< Receiver strength indicator (dBm)
    dpl_float32_t fppl;                //!< First path strength (dBm)
    dpl_float32_t pdoa;                //!< Phase difference of arrival (rad)
    struct _rng_frame_flags {
        uint16_t has_sts:1;            //!< If this frame was received with STS enabled
        uint16_t has_valid_sts:1;      //!< If the difference between the Ipatov ts and the STS ts was below the error margin
    } flags;
    /* TODO: Restructure below? */
    dpl_float32_t vrssi[3];              //!< [ipatov, sts, sts2] Receiver strength indicator (dBm)
} twr_data_t;

//! TWR frame format
typedef struct {
    union {
        //! Structure of TWR frame, including remote data used by EXT twr types
        struct _twr_frame_t{
            struct _twr_frame_final_t;               //! Structure of TWR final frame
            struct _twr_data_t remote;               //!< Structure of twr_data
        };
        uint8_t array[sizeof(struct _twr_frame_t)];  //!< Array of size twr_frame
    };
    /* Local Data */
    struct _twr_data_t local;                    //!< Structure of twr_data
} twr_frame_t;

//! Only the first TWR_EXT_FRAME_SIZE bytes are sent across uwb in ext-mode
#define TWR_EXT_FRAME_SIZE (offsetof(twr_frame_t, local) - sizeof(struct _twr_data_t))


//! List of range types available
struct rng_config_list {
    uint16_t rng_code;
    const char* name;
    struct uwb_rng_config *config;
    SLIST_ENTRY(rng_config_list) next;
};

//! Structure of range instance
struct uwb_rng_instance{
    struct uwb_dev * dev_inst;              //!< Structure of uwb_dev
#if MYNEWT_VAL(UWB_WCS_ENABLED)
    struct uwb_ccp_instance * ccp_inst;     //!< Structure of CCP
#endif
#if MYNEWT_VAL(RNG_STATS)
    STATS_SECT_DECL(rng_stat_section) stat; //!< Stats instance
#endif
    uint16_t code;                          //!< Range profile code
    uint16_t seq_num;                       //!< Local sequence number
    struct dpl_sem sem;                     //!< Structure of semaphores
    uint64_t delay;                         //!< Delay in transmission
    struct uwb_rng_config config;           //!< Structure of range config
    uwb_rng_control_t control;              //!< Structure of range control
    uwb_rng_status_t status;                //!< Structure of range status
    uint16_t frame_shr_duration;            //!< Precalculated SHR duration
    uint16_t idx;                           //!< Input index to circular buffer
    uint16_t idx_current;                   //!< Output index to circular buffer
    uint16_t nframes;                       //!< Number of buffers defined to store the ranging data
    uint64_t ack_rx_timestamp;              //!< Ack rx timestamp for use later
    struct dpl_event complete_event;        //!< Range complete event

    SLIST_HEAD(, rng_config_list) rng_configs;
    twr_frame_t * frames[];                 //!< Pointer to twr buffers
};


void rng_pkg_init(void);
struct uwb_rng_instance * uwb_rng_init(struct uwb_dev * dev, struct uwb_rng_config * config, uint16_t nframes);
void uwb_rng_free(struct uwb_rng_instance * rng);
struct uwb_dev_status uwb_rng_config(struct uwb_rng_instance * rng, struct uwb_rng_config * config);
struct uwb_dev_status uwb_rng_request(struct uwb_rng_instance * rng, uint16_t dst_address, uwb_dataframe_code_t code);
struct uwb_dev_status uwb_rng_listen(struct uwb_rng_instance * rng, int32_t timeout, uwb_dev_modes_t mode);
struct uwb_dev_status uwb_rng_request_delay_start(struct uwb_rng_instance * rng, uint16_t dst_address, uint64_t delay, uwb_dataframe_code_t code);
struct uwb_dev_status uwb_rng_listen_delay_start(struct uwb_rng_instance * rng, uint64_t dx_time, uint32_t timeout, uwb_dev_modes_t mode);
struct uwb_rng_config * uwb_rng_get_config(struct uwb_rng_instance * rng, uwb_dataframe_code_t code);
void uwb_rng_set_frames(struct uwb_rng_instance * rng, twr_frame_t twr[], uint16_t nframes);
void uwb_rng_clear_twr_data(struct _twr_data_t *s);
dpl_float64_t uwb_rng_twr_to_tof(struct uwb_rng_instance * rng, uint16_t idx);
dpl_float64_t uwb_rng_tof_to_meters(dpl_float64_t ToF);
void uwb_rng_calc_rel_tx(struct uwb_rng_instance * rng, struct uwb_rng_txd *ret, struct uwb_rng_config *cfg, uint64_t ts, uint16_t rx_data_len);
void rng_issue_complete(struct uwb_dev * inst);

#ifndef __KERNEL__
float uwb_rng_path_loss(float Pt, float G, float fc, float R);
float uwb_rng_bias_correction(struct uwb_dev * dev, float Pr);
#endif
uint32_t uwb_rng_twr_to_tof_sym(twr_frame_t twr[], uwb_dataframe_code_t code);

void uwb_rng_append_config(struct uwb_rng_instance * rng, struct rng_config_list *cfgs);
void uwb_rng_remove_config(struct uwb_rng_instance * rng, uwb_dataframe_code_t code);


#ifdef __cplusplus
}
#endif

#endif /* _UWB_RNG_H_ */
