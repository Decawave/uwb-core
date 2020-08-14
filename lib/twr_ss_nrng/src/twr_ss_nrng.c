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
#include <os/os.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>

#include <stats/stats.h>
#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwb/uwb_ftypes.h>
#include <nrng/nrng.h>
#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif
#include <dsp/polyval.h>
#include <uwb_rng/slots.h>

#define WCS_DTU MYNEWT_VAL(WCS_DTU)

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

#if MYNEWT_VAL(TWR_SS_NRNG_STATS)
STATS_SECT_START(twr_ss_nrng_stat_section)
    STATS_SECT_ENTRY(rx_error)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(rx_complete)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(reset)
STATS_SECT_END

STATS_NAME_START(twr_ss_nrng_stat_section)
    STATS_NAME(twr_ss_nrng_stat_section, rx_error)
    STATS_NAME(twr_ss_nrng_stat_section, rx_timeout)
    STATS_NAME(twr_ss_nrng_stat_section, rx_complete)
    STATS_NAME(twr_ss_nrng_stat_section, rx_unsolicited)
    STATS_NAME(twr_ss_nrng_stat_section, reset)
STATS_NAME_END(twr_ss_nrng_stat_section)

STATS_SECT_DECL(twr_ss_nrng_stat_section) g_twr_ss_nrng_stat;
#define SS_STATS_INC(__X) STATS_INC(g_twr_ss_nrng_stat, __X)
#else
#define SS_STATS_INC(__X) {}
#endif


static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

static struct  uwb_mac_interface g_cbs = {
    .id = UWBEXT_NRNG_SS,
    .rx_complete_cb = rx_complete_cb,
    .rx_timeout_cb = rx_timeout_cb,
    .rx_error_cb = rx_error_cb,
    .reset_cb = reset_cb
};

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(TWR_SS_NRNG_TX_HOLDOFF),         // Send Time delay in usec.
    .rx_timeout_delay = MYNEWT_VAL(TWR_SS_NRNG_RX_TIMEOUT),        // Receive response timeout in usec
    .tx_guard_delay = MYNEWT_VAL(TWR_SS_NRNG_TX_GUARD_DELAY)        // Guard delay to be added between each frame from node
};

static struct rng_config_list g_rng_cfgs = {
    .rng_code = UWB_DATA_CODE_SS_TWR_NRNG,
    .config = &g_config
};

/**
 * API to initialise the rng_ss package.
 *
 *
 * @return void
 */
void twr_ss_nrng_pkg_init(void)
{
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %lu,\"msg\": \"ss_nrng_pkg_init\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
#endif
    struct uwb_dev *udev;
    udev = uwb_dev_idx_lookup(0);
    struct nrng_instance *nrng = (struct nrng_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_NRNG);
    g_cbs.inst_ptr = nrng;
    uwb_mac_append_interface(udev, &g_cbs);
    nrng_append_config(nrng, &g_rng_cfgs);

#if MYNEWT_VAL(TWR_SS_NRNG_STATS)
    int rc = stats_init(
    STATS_HDR(g_twr_ss_nrng_stat),
    STATS_SIZE_INIT_PARMS(g_twr_ss_nrng_stat, STATS_SIZE_32),
    STATS_NAME_INIT_PARMS(twr_ss_nrng_stat_section));
    assert(rc == 0);

    rc = stats_register("twr_ss_nrng", STATS_HDR(g_twr_ss_nrng_stat));
    assert(rc == 0);
#endif
}


/**
 * API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
twr_ss_nrng_free(struct uwb_dev * inst)
{
    assert(inst);
    uwb_mac_remove_interface(inst, UWBEXT_NRNG_SS);
}


/**
 * API for receive error callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct nrng_instance * nrng = (struct nrng_instance *)cbs->inst_ptr;
    if(inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(dpl_sem_get_count(&nrng->sem) == 0){
        SS_STATS_INC(rx_error);
        dpl_error_t err = dpl_sem_release(&nrng->sem);
        assert(err == DPL_OK);
        return true;
    }
    return false;
}


/**
 * API for receive timeout callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct nrng_instance * nrng = (struct nrng_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&nrng->sem) == 1)
        return false;

    if(dpl_sem_get_count(&nrng->sem) == 0){
        SS_STATS_INC(rx_timeout);
        // In the case of a NRNG timeout is used to mark the end of the request
        // and is used to call the completion callback
        if(!(SLIST_EMPTY(&inst->interface_cbs))){
            SLIST_FOREACH(cbs, &inst->interface_cbs, next){
            if (cbs!=NULL && cbs->complete_cb)
                if(cbs->complete_cb(inst, cbs)) continue;
            }
        }
        dpl_error_t err = dpl_sem_release(&nrng->sem);
        assert(err == DPL_OK);
    }
    return true;
}


/**
 * API for reset_cb of nrng interface
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return true on sucess
 */
static bool
reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct nrng_instance * nrng = (struct nrng_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&nrng->sem) == 0){
        dpl_error_t err = dpl_sem_release(&nrng->sem);
        assert(err == DPL_OK);
        SS_STATS_INC(reset);
        return true;
    }
    else
        return false;
}

/**
 * API for receive complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct nrng_instance * nrng = (struct nrng_instance *)cbs->inst_ptr;

    if(inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(dpl_sem_get_count(&nrng->sem) == 1){
        // unsolicited inbound
        SS_STATS_INC(rx_unsolicited);
        return false;
    }

    struct uwb_rng_config * config = nrng_get_config(nrng, UWB_DATA_CODE_SS_TWR_NRNG);
    nrng_request_frame_t * _frame = (nrng_request_frame_t * )inst->rxbuf;

    if (_frame->dst_address != inst->my_short_address && _frame->dst_address != UWB_BROADCAST_ADDRESS)
        return true;

    SS_STATS_INC(rx_complete);

    switch(_frame->code){
        case UWB_DATA_CODE_SS_TWR_NRNG:
            {
                // This code executes on the device that is responding to a request
                DIAGMSG("{\"utime\": %lu,\"msg\": \"UWB_DATA_CODE_SS_TWR_NRNG\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
                if (inst->frame_len < sizeof(nrng_request_frame_t))
                    break;
                uint16_t slot_idx;
#if MYNEWT_VAL(CELL_ENABLED)
                if (_frame->ptype != PTYPE_CELL)
                    break;
                if (_frame->cell_id != inst->cell_id)
                    break;
                if (_frame->slot_mask & (1UL << inst->slot_id))
                    slot_idx = BitIndex(_frame->slot_mask, 1UL << inst->slot_id, SLOT_POSITION);
                else
                    break;
#else
                if (_frame->bitfield & (1UL << inst->slot_id))
                    slot_idx = BitIndex(_frame->bitfield, 1UL << inst->slot_id, SLOT_POSITION);
                else
                    break;
#endif
                nrng_final_frame_t * frame = (nrng_final_frame_t *) nrng->frames[(++nrng->idx)%nrng->nframes];
                memcpy(frame->array, inst->rxbuf, sizeof(nrng_request_frame_t));

                uint64_t request_timestamp = inst->rxtimestamp;
                uint64_t response_tx_delay = request_timestamp
                            + (((uint64_t)config->tx_holdoff_delay
                            + (uint64_t)(slot_idx * ((uint64_t)config->tx_guard_delay
                            + (uint64_t)(uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(nrng_response_frame_t)))))))<< 16);
                uint64_t response_timestamp = (response_tx_delay & 0xFFFFFFFE00UL) + inst->tx_antenna_delay;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
                struct uwb_wcs_instance * wcs = ccp->wcs;
                frame->reception_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, request_timestamp)) & 0xFFFFFFFFULL;
                frame->transmission_timestamp = (uint32_t)(uwb_wcs_local_to_master(wcs, response_timestamp)) & 0xFFFFFFFFULL;
#else
                frame->reception_timestamp = request_timestamp & 0xFFFFFFFFULL;
                frame->transmission_timestamp = response_timestamp & 0xFFFFFFFFULL;
#endif
                frame->dst_address = _frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_NRNG_T1;
                frame->slot_id = slot_idx;
                frame->seq_num = _frame->seq_num;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = - inst->carrier_integrator;
#endif
                uwb_write_tx(inst, frame->array, 0, sizeof(nrng_response_frame_t));
                uwb_write_tx_fctrl(inst, sizeof(nrng_response_frame_t), 0);
                uwb_set_wait4resp(inst, false);
                uwb_set_delay_start(inst, response_tx_delay);

                if (uwb_start_tx(inst).start_tx_error){
                    dpl_sem_release(&nrng->sem);
                }else{
                    dpl_sem_release(&nrng->sem);
                }
            break;
            }
        case UWB_DATA_CODE_SS_TWR_NRNG_T1:
            {
                // This code executes on the device that initiated a request, and is now preparing the final timestamps
                DIAGMSG("{\"utime\": %lu,\"msg\": \"UWB_DATA_CODE_SS_TWR_NRNG_T1\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
                if (inst->frame_len < sizeof(nrng_response_frame_t))
                    break;

                nrng_response_frame_t * _frame = (nrng_response_frame_t * )inst->rxbuf;
                uint16_t idx = _frame->slot_id;

                // Reject out of sequence ranges, this should never occur in a well behaved system
                if (nrng->seq_num != _frame->seq_num)
                    break;

                nrng_frame_t * frame = nrng->frames[(nrng->idx + idx)%nrng->nframes];
                memcpy(frame, inst->rxbuf, sizeof(nrng_response_frame_t));

                uint64_t response_timestamp = 0x0;
                if (inst->status.lde_error == 0)
                   response_timestamp = inst->rxtimestamp;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
                struct uwb_wcs_instance * wcs = ccp->wcs;
                frame->request_timestamp = uwb_wcs_local_to_master(wcs, uwb_read_txtime(inst)) & 0xFFFFFFFFULL;
                frame->response_timestamp = uwb_wcs_local_to_master(wcs, response_timestamp) & 0xFFFFFFFFULL;
#else
                frame->request_timestamp = uwb_read_txtime_lo32(inst) & 0xFFFFFFFFUL;
                frame->response_timestamp  = (uint32_t)(response_timestamp & 0xFFFFFFFFULL);
#endif
                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_NRNG_FINAL;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = inst->carrier_integrator;
#endif
                if(inst->config.rxdiag_enable) {
                    memcpy(&frame->diag, inst->rxdiag, inst->rxdiag->rxd_len);
                }
                if(idx == nrng->nnodes-1){
                     uwb_set_rx_timeout(inst, 1); // Triger timeout event
                }else{
                    // Incrementally reduce the remaining timeout calculation in accordance with what is still to come.
                    uint16_t timeout = usecs_to_response(inst,
                                nrng->nnodes - idx,                // no. of remaining frames
                                config,                            // Guard delay
                                uwb_phy_frame_duration(inst, sizeof(nrng_response_frame_t)) // frame duration in usec
                            ) + config->rx_timeout_delay;          // TOF allowance.
                    uwb_set_rx_timeout(inst, timeout);
                }
            break;
            }
        default:
                return false;
            break;
        }
    return true;
}
