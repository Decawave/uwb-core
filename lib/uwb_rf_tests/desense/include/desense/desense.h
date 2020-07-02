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
 * @file desense.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2020
 * @brief UWB RF test / Desense
 *
 * @details Package used to check sensitivity to interferance
 *
 */

#ifndef _UWB_RF_TEST_DESENSE_H_
#define _UWB_RF_TEST_DESENSE_H_


#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <streamer/streamer.h>

//! Structure of desense test parameters
struct desense_test_parameters {
    uint16_t ms_start_delay;                //!< ms delay between last strong start packet and first test packet
    uint16_t ms_test_delay;                 //!< ms delay between test packets
    int8_t strong_coarse_power;             //!< Coarse tx-power to use during initial and final packets (if -1 return to uwbcfg value)
    int8_t strong_fine_power;               //!< Fine tx-power to use during initial and final packets (if -1 return to uwbcfg value)
    uint16_t n_strong;                      //!< Number of strong initial and final packets to send
    int8_t test_coarse_power;               //!< Coarse tx-power to use during test
    int8_t test_fine_power;                 //!< Fine tx-power to use during test
    uint16_t _padding;                      //!< Padding for 32bit alignment
    uint32_t n_test;                        //!< Number of test packets to send
} __attribute__((__packed__, aligned(1)));

//! Structure of desense request packet
struct desense_request_frame {
    struct _ieee_std_frame_t head;
    struct desense_test_parameters test_params;
    uint8_t ret;
} __attribute__((__packed__, aligned(1)));

//! Structure of desense test packet
struct desense_test_frame {
    struct _ieee_std_frame_t head;
    uint32_t test_nth_packet;
} __attribute__((__packed__, aligned(1)));

//! Structure of desense test packet
struct desense_test_log {
    struct desense_test_frame frame;
    dpl_float32_t rssi;
    dpl_float64_t clko;
};

//! Tag states
typedef enum _uwb_desense_state {
    DESENSE_STATE_IDLE,                 //!< Idle state
    DESENSE_STATE_RX,                   //!< Waiting for request
    DESENSE_STATE_REQUEST,              //!< Sending request
    DESENSE_STATE_START,                //!< Send i'th start packet
    DESENSE_STATE_TEST,                 //!< N'th TEST packet
    DESENSE_STATE_END,                  //!< END of sequence
    DESENSE_STATE_FINISH,               //!< Finished
} uwb_desense_state_t;

//! Structure of desense instance
struct uwb_desense_instance {
    struct uwb_dev * dev_inst;              //!< Structure of uwb_dev
    struct dpl_sem sem;                     //!< Semaphore indicating test in progress

    struct desense_request_frame req_frame; //!< Request frame
    struct desense_request_frame start_frame; //!< Start frame
    struct desense_test_frame test_frame;   //!< Test frame
    struct uwb_dev_txrf_config strong_txrf_config; //!< Strong txrf config
    struct uwb_dev_txrf_config test_txrf_config; //!< Test txrf config

    uwb_desense_state_t state;              //!< Tag state
    struct dpl_callout callout;             //!< Tag state machine callout
    uint32_t num_start_sent;                //!< Number of START packets sent
    uint32_t num_test_sent;                 //!< Number of TEST packets sent
    uint32_t num_end_sent;                  //!< Number of END packets sent

    struct uwb_mac_interface cbs;           //!< MAC layer callback structure

    struct uwb_dev_evcnt evc;               //!< Event counter data
    struct desense_test_log *test_log;      //!< Log of test packets received
    uint32_t test_log_idx;                  //!< Index into test_log
    uint32_t test_log_size;                 //!< Size of test_log in number of packets
};

void uwb_desense_free(struct uwb_desense_instance * desense);
int desense_send_request(struct uwb_desense_instance * desense, uint16_t dst_address, struct desense_test_parameters *tp);
int desense_listen(struct uwb_desense_instance * desense);
int desense_txon(struct uwb_desense_instance * desense, uint16_t length, uint32_t delay_ns);
int desense_txoff(struct uwb_desense_instance * desense);

void desense_dump_data(struct uwb_desense_instance * desense, struct streamer *streamer);
void desense_dump_stats(struct uwb_desense_instance * desense, struct streamer *streamer);

int desense_cli_register(void);
int desense_cli_down(int reason);

void uwb_desense_pkg_init(void);
int uwb_desense_pkg_down(int reason);

#ifdef __cplusplus
}
#endif

#endif
