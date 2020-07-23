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
 * @file desense.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2020
 * @brief UWB RF test / Desense
 *
 * @details Package used to check sensitivity to interferance
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <streamer/streamer.h>

#include <stats/stats.h>
#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwb/uwb_ftypes.h>
#include <desense/desense.h>

#if __KERNEL__
#include <linux/kernel.h>

#define slog(fmt, ...)                                                  \
    pr_info("uwb_desense: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

void desense_sysfs_init(struct uwb_desense_instance *desense);
void desense_sysfs_deinit(int idx);
void desense_debugfs_init(struct uwb_desense_instance *desense);
void desense_debugfs_deinit(void);
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

STATS_SECT_START(desense_stat_section)
    STATS_SECT_ENTRY(request_tx)
    STATS_SECT_ENTRY(request_rx)
    STATS_SECT_ENTRY(start_tx)
    STATS_SECT_ENTRY(start_rx)
    STATS_SECT_ENTRY(test_tx)
    STATS_SECT_ENTRY(test_rx)
    STATS_SECT_ENTRY(end_tx)
    STATS_SECT_ENTRY(end_rx)
    STATS_SECT_ENTRY(complete)
    STATS_SECT_ENTRY(tx_error)
    STATS_SECT_ENTRY(rx_error)
STATS_SECT_END

STATS_NAME_START(desense_stat_section)
    STATS_NAME(desense_stat_section, complete)
    STATS_NAME(desense_stat_section, tx_error)
    STATS_NAME(desense_stat_section, request_tx)
    STATS_NAME(desense_stat_section, request_rx)
    STATS_NAME(desense_stat_section, start_tx)
    STATS_NAME(desense_stat_section, start_rx)
    STATS_NAME(desense_stat_section, test_tx)
    STATS_NAME(desense_stat_section, test_rx)
    STATS_NAME(desense_stat_section, end_tx)
    STATS_NAME(desense_stat_section, end_rx)
    STATS_NAME(desense_stat_section, complete)
    STATS_NAME(desense_stat_section, tx_error)
    STATS_NAME(desense_stat_section, rx_error)
STATS_NAME_END(desense_stat_section)

STATS_SECT_DECL(desense_stat_section) g_desense_stat;
#define DESENSE_STATS_INC(__X) STATS_INC(g_desense_stat, __X)

struct uwb_desense_instance g_desense_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};

static void callout_cb(struct dpl_event * ev);

/**
 * @fn translate_tx_power(struct uwb_desense_instance * desense, struct desense_test_parameters *tp)
 * @brief Translate coarse + fine tx power parameters to device
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @param to       Pointer to struct desense_test_parameters from where to extract tx pwr
 *
 * @return int     0 on success
 */
static int
translate_tx_power(struct uwb_desense_instance * desense, struct desense_test_parameters *tp)
{
    int return_val = 0;
    bool ret;
    uint8_t reg;
    uint32_t reg32;

    /* Copy txrf configuration from current system configuration by default */
    memcpy(&desense->strong_txrf_config, &desense->dev_inst->config.txrf, sizeof(desense->strong_txrf_config));
    memcpy(&desense->test_txrf_config, &desense->dev_inst->config.txrf, sizeof(desense->strong_txrf_config));

    /* Only if the test_params have values greater than -1 do they
     * override the current configured values */
    if (desense->req_frame.test_params.strong_coarse_power > -1 ||
        desense->req_frame.test_params.strong_fine_power > -1) {
        ret = uwb_txrf_power_value(desense->dev_inst, &reg,
                                   DPL_FLOAT32_I32_TO_F32((int32_t)desense->req_frame.test_params.strong_coarse_power),
                                   DPL_FLOAT32_I32_TO_F32((int32_t)desense->req_frame.test_params.strong_fine_power)
            );
        if (ret) {
            reg32 = reg;
            desense->strong_txrf_config.power = (reg32 << 24) | (reg32 << 16) | (reg32 << 8) | reg32;
        } else {
            printf("%s:%d Warning, invalid strong TX power values(%d,%d), defaults used.\n", __func__, __LINE__,
                   desense->req_frame.test_params.strong_coarse_power,
                   desense->req_frame.test_params.strong_fine_power);
            return_val = DPL_EINVAL;
        }
    }

    if (desense->req_frame.test_params.test_coarse_power > -1 ||
        desense->req_frame.test_params.test_fine_power > -1) {
        ret = uwb_txrf_power_value(desense->dev_inst, &reg,
                                   DPL_FLOAT32_I32_TO_F32((int32_t)desense->req_frame.test_params.test_coarse_power),
                                   DPL_FLOAT32_I32_TO_F32((int32_t)desense->req_frame.test_params.test_fine_power)
            );
        if (ret) {
            reg32 = reg;
            desense->test_txrf_config.power = (reg32 << 24) | (reg32 << 16) | (reg32 << 8) | reg32;
        } else {
            printf("%s:%d Warning, invalid test TX power values (%d,%d), defaults used.\n", __func__, __LINE__,
                   desense->req_frame.test_params.test_coarse_power, desense->req_frame.test_params.test_fine_power);
            return_val = DPL_EINVAL;
        }
    }
    return return_val;
}

/**
 * @fn send_start_end(struct uwb_desense_instance * desense, uint16_t code)
 * @brief Send start or end packet to requester
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @param code     UWB_DATA_CODE_DESENSE_START or UWB_DATA_CODE_DESENSE_END
 *
 * @return void
 */
static void
send_start_end(struct uwb_desense_instance * desense, uint16_t code)
{
    struct uwb_dev * inst = desense->dev_inst;

    /* Set strong tx_power */
    uwb_txrf_config(inst, &desense->strong_txrf_config);

    /* START (and END) frame has the same structure as request frame but with a different
     * frame code. This means the requester can verify that the parameters came
     * through properly and there were no errors when receiving the START frame. */
    desense->start_frame.head.code = code;
    desense->start_frame.head.dst_address = desense->req_frame.head.src_address;
    desense->start_frame.head.src_address = inst->my_short_address;
    desense->start_frame.head.seq_num++;

    /* write frame to device */
    uwb_write_tx(inst, (uint8_t*)&desense->start_frame, 0, sizeof(desense->req_frame));
    uwb_write_tx_fctrl(inst, sizeof(desense->start_frame), 0);

    /* Send immediately */
    if (uwb_start_tx(inst).start_tx_error) {
        DESENSE_STATS_INC(tx_error);
        dpl_sem_release(&desense->sem);
    } else {
        if (code == UWB_DATA_CODE_DESENSE_START) {
            printf("%s:%d START sent\n", __func__, __LINE__);
            desense->num_start_sent++;
        } else {
            printf("%s:%d END sent\n", __func__, __LINE__);
            desense->num_end_sent++;
        }
    }
    return;
}

/**
 * @fn send_test(struct uwb_desense_instance * desense)
 * @brief Send test packet to requester
 *
 * @param desense  Pointer to struct uwb_desense_instance
 *
 * @return void
 */
static void
send_test(struct uwb_desense_instance * desense)
{
    struct uwb_dev * inst = desense->dev_inst;
    struct desense_test_frame *frame = &desense->test_frame;
    /* Set test tx_power */
    uwb_txrf_config(inst, &desense->test_txrf_config);

    frame->head.fctrl = UWB_FCTRL_STD_DATA_FRAME;
    frame->head.PANID = desense->req_frame.head.PANID;
    frame->head.dst_address = desense->req_frame.head.src_address;
    frame->head.src_address = inst->my_short_address;
    frame->head.code = UWB_DATA_CODE_DESENSE_TEST;
    frame->head.seq_num++;

    /* write frame to device */
    uwb_write_tx(inst, (uint8_t*)frame, 0, sizeof(struct desense_test_frame));
    uwb_write_tx_fctrl(inst, sizeof(struct desense_test_frame), 0);

    /* Send immediately */
    if (uwb_start_tx(inst).start_tx_error) {
        DESENSE_STATS_INC(tx_error);
        dpl_sem_release(&desense->sem);
    } else {
        printf("%s:%d TEST %"PRIu32"\n", __func__, __LINE__, desense->num_test_sent);
        desense->num_test_sent++;
    }

    /* Increase value in payload after transmission */
    frame->test_nth_packet++;
    return;
}

/**
 * @fn reset_stats(struct uwb_desense_instance * desense)
 * @brief Reset internal device event counters
 *
 * @param desense  Pointer to struct uwb_desense_instance
 *
 * @return void
 */
static void
reset_stats(struct uwb_desense_instance * desense)
{
    uwb_event_cnt_ctrl(desense->dev_inst, true, true);
}

/**
 * @fn desense_dump_data(struct uwb_desense_instance * desense, desense_out_cb_t cb)
 * @brief Dump packet data and individual measurements from round on callback cb
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @param cb       Callback used for formatting output
 *
 * @return void
 */
void
desense_dump_data(struct uwb_desense_instance * desense, struct streamer *streamer)
{
    int i;
    /* Dump packages */
    for (i=0;i<desense->test_log_idx;i++) {
        streamer_printf(streamer, "{\"idx\": %d, \"nth_pkt\": %"PRIu32", ",
           i,
           desense->test_log[i].frame.test_nth_packet
            );
        streamer_printf(streamer, "\"rssi\": "DPL_FLOAT32_PRINTF_PRIM", ",
           DPL_FLOAT32_PRINTF_VALS(desense->test_log[i].rssi));
        streamer_printf(streamer, "\"clko_ppm\": "DPL_FLOAT64_PRINTF_PRIM"}\n",
           DPL_FLOAT64_PRINTF_VALS(desense->test_log[i].clko));
    }
}

/**
 * @fn desense_dump_stats(struct uwb_desense_instance * desense, desense_out_cb_t cb)
 * @brief Dump statistics from measurement round on callback cb
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @param cb       Callback used for formatting output
 *
 * @return void
 */
void
desense_dump_stats(struct uwb_desense_instance * desense, struct streamer *streamer)
{
    int i;
    int32_t nom;
    dpl_float32_t rssi_sum = DPL_FLOAT32_INIT(0.0f);
    dpl_float64_t clko_sum = DPL_FLOAT64_INIT(0.0l);
    dpl_float64_t nomf64, n_test;
    dpl_float64_t per = DPL_FLOAT64_INIT(0.0l);
    struct uwb_dev_evcnt *cnt = &desense->evc;

    /* Collate rssi and clkoffset from packages */
    for (i=0;i<desense->test_log_idx;i++) {
        rssi_sum = DPL_FLOAT32_ADD(rssi_sum, desense->test_log[i].rssi);
        clko_sum = DPL_FLOAT64_ADD(clko_sum, desense->test_log[i].clko);
    }

    streamer_printf(streamer, "\n# Event counters:\n");
    streamer_printf(streamer, "  RXPHE:  %6d  # rx PHR err\n", cnt->ev0s.count_rxphe);
    streamer_printf(streamer, "  RXFSL:  %6d  # rx sync loss\n", cnt->ev0s.count_rxfsl);
    streamer_printf(streamer, "  RXFCG:  %6d  # rx CRC OK\n", cnt->ev1s.count_rxfcg);
    streamer_printf(streamer, "  RXFCE:  %6d  # rx CRC Errors\n", cnt->ev1s.count_rxfce);
    streamer_printf(streamer, "  ARFE:   %6d  # addr filt err\n", cnt->ev2s.count_arfe);
    streamer_printf(streamer, "  RXOVRR: %6d  # rx overflow\n", cnt->ev2s.count_rxovrr);
    streamer_printf(streamer, "  RXSTO:  %6d  # rx SFD TO\n", cnt->ev3s.count_rxsto);
    streamer_printf(streamer, "  RXPTO:  %6d  # pream search TO\n", cnt->ev3s.count_rxpto);
    streamer_printf(streamer, "  FWTO:   %6d  # rx frame wait TO\n", cnt->ev4s.count_fwto);
    streamer_printf(streamer, "  TXFRS:  %6d  # tx frames sent\n", cnt->ev4s.count_txfrs);

    nom = desense->req_frame.test_params.n_test - cnt->ev1s.count_rxfcg + 1;
    nomf64 = DPL_FLOAT64_I32_TO_F64(nom);
    if (desense->req_frame.test_params.n_test) {
        n_test = DPL_FLOAT64_I32_TO_F64(desense->req_frame.test_params.n_test);
        per = DPL_FLOAT64_DIV(nomf64, n_test);
    }

    streamer_printf(streamer, "\n  PER:  "DPL_FLOAT64_PRINTF_PRIM"\n", DPL_FLOAT64_PRINTF_VALS(per));
    if (desense->test_log_idx) {
        rssi_sum = DPL_FLOAT32_DIV(rssi_sum, DPL_FLOAT32_I32_TO_F32(desense->test_log_idx));
        clko_sum = DPL_FLOAT64_DIV(clko_sum, DPL_FLOAT64_I32_TO_F64(desense->test_log_idx));
    }
    streamer_printf(streamer, "  RSSI: "DPL_FLOAT32_PRINTF_PRIM"  dBm\n", DPL_FLOAT32_PRINTF_VALS(rssi_sum));
    streamer_printf(streamer, "  CLKO: "DPL_FLOAT64_PRINTF_PRIM" ppm\n", DPL_FLOAT64_PRINTF_VALS(clko_sum));

}

/**
 * @fn callout_cb(struct dpl_event * ev)
 * @brief State machine handling delays and transmit for Tag mode
 *
 * @param ev  Pointer to struct dpl_event.
 *
 * @return void
 */
static void
callout_cb(struct dpl_event * ev)
{
    assert(ev != NULL);
    struct uwb_desense_instance * desense = (struct uwb_desense_instance *)dpl_event_get_arg(ev);
    assert(desense);

    switch (desense->state) {
    case (DESENSE_STATE_IDLE): {
        break;
    }
    case (DESENSE_STATE_RX): {
        break;
    }
    case (DESENSE_STATE_REQUEST): {
        break;
    }
    case DESENSE_STATE_START: {

        if (desense->num_start_sent < desense->req_frame.test_params.n_strong) {
            send_start_end(desense, UWB_DATA_CODE_DESENSE_START);
            dpl_callout_reset(&desense->callout,
                              dpl_time_ms_to_ticks32(
                                  desense->req_frame.test_params.ms_test_delay));
        } else {
            desense->state = DESENSE_STATE_TEST;
            dpl_callout_reset(&desense->callout,
                              dpl_time_ms_to_ticks32(
                                  desense->req_frame.test_params.ms_start_delay));
        }
        break;
    }
    case DESENSE_STATE_TEST: {
        if (desense->num_test_sent < desense->req_frame.test_params.n_test) {
            send_test(desense);
            dpl_callout_reset(&desense->callout,
                              dpl_time_ms_to_ticks32(
                                  desense->req_frame.test_params.ms_test_delay));
        } else {
            desense->state = DESENSE_STATE_END;
            dpl_callout_reset(&desense->callout, 0);
        }
        break;
    }
    case DESENSE_STATE_END: {
        if (desense->num_end_sent < desense->req_frame.test_params.n_strong) {
            send_start_end(desense, UWB_DATA_CODE_DESENSE_END);
            dpl_callout_reset(&desense->callout,
                              dpl_time_ms_to_ticks32(
                                  desense->req_frame.test_params.ms_test_delay));
        } else {
            desense->state = DESENSE_STATE_FINISH;
            dpl_callout_reset(&desense->callout,
                              dpl_time_ms_to_ticks32(
                                  desense->req_frame.test_params.ms_start_delay));
        }
        break;
    }
    case DESENSE_STATE_FINISH: {
        if(dpl_sem_get_count(&desense->sem) == 0){
            dpl_sem_release(&desense->sem);
        }
        /* Restart rx if we're in continuous rx mode */
        if (desense->do_continuous_rx) {
            desense_listen(desense);
        }
        break;
    }
    }
}

/**
 * @fn rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Receive complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true if we accepted package
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct _ieee_std_frame_t * frame;
    struct desense_test_log *log_elem;
    struct desense_request_frame * req;
    struct uwb_desense_instance * desense = (struct uwb_desense_instance *)cbs->inst_ptr;
    if (inst->fctrl != UWB_FCTRL_STD_DATA_FRAME)
        return false;

    if (dpl_sem_get_count(&desense->sem) == 1) {
        /* unsolicited inbound */
        return false;
    }

    if (inst->frame_len < sizeof(struct _ieee_std_frame_t)) {
        return false;
    }
    frame = (struct _ieee_std_frame_t *)inst->rxbuf;

    if (frame->dst_address != inst->my_short_address && frame->dst_address != UWB_BROADCAST_ADDRESS) {
        if(dpl_sem_get_count(&desense->sem) == 0){
            dpl_sem_release(&desense->sem);
            printf("%s:%d Frame for %x received, aborting\n", __func__, __LINE__, frame->dst_address);
        }
    }

    switch(frame->code) {
        case UWB_DATA_CODE_DESENSE_REQUEST:
            {
                /* This code executes on the device that is responding to a request */
                if (inst->frame_len != sizeof(struct desense_request_frame)) {
                    return false;
                }
                DESENSE_STATS_INC(request_rx);
                req = (struct desense_request_frame *)inst->rxbuf;

                /* Copy request frame and parameters */
                memcpy(&desense->req_frame, req, sizeof(desense->req_frame));
                desense->req_frame.ret = translate_tx_power(desense, &desense->req_frame.test_params);

                /* Require at least 1 stong and 1 test packet */
                if (!desense->req_frame.test_params.n_strong) {
                    desense->req_frame.test_params.n_strong = 1;
                }
                if (!desense->req_frame.test_params.n_test) {
                    desense->req_frame.test_params.n_test = 1;
                }
                /* Copy request frame and parameters to our start/end frame */
                memcpy(&desense->start_frame, &desense->req_frame, sizeof(desense->start_frame));

                desense->state = DESENSE_STATE_START;
                desense->num_start_sent = 0;
                desense->num_test_sent = 0;
                desense->num_end_sent = 0;
                desense->test_frame.test_nth_packet = 0;
                dpl_callout_reset(&desense->callout, 0);
                break;
            }
        case UWB_DATA_CODE_DESENSE_START:
            {
                /* This code executes on the device that initiated a request, */
                /* and is now preparing for receiving test data */
                if (inst->frame_len != sizeof(struct desense_request_frame))
                    break;
                req = (struct desense_request_frame *)inst->rxbuf;
                if (req->ret) {
                    printf("WARNING Test sender reported issue with test parameters\n");
                }
                DESENSE_STATS_INC(start_rx);

                reset_stats(desense);
                desense->state = DESENSE_STATE_TEST;
                desense->test_log_idx = 0;
                if (desense->test_log_size < req->test_params.n_test) {
                    if (desense->test_log) {
                        free(desense->test_log);
                    }
                    desense->test_log = calloc(req->test_params.n_test, sizeof(struct desense_test_log));
                    if (!desense->test_log) {
                        printf("WARNING Error allocating memory for %"PRIu32" packets\n", req->test_params.n_test);
                    }
                    desense->test_log_size = req->test_params.n_test;
                }
                break;
            }
        case UWB_DATA_CODE_DESENSE_TEST:
            {
                /* This code executes on the device that initiated a request, */
                /* and is now sampling test data */
                if (inst->frame_len != sizeof(struct desense_test_frame))
                   break;
                DESENSE_STATS_INC(test_rx);

                /* Store frame data + rssi */
                if (desense->test_log) {
                    log_elem = &desense->test_log[desense->test_log_idx%desense->test_log_size];
                    memcpy(&log_elem->frame,
                           inst->rxbuf,
                           sizeof(struct desense_test_frame));
                    log_elem->rssi = uwb_get_rssi(inst);
                    log_elem->clko = uwb_calc_clock_offset_ratio(inst,
                                                                 inst->carrier_integrator,
                                                                 UWB_CR_CARRIER_INTEGRATOR);
                    /* Convert to ppm */
                    log_elem->clko = DPL_FLOAT64_MUL(log_elem->clko, DPL_FLOAT64_INIT(1.0e6));
                    desense->test_log_idx++;
                }
                break;
            }
        case UWB_DATA_CODE_DESENSE_END:
            {
                /* This code executes on the device that initiated a request, */
                /* and is now ending the test and preparing the results */

                if (inst->frame_len != sizeof(struct desense_request_frame))
                   break;

                DESENSE_STATS_INC(end_rx);
                DESENSE_STATS_INC(complete);
                /* Read event counters from device */
                uwb_event_cnt_read(desense->dev_inst, &desense->evc);
#ifndef __KERNEL__
                streamer_printf(streamer_console_get(), "\n# Packets received:\n");
                desense_dump_data(desense, streamer_console_get());
                desense_dump_stats(desense, streamer_console_get());
#endif
                uwb_stop_rx(inst);
                dpl_sem_release(&desense->sem);
                break;
            }
        default:
                return false;
                break;
    }

    return true;
}

/**
 * @fn tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Transmission complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true if active
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_desense_instance * desense = (struct uwb_desense_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&desense->sem) == 1) {
        return false;
    }
    return true;
}

/**
 * @fn rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Receive timeout callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true if used/accepted
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_desense_instance * desense = (struct uwb_desense_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&desense->sem) == 1) {
        return false;
    }

    if (desense->state == DESENSE_STATE_RX) {
        uwb_set_rx_timeout(inst, 0xfffff);

        if (uwb_start_rx(inst).start_rx_error) {
            DESENSE_STATS_INC(rx_error);
            printf("%s:%d Rx error\n", __func__, __LINE__);
            dpl_sem_release(&desense->sem);
            return DPL_ERROR;
        }
    }

    if (desense->state == DESENSE_STATE_REQUEST) {
        printf("%s:%d Rx timeout waiting for START\n", __func__, __LINE__);
        desense->state = DESENSE_STATE_IDLE;
        dpl_sem_release(&desense->sem);
    }

    if (desense->state == DESENSE_STATE_TEST) {
        uwb_set_rx_timeout(inst, 0xfffff);

        if (uwb_start_rx(inst).start_rx_error) {
            DESENSE_STATS_INC(rx_error);
            printf("%s:%d Rx error\n", __func__, __LINE__);
            dpl_sem_release(&desense->sem);
            return DPL_ERROR;
        }
    }
    return true;
}

/**
 * @fn reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Reset callback from mac layer
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return true on success
 */
static bool
reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_desense_instance * desense = (struct uwb_desense_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&desense->sem) == 1) {
        return false;
    }
    printf("%s:%d Rx Reset\n", __func__, __LINE__);
    dpl_sem_release(&desense->sem);
    return true;
}

/**
 * @fn desense_send_request(struct uwb_desense_instance * desense, uint16_t dst_address, struct desense_test_parameters *tp)
 * @brief Send desense request
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @param length   Address of listening tag to send to
 * @param tp       Pointer to test-parameters if given, NULL to use last parameters
 * @return int     0 if ok
 */
int
desense_send_request(struct uwb_desense_instance * desense, uint16_t dst_address,
                     struct desense_test_parameters *tp)
{
    dpl_error_t err;
    struct uwb_dev * inst = desense->dev_inst;
    struct desense_request_frame *frame = &desense->req_frame;

    err = dpl_sem_pend(&desense->sem, dpl_time_ms_to_ticks32(1000));
    if (err == DPL_TIMEOUT) {
        printf("%s:%d Sem timeout\n", __func__, __LINE__);
        return DPL_TIMEOUT;
    }

    /* Enable ipatov diagnostics */
    inst->rxdiag->enabled = UWB_RXDIAG_IPATOV | UWB_RXDIAG_COMB_XTALO | UWB_RXDIAG_COMB_TPDOA;
    inst->config.rxdiag_enable = 1;

    /* Disable frame filter */
    // uwb_mac_framefilter(inst, 0x0);

    frame->head.fctrl = UWB_FCTRL_STD_DATA_FRAME;
    frame->head.PANID = inst->pan_id;
    frame->head.dst_address = dst_address;
    frame->head.src_address = inst->my_short_address;
    frame->head.code = UWB_DATA_CODE_DESENSE_REQUEST;
    frame->head.seq_num++;

    /* Only overwrite test-parameters if given as argument */
    if (tp) {
        memcpy(&frame->test_params, tp, sizeof(frame->test_params));
    }

    /* write frame to device */
    desense->state = DESENSE_STATE_REQUEST;
    uwb_write_tx(inst, (uint8_t*)frame, 0, sizeof(struct desense_request_frame));
    uwb_write_tx_fctrl(inst, sizeof(struct desense_request_frame), 0);

    /* Prepare to listen after sent packet */
    uwb_set_wait4resp(inst, true);
    uwb_set_rx_timeout(inst, 0xfffff);

    /* Send immediately */
    if (uwb_start_tx(inst).start_tx_error) {
        DESENSE_STATS_INC(tx_error);
        printf("%s:%d Tx error\n", __func__, __LINE__);
        dpl_sem_release(&desense->sem);
        return DPL_ERROR;
    } else {
        DESENSE_STATS_INC(request_tx);
    }

    printf("%s:%d Request sent\n", __func__, __LINE__);
#if 0
    /* Wait for completion of transactions */
    err = dpl_sem_pend(&desense->sem, dpl_time_ms_to_ticks32(10000));
    if (err != DPL_OK) {
        return DPL_TIMEOUT;
    }
    dpl_sem_release(&desense->sem);
#endif
    return DPL_OK;
}

/**
 * @fn desense_listen(struct uwb_desense_instance * desense)
 * @brief Listen for requests
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @return int     0 if ok
 */
int
desense_listen(struct uwb_desense_instance * desense)
{
    dpl_error_t err;
    struct uwb_dev * inst = desense->dev_inst;

    err = dpl_sem_pend(&desense->sem, dpl_time_ms_to_ticks32(1000));
    if (err == DPL_TIMEOUT) {
        printf("%s:%d Sem timeout\n", __func__, __LINE__);
        return DPL_TIMEOUT;
    }

    /* Prepare to listen */
    desense->state = DESENSE_STATE_RX;
    uwb_set_rx_timeout(inst, 0xfffff);

    if (uwb_start_rx(inst).start_rx_error) {
        DESENSE_STATS_INC(rx_error);
        printf("%s:%d Rx error\n", __func__, __LINE__);
        dpl_sem_release(&desense->sem);
        return DPL_ERROR;
    }

    printf("%s:%d Listening\n", __func__, __LINE__);
    return DPL_OK;
}

/**
 * @fn desense_abort(struct uwb_desense_instance * desense)
 * @brief Abort any current listening or request
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @return int     0 if ok
 */
int
desense_abort(struct uwb_desense_instance * desense)
{
    dpl_error_t err;
    struct uwb_dev * inst = desense->dev_inst;

    if(dpl_sem_get_count(&desense->sem) == 1) {
        printf("%s:%d No ongoing operation\n", __func__, __LINE__);
    }

    desense->do_continuous_rx = 0;
    uwb_phy_forcetrxoff(inst);
    err = dpl_sem_pend(&desense->sem, dpl_time_ms_to_ticks32(1000));
    if (err == DPL_TIMEOUT) {
        printf("%s:%d Sem timeout, forceful release needed\n", __func__, __LINE__);
    }
    dpl_sem_release(&desense->sem);
    desense->state = DESENSE_STATE_IDLE;
    printf("%s:%d Aborted Ok\n", __func__, __LINE__);
    return DPL_OK;
}


/**
 * @fn desense_txon(struct uwb_desense_instance * desense, uint16_t length, uint32_t delay_ns)
 * @brief Turn on aggressor transmission
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @param length   length of data to send
 * @param delay_ns Delay in ns from start of packet n to start of packet n+1
 * @return int     0 if ok
 */
int
desense_txon(struct uwb_desense_instance * desense, uint16_t length, uint32_t delay_ns)
{
    int i, offs = 0;
    uint8_t buf[8] = {0};
    dpl_error_t err;
    uint64_t delay_dtu;

    err = dpl_sem_pend(&desense->sem, dpl_time_ms_to_ticks32(1000));
    if (err == DPL_TIMEOUT) {
        printf("%s:%d Sem timeout\n", __func__, __LINE__);
        return DPL_TIMEOUT;
    }

    /* Prepare data to send */
    for (i=0;i<length;i+=sizeof(buf)) {
        /* Any random set of bytes would be ok here */
        memcpy(buf, (uint8_t[8]){0xde,0xca,0xaa,0x55,0xaa,0x55,0xaa,0x55}, 8);
        if (i==0) {
            /* Make sure fctrl bytes are invalid (0x00 00) */
            buf[0] = 0;
            buf[1] = 0;
        }
        uwb_write_tx(desense->dev_inst, buf, offs, sizeof(buf));
        offs += sizeof(buf);
    }

    /* Set length and offset of frame for tranceiver to use */
    uwb_write_tx_fctrl(desense->dev_inst, length, 0);

    /* Recalculate delay into dtu: 1000 ns = 65535 dtu */
    /* 65535 * 1024 / 1000 = 67107 - saves a 64bit division in kernel */
    delay_dtu = ((uint64_t)delay_ns * 67107)>>10;
    uwb_phy_repeated_frames(desense->dev_inst, delay_dtu);

    return DPL_OK;
}

/**
 * @fn desense_txoff(struct uwb_desense_instance * desense)
 * @brief Turn off aggressor transmission
 *
 * @param desense  Pointer to struct uwb_desense_instance
 * @return int     0 if ok
 */
int
desense_txoff(struct uwb_desense_instance * desense)
{
    uwb_phy_repeated_frames(desense->dev_inst, 0);
    dpl_sem_release(&desense->sem);
    return DPL_OK;
}

/**
 * @fn uwb_desense_pkg_init(void)
 * @brief Initialise the desense package.
 *
 * @return void
 */
void
uwb_desense_pkg_init(void)
{
    int i;
    dpl_error_t err;
    struct uwb_dev *udev;
    struct uwb_desense_instance *desense;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"uwb_desense_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i=0;i < ARRAY_SIZE(g_desense_inst);i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        desense = &g_desense_inst[i];
        desense->dev_inst = udev;
        desense->cbs = (struct uwb_mac_interface){
                .id = UWBEXT_RF_DESENSE,
                .inst_ptr = desense,
                .rx_complete_cb = rx_complete_cb,
                .tx_complete_cb = tx_complete_cb,
                .rx_timeout_cb = rx_timeout_cb,
                .reset_cb = reset_cb,
        };

        err = dpl_sem_init(&desense->sem, 0x1);
        assert(err == DPL_OK);
        dpl_callout_init(&desense->callout, dpl_eventq_dflt_get(), callout_cb, (void *) desense);

        uwb_mac_append_interface(udev, &desense->cbs);

#if __KERNEL__
        desense_sysfs_init(desense);
        desense_debugfs_init(desense);
#endif
    }

#ifndef __KERNEL__
    {
        int rc;
        rc = stats_init(
            STATS_HDR(g_desense_stat),
            STATS_SIZE_INIT_PARMS(g_desense_stat, STATS_SIZE_32),
            STATS_NAME_INIT_PARMS(desense_stat_section));
        assert(rc == 0);

        rc |= stats_register("desense", STATS_HDR(g_desense_stat));
        assert(rc == 0);
    }
#endif
    desense_cli_register();
}

/**
 * @fn uwb_desense_free(struct uwb_dev * dev)
 * @brief API to free the allocated resources.
 *
 * @return void
 */
void
uwb_desense_free(struct uwb_desense_instance * desense)
{
    uwb_mac_remove_interface(desense->dev_inst, UWBEXT_RF_DESENSE);
    if (desense->test_log) {
        free(desense->test_log);
        desense->test_log = 0;
    }
#if __KERNEL__
    desense_sysfs_deinit(desense->dev_inst->idx);
    desense_debugfs_deinit();
#endif
}

/**
 * @fn uwb_desense_pkg_down(int reason)
 * @brief Shutdown and remove desense module from system
 *
 * @param reason   Not used
 * @return int     0 if ok
 */
int
uwb_desense_pkg_down(int reason)
{
    int i;
    struct uwb_dev *udev;

    for (i=0;i < ARRAY_SIZE(g_desense_inst);i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        uwb_desense_free(&g_desense_inst[i]);
    }
    desense_cli_down(reason);

    return 0;
}
