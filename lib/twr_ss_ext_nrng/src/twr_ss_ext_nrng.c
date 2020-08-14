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
 * @file twr_ss_ext.c
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
#include "bsp/bsp.h"

#include <dw1000/dw1000_regs.h>
#include <dw1000/dw1000_dev.h>
#include <dw1000/dw1000_hal.h>
#include <dw1000/dw1000_mac.h>
#include <dw1000/dw1000_phy.h>
#include <uwb/uwb_ftypes.h>
#include <rng/nrng.h>
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

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_final_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbscbs);

static struct uwb_mac_interface g_cbs = {
            .id = UWBEXT_NRNG_SS_EXT,
            .rx_complete_cb = rx_complete_cb,
            .rx_error_cb = rx_error_cb,
            .final_cb = tx_final_cb,
};

#if MYNEWT_VAL(TWR_SS_EXT_NRNG_STATS)
STATS_SECT_START(twr_ss_ext_nrng_stat_section)
    STATS_SECT_ENTRY(complete)
    STATS_SECT_ENTRY(rx_error)
    STATS_SECT_ENTRY(rx_timeout)
STATS_SECT_END

STATS_NAME_START(twr_ss_ext_nrng_stat_section)
    STATS_NAME(twr_ss_ext_nrng_stat_section, complete)
    STATS_NAME(twr_ss_ext_nrng_stat_section, rx_error)
    STATS_NAME(twr_ss_ext_nrng_stat_section, rx_timeout)
STATS_NAME_END(twr_ss_ext_nrng_stat_section)

static STATS_SECT_DECL(twr_ss_ext_nrng_stat_section) g_stat;
#define SS_STATS_INC(__X) STATS_INC(g_stat, __X)
#else
#define SS_STATS_INC(__X) {}
#endif

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(TWR_SS_EXT_NRNG_TX_HOLDOFF),         // Send Time delay in usec.
    .rx_timeout_delay = MYNEWT_VAL(TWR_SS_EXT_NRNG_RX_TIMEOUT),        // Receive response timeout in usec
    .tx_guard_delay = MYNEWT_VAL(TWR_SS_EXT_NRNG_TX_GUARD_DELAY)        // Guard delay to be added between each frame from node
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

void twr_ss_ext_nrng_pkg_init(void)
{
    printf("{\"utime\": %lu,\"msg\": \"ss_ext_nrng_pkg_init\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
    struct nrng_instance *nrng = (struct nrng_instance*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_NRNG);
    g_cbs.inst_ptr = nrng;
    uwb_mac_append_interface(uwb_dev_idx_lookup(0), &g_cbs);
    nrng_append_config(nrng, &g_rng_cfgs);

#if MYNEWT_VAL(TWR_SS_EXT_NRNG_STATS)
    int rc = stats_init(
    STATS_HDR(g_stat),
    STATS_SIZE_INIT_PARMS(g_stat, STATS_SIZE_32),
    STATS_NAME_INIT_PARMS(twr_ss_ext_nrng_stat_section));
    rc |= stats_register("ss_ext_nrng", STATS_HDR(g_stat));
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
twr_ss_ext_nrng_free(struct uwb_dev * inst)
{
    assert(inst);
    uwb_mac_remove_interface(inst, UWBEXT_NRNG_SS_EXT);
}

/**
 * API for receive error callback.
 *
 * @param inst  Pointer to dw1000_dev_instance_t.
 *
 * @return true on sucess
 */
static bool
rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs){
    /* Place holder */
    if(inst->fctrl != FCNTL_IEEE_RANGE_16){
        return false;
    }
    struct uwb_rng_instance * rng = inst->rng;
    if(os_sem_get_count(&rng->sem) == 0){
        SS_STATS_INC(rx_error);
        os_error_t err = os_sem_release(&rng->sem);
        assert(err == OS_OK);
        return true;
    }
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
    if (inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if (os_sem_get_count(&inst->rng->sem) == 1) // unsolicited inbound
	    return false;
    assert(inst->nrng);
    struct uwb_rng_instance * rng = inst->rng;
    struct nrng_instance * nrng = inst->nrng;
    struct uwb_rng_config * config = nrng_get_config(inst, UWB_DATA_CODE_SS_TWR_NRNG_EXT);
    uint16_t slot_idx;

    switch(rng->code){
        case UWB_DATA_CODE_SS_TWR_NRNG_EXT:
            {
                // This code executes on the device that is responding to a request
                DIAGMSG("{\"utime\": %lu,\"msg\": \"UWB_DATA_CODE_SS_TWR_NRNG_EXT\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
                if (inst->frame_len != sizeof(nrng_request_frame_t))
                    break;

                nrng_request_frame_t * _frame = (nrng_request_frame_t * )inst->rxbuf;

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
                nrng_frame_t * frame = (nrng_frame_t *) nrng->frames[(++nrng->idx)%(nrng->nframes/FRAMES_PER_RANGE)][FIRST_FRAME_IDX];
                memcpy(frame->array, inst->rxbuf, sizeof(nrng_frame_t));

                uint64_t request_timestamp = inst->rxtimestamp;
                uint64_t response_tx_delay = request_timestamp
                            + (((uint64_t)config->tx_holdoff_delay
                            + (uint64_t)(slot_idx * ((uint64_t)config->tx_guard_delay
                            + (uint64_t)(uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(nrng_frame_t)))))))<< 16);
                uint64_t response_timestamp = (response_tx_delay & 0xFFFFFFFE00UL) + inst->tx_antenna_delay;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_wcs_instance * wcs = inst->ccp->wcs;
                frame->reception_timestamp = (uint32_t)(wcs_local_to_master(wcs, request_timestamp)) & 0xFFFFFFFFULL;
                frame->transmission_timestamp = (uint32_t)(wcs_local_to_master(wcs, response_timestamp)) & 0xFFFFFFFFULL;
#else
                frame->reception_timestamp = request_timestamp & 0xFFFFFFFFULL;
                frame->transmission_timestamp = response_timestamp & 0xFFFFFFFFULL;
#endif
                frame->dst_address = _frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_NRNG_EXT_T1;
                frame->slot_id = slot_idx;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = - inst->carrier_integrator;
#endif
                // Final callback, prior to transmission, use this callback to populate the EXTENDED_FRAME fields.
                if (cbs!=NULL && cbs->final_cb)
                    cbs->final_cb(inst, cbs);

                uwb_write_tx(inst, frame->array, 0, sizeof(nrng_frame_t));
                uwb_write_tx_fctrl(inst, sizeof(nrng_frame_t), 0);
                uwb_set_wait4resp(inst, false);
                uwb_set_delay_start(inst, response_tx_delay);

                if (uwb_start_tx(inst).start_tx_error){
                    os_sem_release(&rng->sem);
                    if (cbs!=NULL && cbs->start_tx_error_cb)
                        cbs->start_tx_error_cb(inst, cbs);
                }else{
                    os_sem_release(&rng->sem);
                }
            break;
            }

        case UWB_DATA_CODE_SS_TWR_NRNG_EXT_T1:
            {
                // This code executes on the device that initiated a request, and is now preparing the final timestamps
                DIAGMSG("{\"utime\": %lu,\"msg\": \"UWB_DATA_CODE_SS_TWR_NRNG_EXT_T1\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
                if (inst->frame_len != sizeof(nrng_frame_t))
                    break;

                nrng_frame_t * _frame = (nrng_frame_t * )inst->rxbuf;
                uint16_t idx = _frame->slot_id;

                // Reject out of sequence ranges, this should never occur in a well behaved system
                if (inst->nrng->seq_num != _frame->seq_num)
                    break;

                if(idx < nrng->nnodes && inst->config.rxauto_enable == 0)
                    uwb_start_rx(inst);
                nrng_frame_t * frame = nrng->frames[(nrng->idx + idx)%(nrng->nframes/FRAMES_PER_RANGE)][FIRST_FRAME_IDX];
                memcpy(frame, inst->rxbuf, sizeof(nrng_frame_t));

                uint64_t response_timestamp = 0x0;
                if (inst->status.lde_error == 0)
                   response_timestamp = inst->rxtimestamp;

#if MYNEWT_VAL(UWB_WCS_ENABLED)
                struct uwb_wcs_instance * wcs = inst->ccp->wcs;
                frame->request_timestamp = wcs_local_to_master(wcs, dw1000_read_txtime(inst)) & 0xFFFFFFFFULL;
                frame->response_timestamp = wcs_local_to_master(wcs, response_timestamp) & 0xFFFFFFFFULL;
#else
                frame->request_timestamp = dw1000_read_txtime_lo(inst) & 0xFFFFFFFFUL;
                frame->response_timestamp  = (uint32_t)(response_timestamp & 0xFFFFFFFFULL);
#endif
                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_SS_TWR_NRNG_EXT_FINAL;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = inst->carrier_integrator;
#endif
            break;
            }
        default:
                return false;
            break;
    }
    return true;
}

static bool
tx_final_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbscbs){
    struct nrng_instance * nrng = inst->nrng;
    nrng_frame_t * frame = nrng->frames[nrng->idx%(nrng->nframes/FRAMES_PER_RANGE)][FIRST_FRAME_IDX];

    frame->cartesian.x = MYNEWT_VAL(LOCAL_COORDINATE_X);
    frame->cartesian.y = MYNEWT_VAL(LOCAL_COORDINATE_Y);
    frame->cartesian.z = MYNEWT_VAL(LOCAL_COORDINATE_Z);

    frame->spherical_variance.range = MYNEWT_VAL(RANGE_VARIANCE);
    frame->spherical_variance.azimuth = -1;
    frame->spherical_variance.zenith = -1;
    return true;
}
