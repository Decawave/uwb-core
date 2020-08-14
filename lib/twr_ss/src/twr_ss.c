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
 * @file twr_ss.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Range
 *
 * @details This is the rng base class which utilises the functions to enable/disable the configurations related to rng.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
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

static struct uwb_mac_interface g_cbs[] = {
        [0] = {
            .id = UWBEXT_RNG_SS,
            .inst_ptr = 0,
            .rx_complete_cb = rx_complete_cb,
        },
#if MYNEWT_VAL(UWB_DEVICE_1) ||  MYNEWT_VAL(UWB_DEVICE_2)
        [1] = {
            .id = UWBEXT_RNG_SS,
            .inst_ptr = 0,
            .rx_complete_cb = rx_complete_cb,
        },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
        [2] = {
            .id = UWBEXT_RNG_SS,
            .inst_ptr = 0,
            .rx_complete_cb = rx_complete_cb,
        }
#endif
};

#if MYNEWT_VAL(TWR_SS_STATS)
STATS_SECT_START(twr_ss_stat_section)
    STATS_SECT_ENTRY(complete)
    STATS_SECT_ENTRY(tx_error)
STATS_SECT_END

STATS_NAME_START(twr_ss_stat_section)
    STATS_NAME(twr_ss_stat_section, complete)
    STATS_NAME(twr_ss_stat_section, tx_error)
STATS_NAME_END(twr_ss_stat_section)

STATS_SECT_DECL(twr_ss_stat_section) g_twr_ss_stat;
#define SS_STATS_INC(__X) STATS_INC(g_twr_ss_stat, __X)
#else
#define SS_STATS_INC(__X) {}
#endif

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(TWR_SS_TX_HOLDOFF),         // Send Time delay in usec.
    .rx_timeout_delay = MYNEWT_VAL(TWR_SS_RX_TIMEOUT)       // Receive response timeout in usec
};

static struct rng_config_list g_rng_cfgs[] = {
    [0] = {
        .rng_code = UWB_DATA_CODE_SS_TWR,
        .name = "twr_ss",
        .config = &g_config
    },
#if MYNEWT_VAL(UWB_DEVICE_1) ||  MYNEWT_VAL(UWB_DEVICE_2)
    [1] = {
        .rng_code = UWB_DATA_CODE_SS_TWR,
        .name = "twr_ss",
        .config = &g_config
    },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    [2] = {
        .rng_code = UWB_DATA_CODE_SS_TWR,
        .name = "twr_ss",
        .config = &g_config
    },
#endif
};


/**
 * @fn twr_ss_pkg_init(void)
 * @brief API to initialise the rng_ss package.
 *
 * @return void
 */
void
twr_ss_pkg_init(void)
{
    int i, rc;
    struct uwb_dev *udev;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"twr_ss_pkg_init\"}\n",
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

#if MYNEWT_VAL(TWR_SS_STATS)
    rc = stats_init(
        STATS_HDR(g_twr_ss_stat),
        STATS_SIZE_INIT_PARMS(g_twr_ss_stat, STATS_SIZE_32),
        STATS_NAME_INIT_PARMS(twr_ss_stat_section));
    assert(rc == 0);

    rc |= stats_register("twr_ss", STATS_HDR(g_twr_ss_stat));
    assert(rc == 0);
#endif
}

/**
 * @fn twr_ss_free(dw1000_dev_instance_t * inst)
 * @brief API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
twr_ss_free(struct uwb_dev * dev)
{
    assert(dev);
    uwb_mac_remove_interface(dev, UWBEXT_RNG_SS);
}

int
twr_ss_pkg_down(int reason)
{
    int i;
    struct uwb_rng_instance * rng;

    for (i=0;i < sizeof(g_cbs)/sizeof(g_cbs[0]);i++) {
        rng = (struct uwb_rng_instance *)g_cbs[i].inst_ptr;
        if (!rng) continue;
        uwb_rng_remove_config(g_cbs[i].inst_ptr, g_rng_cfgs[i].rng_code);
        twr_ss_free(rng->dev_inst);
    }
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
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    if (inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    assert(rng);
    if(dpl_sem_get_count(&rng->sem) == 1) // unsolicited inbound
        return false;

    frame = rng->frames[(rng->idx)%rng->nframes]; // Frame already read within loader layers.

    switch(rng->code){
        case UWB_DATA_CODE_SS_TWR:
            {
                // This code executes on the device that is responding to a request
                uint64_t request_timestamp = inst->rxtimestamp;
                struct uwb_rng_txd txd;
                uwb_rng_calc_rel_tx(rng, &txd, &g_config, request_timestamp, inst->frame_len);

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_wcs_instance * wcs = rng->ccp_inst->wcs;
                frame->reception_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, request_timestamp)) & 0xFFFFFFFFULL;
                frame->transmission_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, txd.response_timestamp)) & 0xFFFFFFFFULL;
#else
                frame->reception_timestamp = request_timestamp & 0xFFFFFFFFULL;
                frame->transmission_timestamp = txd.response_timestamp & 0xFFFFFFFFULL;
#endif

                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_T1;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0l;
#else
                frame->carrier_integrator  = - inst->carrier_integrator;
#endif
                // Write the second part of the response
                uwb_write_tx(inst, frame->array ,0 ,sizeof(ieee_rng_response_frame_t));
                uwb_write_tx_fctrl(inst, sizeof(ieee_rng_response_frame_t), 0);
                uwb_set_wait4resp(inst, true);
                uwb_set_delay_start(inst, txd.response_tx_delay);

                /* Start tx now, the remaining settings can be done whilst sending anyway */
                if (uwb_start_tx(inst).start_tx_error){
                    SS_STATS_INC(tx_error);
                    dpl_sem_release(&rng->sem);
                }

                /* Setup when to listen for response, relative the end of our transmitted frame */
                uwb_set_wait4resp_delay(inst, g_config.tx_holdoff_delay -
                                        inst->config.rx.timeToRxStable);
                uwb_set_rx_timeout(inst, uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(twr_frame_final_t))) +
                                   g_config.rx_timeout_delay + inst->config.rx.timeToRxStable);

                // Disable default behavor, do not RXENAB on RXFCG thereby avoiding rx timeout events on sucess
                uwb_set_rxauto_disable(inst, true);
                break;
            }
        case UWB_DATA_CODE_SS_TWR_T1:
            {
                // This code executes on the device that initiated a request, and is now preparing the final timestamps
                struct uwb_rng_txd txd;
                uint64_t response_timestamp = inst->rxtimestamp;
                if (inst->frame_len != sizeof(ieee_rng_response_frame_t))
                    break;
                if(inst->status.lde_error)
                    break;

                uwb_rng_calc_rel_tx(rng, &txd, &g_config, response_timestamp, inst->frame_len);

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_wcs_instance * wcs = rng->ccp_inst->wcs;
                frame->request_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, uwb_read_txtime(inst))) & 0xFFFFFFFFULL;
                frame->response_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, response_timestamp)) & 0xFFFFFFFFULL;
#else
                frame->request_timestamp = uwb_read_txtime_lo32(inst) & 0xFFFFFFFFULL;
                frame->response_timestamp = (uint32_t)(response_timestamp & 0xFFFFFFFFULL);
#endif

                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_FINAL;
                frame->carrier_integrator = inst->carrier_integrator;

                // Transmit timestamp final report
                uwb_write_tx(inst, frame->array, 0,  sizeof(twr_frame_final_t));
                uwb_write_tx_fctrl(inst, sizeof(twr_frame_final_t), 0);
                uwb_set_delay_start(inst, txd.response_tx_delay);

                if (uwb_start_tx(inst).start_tx_error) {
                    SS_STATS_INC(tx_error);
                    dpl_sem_release(&rng->sem);
                    rng_issue_complete(inst);
                } else {
                    SS_STATS_INC(complete);
                    rng->control.complete_after_tx = 1;
                }
                break;
            }
        case  UWB_DATA_CODE_SS_TWR_FINAL:
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
