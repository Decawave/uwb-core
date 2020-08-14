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
 * @file twr_ss_ack.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Range
 *
 * @details Single sided ranging using a hw generated ack as the response
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>

#include <stats/stats.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <uwb_rng/uwb_rng.h>
#include <dsp/polyval.h>

#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif

#if MYNEWT_VAL(RNG_VERBOSE)
#define DIAGMSG(s,u) printf(s,u)
#endif
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

static struct uwb_mac_interface g_cbs[] = {
        [0] = {
            .id = UWBEXT_RNG_SS_ACK,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
        },
#if MYNEWT_VAL(UWB_DEVICE_1) ||  MYNEWT_VAL(UWB_DEVICE_2)
        [1] = {
            .id = UWBEXT_RNG_SS_ACK,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
        },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
        [2] = {
            .id = UWBEXT_RNG_SS_ACK,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
        }
#endif
};

#if MYNEWT_VAL(TWR_SS_ACK_STATS)
STATS_SECT_START(twr_ss_ack_stat_section)
    STATS_SECT_ENTRY(complete)
    STATS_SECT_ENTRY(tx_error)
    STATS_SECT_ENTRY(ack_tx_err)
    STATS_SECT_ENTRY(ack_rx_err)
    STATS_SECT_ENTRY(ack_seq_err)
STATS_SECT_END

STATS_NAME_START(twr_ss_ack_stat_section)
    STATS_NAME(twr_ss_ack_stat_section, complete)
    STATS_NAME(twr_ss_ack_stat_section, tx_error)
    STATS_NAME(twr_ss_ack_stat_section, ack_tx_err)
    STATS_NAME(twr_ss_ack_stat_section, ack_rx_err)
    STATS_NAME(twr_ss_ack_stat_section, ack_seq_err)
STATS_NAME_END(twr_ss_ack_stat_section)

STATS_SECT_DECL(twr_ss_ack_stat_section) g_twr_ss_ack_stat;
#define SS_STATS_INC(__X) STATS_INC(g_twr_ss_ack_stat, __X)
#else
#define SS_STATS_INC(__X) {}
#endif

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(TWR_SS_ACK_TX_HOLDOFF),       // Send Time delay in usec.
    .rx_timeout_delay = MYNEWT_VAL(TWR_SS_ACK_RX_TIMEOUT),       // Receive response timeout in usec
    .fctrl_req_ack = 1
};

static struct rng_config_list g_rng_cfgs[] = {
    [0] = {
        .rng_code = UWB_DATA_CODE_SS_TWR_ACK,
        .name = "twr_ss_ack",
        .config = &g_config
    },
#if MYNEWT_VAL(UWB_DEVICE_1) ||  MYNEWT_VAL(UWB_DEVICE_2)
    [1] = {
        .rng_code = UWB_DATA_CODE_SS_TWR_ACK,
        .name = "twr_ss_ack",
        .config = &g_config
    },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    [2] = {
        .rng_code = UWB_DATA_CODE_SS_TWR_ACK,
        .name = "twr_ss_ack",
        .config = &g_config
    },
#endif
};


/**
 * @fn twr_ss_pkg_init(void)
 * @brief API to initialise the rng_ss_ack package.
 *
 * @return void
 */
void
twr_ss_ack_pkg_init(void)
{
    int i, rc;
    struct uwb_dev *udev;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"twr_ss_ack_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i=0;i < sizeof(g_cbs)/sizeof(g_cbs[0]);i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        g_cbs[i].inst_ptr = (struct uwb_rng_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_RNG);
        uwb_mac_append_interface(udev, &g_cbs[i]);
        uwb_rng_append_config(g_cbs[i].inst_ptr, &g_rng_cfgs[i]);
    }

#if MYNEWT_VAL(TWR_SS_ACK_STATS)
    rc = stats_init(
        STATS_HDR(g_twr_ss_ack_stat),
        STATS_SIZE_INIT_PARMS(g_twr_ss_ack_stat, STATS_SIZE_32),
        STATS_NAME_INIT_PARMS(twr_ss_ack_stat_section));
    assert(rc == 0);

    rc |= stats_register("twr_ss_ack", STATS_HDR(g_twr_ss_ack_stat));
    assert(rc == 0);
#endif
}

/**
 * @fn twr_ss_free(struct uwb_dev * inst)
 * @brief API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
twr_ss_ack_free(struct uwb_dev * dev)
{
    assert(dev);
    uwb_mac_remove_interface(dev, UWBEXT_RNG_SS_ACK);
}

int
twr_ss_ack_pkg_down(int reason)
{
#if MYNEWT_VAL(UWB_DEVICE_0)
    if (g_cbs[0].inst_ptr) {
        struct uwb_rng_instance * rng = (struct uwb_rng_instance *)g_cbs[0].inst_ptr;
        twr_ss_ack_free(rng->dev_inst);
    }
#endif
#if MYNEWT_VAL(UWB_DEVICE_1)
    if (g_cbs[1].inst_ptr) {
        struct uwb_rng_instance * rng = (struct uwb_rng_instance *)g_cbs[1].inst_ptr;
        twr_ss_ack_free(rng->dev_inst);
    }
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    if (g_cbs[2].inst_ptr) {
        struct uwb_rng_instance * rng = (struct uwb_rng_instance *)g_cbs[2].inst_ptr;
        twr_ss_ack_free(rng->dev_inst);
    }
#endif
    return 0;
}

/**
 * @fn rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for receive complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true on sucess
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    twr_frame_t * frame;
    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED) &&
        inst->fctrl != UWB_FCTRL_FRAME_TYPE_ACK) {
        return false;
    }

    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    assert(rng);
    if(dpl_sem_get_count(&rng->sem) == 1) // unsolicited inbound
        return false;

    if (inst->fctrl == UWB_FCTRL_FRAME_TYPE_ACK && !rng->status.rx_ack_expected) {
        return false;
    }
    rng->status.rx_ack_expected = 0;
    rng->status.tx_ack_expected = 0;
    frame = rng->frames[(rng->idx)%rng->nframes]; // Frame already read within loader layers.

    /* Receive the ack response from the other side and store timestamp */
    if (inst->fctrl == UWB_FCTRL_FRAME_TYPE_ACK) {
        /* verify seq no */
        if (inst->rxbuf[2] != (rng->seq_num&0xff)) {
            SS_STATS_INC(ack_seq_err);
            dpl_sem_release(&rng->sem);
            return false;
        }
        rng->ack_rx_timestamp = inst->rxtimestamp;
        uint16_t frame_duration = uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst,sizeof(ieee_rng_response_frame_t)));
        /* Need to set, not just adjust, timeout here */
        uwb_set_rx_timeout(inst, g_config.tx_holdoff_delay + frame_duration + g_config.rx_timeout_delay);
        return true;
    }

    switch(rng->code){
        case UWB_DATA_CODE_SS_TWR_ACK:
            {
                // This code executes on the device that is responding to a request
                if (!inst->status.autoack_triggered) {
                    SS_STATS_INC(ack_tx_err);
                    dpl_sem_release(&rng->sem);
                    return true;
                }

                /* Only prepare response here, replay for real after hw-ack has completed */
                uint64_t request_timestamp = inst->rxtimestamp;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_wcs_instance * wcs = rng->ccp_inst->wcs;
                frame->reception_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, request_timestamp)) & 0xFFFFFFFFULL;
#else
                frame->reception_timestamp = request_timestamp & 0xFFFFFFFFULL;
#endif
                frame->fctrl = FCNTL_IEEE_RANGE_16;
                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_ACK_T1;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0l;
#else
                frame->carrier_integrator  = - inst->carrier_integrator;
#endif

                /* Don't send this here but in the tx_complete_cb for the ack */
                rng->status.tx_ack_expected = 1;
                break;
            }
        case UWB_DATA_CODE_SS_TWR_ACK_T1:
            {
                // This code executes on the device that initiated a request, and is now preparing the final timestamps
                if (inst->frame_len != sizeof(ieee_rng_response_frame_t))
                    break;
                if(inst->status.lde_error)
                    break;

                uint64_t response_timestamp = rng->ack_rx_timestamp;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_wcs_instance * wcs = rng->ccp_inst->wcs;
                frame->request_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, uwb_read_txtime(inst))) & 0xFFFFFFFFULL;
                frame->response_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, response_timestamp)) & 0xFFFFFFFFULL;
#else
                frame->request_timestamp = uwb_read_txtime_lo32(inst) & 0xFFFFFFFFULL;
                frame->response_timestamp = (uint32_t)(response_timestamp & 0xFFFFFFFFULL);
#endif

                if (!rng->ack_rx_timestamp) {
                    /* No ack received, invalidate frame */
                    SS_STATS_INC(ack_rx_err);
                    break;
                }
                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_ACK_FINAL;
                frame->carrier_integrator  = inst->carrier_integrator;

                // Transmit timestamp final report
                uwb_write_tx(inst, frame->array, 0,  sizeof(twr_frame_final_t));
                uwb_write_tx_fctrl(inst, sizeof(twr_frame_final_t), 0);

                if (uwb_start_tx(inst).start_tx_error) {
                    SS_STATS_INC(tx_error);
                    dpl_sem_release(&rng->sem);
                    rng_issue_complete(inst);
                } else {
                    SS_STATS_INC(complete);
                    rng->control.complete_after_tx = 1;
                }
                break;
                break;
            }
        case UWB_DATA_CODE_SS_TWR_ACK_FINAL:
            {
                // This code executes on the device that responded to the original request, and has now receive the response final timestamp.
                // This marks the completion of the single-size-two-way request. This final 4th message is perhaps optional in some applicaiton.

                if (inst->frame_len != sizeof(twr_frame_final_t))
                   break;

                SS_STATS_INC(complete);
                dpl_sem_release(&rng->sem);
                rng_issue_complete(inst);
                break;
            }
        default:
                return false;
                break;
    }

    return true;
}

static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    assert(rng);
    if(dpl_sem_get_count(&rng->sem) == 1) // unsolicited inbound
        return false;

    if (rng->status.tx_ack_expected != 1) {
        return false;
    }
    rng->status.tx_ack_expected = 0;

    twr_frame_t * frame = rng->frames[(rng->idx)%rng->nframes]; // Frame already read in rx_complete_cb

    if (rng->code == UWB_DATA_CODE_SS_TWR_ACK) {
        // This code executes on the device that is responding to a request
        uint64_t response_timestamp = uwb_read_txtime(inst); /* Using tx-time of ack here */
        uint64_t current_timestamp = uwb_read_systime(inst);

#if MYNEWT_VAL(UWB_WCS_ENABLED)
        struct uwb_wcs_instance * wcs = rng->ccp_inst->wcs;
        frame->transmission_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, response_timestamp)) & 0xFFFFFFFFULL;
#else
        frame->transmission_timestamp = response_timestamp & 0xFFFFFFFFULL;
#endif
        // Write the response to the tranceiver
        uwb_write_tx(inst, frame->array ,0 ,sizeof(ieee_rng_response_frame_t));
        uwb_write_tx_fctrl(inst, sizeof(ieee_rng_response_frame_t), 0);
        uwb_set_wait4resp(inst, true);
        uwb_set_wait4resp_delay(inst, 0);
        uint16_t frame_duration = uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst,sizeof(ieee_rng_response_frame_t)));
        uwb_set_rx_timeout(inst, g_config.tx_holdoff_delay + frame_duration + g_config.rx_timeout_delay);

        /* Disable default behavor, do not RXENAB on RXFCG
         * thereby avoiding rx timeout events on success */
        uwb_set_rxauto_disable(inst, true);

        /* Slow down second response in case we have a slow unit on other side */
        if (((0xffffffffffULL&(current_timestamp - response_timestamp))>>16) < MYNEWT_VAL(TWR_SS_ACK_TX_HOLDOFF)/4) {
            uwb_set_delay_start(inst, current_timestamp + (MYNEWT_VAL(TWR_SS_ACK_TX_HOLDOFF)*3/4 << 16));
        }

        if (uwb_start_tx(inst).start_tx_error){
            SS_STATS_INC(tx_error);
            dpl_sem_release(&rng->sem);
        }
    }
    return true;
}
