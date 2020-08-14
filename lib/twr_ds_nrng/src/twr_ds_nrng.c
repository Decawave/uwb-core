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
#include <dw1000/dw1000_regs.h>
#include <dw1000/dw1000_dev.h>
#include <dw1000/dw1000_hal.h>
#include <dw1000/dw1000_mac.h>
#include <dw1000/dw1000_phy.h>
#include <uwb/uwb_ftypes.h>
#include <nranges/nranges.h>
#include <dsp/polyval.h>

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static void send_final_msg(struct uwb_dev * inst , nrng_frame_t * frame);

static struct uwb_mac_interface g_cbs = {
            .id = UWBEXT_NRNG_DS,
            .rx_complete_cb = rx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .rx_error_cb = rx_error_cb,
};

#if MYNEWT_VAL(TWR_DS_NRNG_STATS)
STATS_SECT_START(twr_ds_nrng_stat_section)
    STATS_SECT_ENTRY(complete)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(rx_error)
STATS_SECT_END

STATS_NAME_START(twr_ds_nrng_stat_section)
    STATS_NAME(twr_ds_nrng_stat_section, complete)
    STATS_NAME(twr_ds_nrng_stat_section, rx_timeout)
    STATS_NAME(twr_ds_nrng_stat_section, rx_error)
STATS_NAME_END(twr_ds_nrng_stat_section)

static STATS_SECT_DECL(twr_ds_nrng_stat_section) g_stat;
#define DS_STATS_INC(__X) STATS_INC(rng->stat, __X)
#else
#define DS_STATS_INC(__X) {}
#endif

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(TWR_DS_NRNG_TX_HOLDOFF),         // Send Time delay in usec.
    .rx_timeout_period = MYNEWT_VAL(TWR_DS_NRNG_RX_TIMEOUT),        // Receive response timeout in usec
    .tx_guard_delay = MYNEWT_VAL(TWR_DS_NRNG_TX_GUARD_DELAY)        // Guard delay to be added between each frame from node
};

static struct rng_config_list g_rng_cfgs = {
    .rng_code = UWB_DATA_CODE_DS_TWR_NRNG,
    .config = &g_config
};

/**
 * API to initialise the rng_ss package.
 *
 *
 * @return void
 */

void twr_ds_nrng_pkg_init(void){

    printf("{\"utime\": %lu,\"msg\": \"twr_ds_nrng_pkg_init\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));
    struct nrng_instance *nrng = (struct nrng_instance*)uwb_mac_find_cb_inst_ptr(uwb_dev_idx_lookup(0), UWBEXT_NRNG);
    g_cbs.inst_ptr = nrng;
    uwb_mac_append_interface(uwb_dev_idx_lookup(0), &g_cbs);
    nrng_append_config(nrng, &g_rng_cfgs);

#if MYNEWT_VAL(TWR_DS_NRNG_STATS)
    int rc = stats_init(
    STATS_HDR(g_stat),
    STATS_SIZE_INIT_PARMS(g_stat, STATS_SIZE_32),
    STATS_NAME_INIT_PARMS(twr_ds_nrng_stat_section));
    assert(rc == 0);

    rc = stats_register("twr_ds_nrng", STATS_HDR(g_stat));
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
twr_ds_nrng_free(struct uwb_dev * inst){
    assert(inst);
    uwb_mac_remove_interface(inst, UWBEXT_NRNG_DS);
}


/**
 * API for receive timeout callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs){
    if(inst->fctrl != FCNTL_IEEE_N_RANGES_16){
        return false;
    }
    DS_STATS_INC(rx_timeout);
    switch(inst->nrng->code){
        case UWB_DATA_CODE_DS_TWR_NRNG ... UWB_DATA_CODE_DS_TWR_NRNG_FINAL:
        {
          assert(inst->nrng);
          struct nrng_instance * nrng = inst->nrng;
          if(nrng->device_type == DWT_NRNG_INITIATOR){ // only if the device is an initiator
              if(nrng->resp_count && nrng->t1_final_flag){
                  nrng_frame_t * frame = nrng->frames[nrng->idx][SECOND_FRAME_IDX];
                  send_final_msg(inst,frame);
              }else{
                  if(os_sem_get_count(&nrng->sem) == 0){
                      os_error_t err = os_sem_release(&nrng->sem);
                      assert(err == OS_OK);
                      nrng->resp_count = 0;
                  }
              }
          }else{
              if(os_sem_get_count(&nrng->sem) == 0){
                  os_error_t err = os_sem_release(&nrng->sem);
                  assert(err == OS_OK);
              }
          }
            break;
      }
      default:
        return false;
    }
    return true;
}

/**
 * API for receive error callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs){
    /* Place holder */
    if(inst->fctrl != FCNTL_IEEE_N_RANGES_16){
        return false;
    }
    DS_STATS_INC(rx_error);
    assert(inst->nrng);
    struct nrng_instance * nrng = inst->nrng;
    if(os_sem_get_count(&nrng->sem) == 0){
        os_error_t err = os_sem_release(&nrng->sem);
        assert(err == OS_OK);
    }
    return true;
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
    if(inst->fctrl != FCNTL_IEEE_N_RANGES_16){
        return false;
    }

    if(os_sem_get_count(&inst->nrng->sem) == 1){
        // unsolicited inbound
        return false;
    }

    assert(inst->nrng);
    struct nrng_instance * nrng = inst->nrng;
    struct uwb_rng_config * config = nrng_get_config(inst, UWB_DATA_CODE_DS_TWR_NRNG);
    switch(inst->nrng->code){
        case UWB_DATA_CODE_DS_TWR_NRNG:
            {
                // This code executes on the device that is responding to a original request

                nrng_frame_t * frame = nrng->frames[(++nrng->idx)%(nrng->nframes/FRAMES_PER_RANGE)][FIRST_FRAME_IDX];
                uint16_t slot_id = inst->slot_id;
                if (inst->frame_len >= sizeof(nrng_request_frame_t))
                    dw1000_read_rx(inst, frame->array, 0, sizeof(nrng_request_frame_t));
                else
                    break;
                if(!(slot_id >= frame->start_slot_id && slot_id <= frame->end_slot_id))
                    break;

                uint64_t request_timestamp = dw1000_read_rxtime(inst);
                uint64_t response_tx_delay = request_timestamp + (((uint64_t)config->tx_holdoff_delay
                            + (uint64_t)((slot_id-1) * ((uint64_t)config->tx_guard_delay
                            + (uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(nrng_response_frame_t)))))))<< 16);

                uint64_t response_timestamp = (response_tx_delay & 0xFFFFFFFE00UL) + inst->tx_antenna_delay;

                frame->reception_timestamp =  request_timestamp;
                frame->transmission_timestamp =  response_timestamp;

                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_DS_TWR_NRNG_T1;
                frame->slot_id = inst->slot_id;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = -dw1000_read_carrier_integrator(inst);
#endif
                uwb_write_tx(inst, frame->array, 0, sizeof(nrng_response_frame_t));
                uwb_write_tx_fctrl(inst, sizeof(nrng_response_frame_t), 0);
                uwb_set_wait4resp(inst, true);
                uint16_t timeout =  config->tx_holdoff_delay + (uint16_t)(frame->end_slot_id - slot_id + 1) * (uwb_phy_frame_duration(inst, sizeof(nrng_response_frame_t))
                                    + config->rx_timeout_period
                                    + config->tx_guard_delay);

                uwb_set_rx_timeout(inst, timeout);
                uwb_set_delay_start(inst, response_tx_delay);
                if (uwb_start_tx(inst).start_tx_error){
                    os_sem_release(&nrng->sem);
                    if (cbs!=NULL && cbs->start_tx_error_cb)
                        cbs->start_tx_error_cb(inst, cbs);
                }
                break;
            }
        case UWB_DATA_CODE_DS_TWR_NRNG_T1:
            {
                // This code executes on the device that initiated the original request, and is now preparing the next series of timestamps
                // The 1st frame now contains a local copy of the initial first side of the double sided scheme.
                // printf("T1 \n");
                nrng->resp_count++;
                uint16_t nnodes = nrng->nnodes;
                uint16_t idx = 0;
                nrng_frame_t temp_frame;
                if (inst->frame_len >= sizeof(nrng_response_frame_t))
                    dw1000_read_rx(inst, temp_frame.array, 0, sizeof(nrng_response_frame_t));
                else
                    break;
                uint16_t node_slot_id = temp_frame.slot_id;
                uint16_t end_slot_id = temp_frame.end_slot_id;
                nrng->idx = idx = node_slot_id - temp_frame.start_slot_id;

                if(idx < (nnodes-1)){
                    // At the start the device will wait for the entire nnodes to respond as a single huge timeout.
                    // When a node respond we will recalculate the remaining time to be waited for as (total_nodes - completed_nodes)*(phy_duaration + guard_delay)
                    uint16_t phy_duration = uwb_phy_frame_duration(inst, sizeof(nrng_response_frame_t));
                    uint16_t timeout = ((phy_duration + dw1000_dwt_usecs_to_usecs(config->tx_guard_delay)) * (end_slot_id - node_slot_id));
                    uwb_set_rx_timeout(inst, timeout);
                    if (inst->config.dblbuffon_enabled == 0 || inst->config.rxauto_enable == 0)
                        uwb_start_rx(inst);
                }else{
                    if (inst->config.dblbuffon_enabled)
                        uwb_stop_rx(inst);
                }

                nrng_frame_t * frame = nrng->frames[idx][FIRST_FRAME_IDX];
                nrng_frame_t * next_frame = nrng->frames[idx][SECOND_FRAME_IDX];
                memcpy(frame, &temp_frame, sizeof(nrng_response_frame_t));

                frame->request_timestamp = next_frame->request_timestamp = dw1000_read_txtime_lo(inst);    // This corresponds to when the original request was actually sent
                frame->response_timestamp = next_frame->response_timestamp = dw1000_read_rxtime_lo(inst);  // This corresponds to the response just received

                uint8_t seq_num = frame->seq_num;
                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_DS_TWR_NRNG_T2;
                frame->seq_num = seq_num + 1;
                // Note:: Advance to next frame
                frame = next_frame;
                frame->dst_address = 0xffff;
                frame->src_address = inst->my_short_address;
                frame->seq_num = seq_num + 1;
                frame->code = UWB_DATA_CODE_DS_TWR_NRNG_T2;
                frame->start_slot_id = temp_frame.start_slot_id;
                frame->end_slot_id = temp_frame.end_slot_id;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = dw1000_read_carrier_integrator(inst);
#endif
                uint64_t request_timestamp = dw1000_read_rxtime(inst);
                uint64_t response_timestamp = (request_timestamp & 0xFFFFFFFE00UL) + inst->tx_antenna_delay;
                frame->reception_timestamp = request_timestamp;
                frame->transmission_timestamp = response_timestamp;
                nrng->t1_final_flag = 1;
                if(idx == (nnodes - 1))
                {
                    uint16_t timeout = (((nnodes) * (uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(nrng_final_frame_t))))
                                + (nnodes - 1)*(config->tx_guard_delay))
                            + config->tx_holdoff_delay         // Remote side turn arroud time.
                            + config->rx_timeout_period);
                    uwb_set_rx_timeout(inst, timeout);

                    uwb_write_tx(inst, frame->array, 0, sizeof(nrng_request_frame_t));
                    uwb_write_tx_fctrl(inst, sizeof(nrng_request_frame_t), 0);
                    uwb_set_wait4resp(inst, true);
                    if (uwb_start_tx(inst).start_tx_error){
                        os_sem_release(&nrng->sem);
                        if (cbs!=NULL && cbs->start_tx_error_cb)
                            cbs->start_tx_error_cb(inst, cbs);
                    }
                    nrng->resp_count = 0;
                    nrng->t1_final_flag = 0;
                }
                break;
            }
        case UWB_DATA_CODE_DS_TWR_NRNG_T2:
            {
                // This code executes on the device that responded to the original request, and is now preparing the final timestamps
                // printf("T2\n");
                uint16_t slot_id = inst->slot_id;
                nrng_frame_t * frame = nrng->frames[(++nrng->idx)%(nrng->nframes/FRAMES_PER_RANGE)][SECOND_FRAME_IDX];

                if (inst->frame_len >= sizeof(nrng_request_frame_t))
                    dw1000_read_rx(inst,  frame->array, 0, sizeof(nrng_request_frame_t));
                else
                    break;
                if(!(slot_id >= frame->start_slot_id && slot_id <= frame->end_slot_id))
                    break;
                uint64_t request_timestamp = dw1000_read_rxtime(inst);
                uint64_t response_tx_delay = request_timestamp + (((uint64_t)config->tx_holdoff_delay
                            + (uint64_t)((inst->slot_id-1) * ((uint64_t)config->tx_guard_delay
                                    + uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(nrng_final_frame_t))))))<< 16);
                frame->request_timestamp = dw1000_read_txtime_lo(inst); // This corresponds to when the original request was actually sent
                frame->response_timestamp = dw1000_read_rxtime_lo(inst);  // This corresponds to the response just received
                frame->dst_address = frame->src_address;
                frame->src_address = inst->my_short_address;
                frame->code = UWB_DATA_CODE_DS_TWR_NRNG_FINAL;
                frame->slot_id = slot_id;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
                frame->carrier_integrator  = 0.0l;
#else
                frame->carrier_integrator  = -dw1000_read_carrier_integrator(inst);
#endif
                uwb_write_tx(inst, frame->array, 0, sizeof(nrng_final_frame_t));
                uwb_write_tx_fctrl(inst, sizeof(nrng_final_frame_t), 0);
                uwb_set_delay_start(inst, response_tx_delay);
                if (uwb_start_tx(inst).start_tx_error){
                    os_sem_release(&nrng->sem);
                    if (cbs!=NULL && cbs->start_tx_error_cb)
                        cbs->start_tx_error_cb(inst, cbs);
                }else{
                    DS_STATS_INC(g_stat, complete);
                    os_sem_release(&nrng->sem);
                    struct uwb_mac_interface * cbs = NULL;
                    if(!(SLIST_EMPTY(&inst->interface_cbs))){
                        SLIST_FOREACH(cbs, &inst->interface_cbs, next){
                            if (cbs!=NULL && cbs->complete_cb)
                                if(cbs->complete_cb(inst, cbs)) continue;
                        }
                    }
                }
                break;
            }
        case  UWB_DATA_CODE_DS_TWR_NRNG_FINAL:
            {
                // This code executes on the device that initialed the original request, and has now receive the final response timestamp.
                // This marks the completion of the double-single-two-way request.
                //printf("Final\n");
                nrng->resp_count++;
                uint16_t nnodes = nrng->nnodes;
                uint16_t idx = 0;
                nrng_frame_t temp;
                if (inst->frame_len >= sizeof(nrng_final_frame_t))
                    dw1000_read_rx(inst, (uint8_t *)&temp, 0, sizeof(nrng_final_frame_t));

                uint16_t node_slot_id = temp.slot_id;
                uint16_t end_slot_id = temp.end_slot_id;
                nrng->idx = idx = node_slot_id - temp.start_slot_id;
                if(idx < nnodes-1){
                    // At the start the device will wait for the entire nnodes to respond as a single huge timeout.
                    // When a node respond we will recalculate the remaining time to be waited for as (total_nodes - completed_nodes)*(phy_duaration + guard_delay)
                    uint16_t phy_duration = uwb_phy_frame_duration(inst, sizeof(nrng_final_frame_t));
                    uint16_t timeout = ((phy_duration + config->tx_guard_delay) * (end_slot_id - node_slot_id));
                    uwb_set_rx_timeout(inst, timeout);
                    if (inst->config.dblbuffon_enabled == 0 || inst->config.rxauto_enable == 0)
                        uwb_start_rx(inst);
                }else{
                     if (inst->config.dblbuffon_enabled)
                        uwb_stop_rx(inst);
                }
                nrng_frame_t *frame = nrng->frames[idx][SECOND_FRAME_IDX];

                nrng->t1_final_flag = 0;
                frame->request_timestamp = temp.request_timestamp;
                frame->response_timestamp = temp.response_timestamp;
                frame->code = temp.code;
                frame->dst_address = temp.src_address;
                frame->transmission_timestamp = dw1000_read_txtime_lo(inst);
                if(idx == nnodes -1)
                {
                    DS_STATS_INC(g_stat, complete);
                    os_sem_release(&nrng->sem);
                    struct uwb_mac_interface * cbs = NULL;
                    if(!(SLIST_EMPTY(&inst->interface_cbs))){
                        SLIST_FOREACH(cbs, &inst->interface_cbs, next){
                            if (cbs!=NULL && cbs->complete_cb)
                                if(cbs->complete_cb(inst, cbs)) continue;
                        }
                    }
                    nrng->resp_count = 0;
                }

                break;
            }
        default:
            return false;
            break;
    }
    return true;
}

static void
send_final_msg(struct uwb_dev * inst , nrng_frame_t * frame)
{
    //printf("final_cb\n");
    assert(inst->nrng);
    struct nrng_instance * nrng = inst->nrng;
    struct uwb_rng_config * config = nrng_get_config(inst, UWB_DATA_CODE_DS_TWR_NRNG);
    uint16_t nnodes = nrng->nnodes;
    uwb_write_tx(inst, frame->array, 0, sizeof(nrng_request_frame_t));
    uwb_write_tx_fctrl(inst, sizeof(nrng_request_frame_t), 0);
    uwb_set_wait4resp(inst, true);
    uint16_t timeout = (((nnodes) * (uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(nrng_final_frame_t))))
            + (nnodes - 1)*(config->tx_guard_delay))
            + config->tx_holdoff_delay         // Remote side turn arroud time.
            + config->rx_timeout_period);
    uwb_set_rx_timeout(inst, timeout);

    nrng->resp_count = 0;
    nrng->t1_final_flag = 0;

    if (uwb_start_tx(inst).start_tx_error)
        os_sem_release(&nrng->sem);
}
