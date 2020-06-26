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
 * @file survey.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 02/2019
 *
 * @brief automatic site survey
 * @details The site survey process involves constructing a matrix of (n * n -1) ranges between n node.
 * For this we designate a slot in the superframe that performs a nrng_requst to all other nodes, a slot for broadcasting
 * the result between nodes. The a JSON encoder sentense of the survey is available with SURVEY_VERBOSE enabled.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <os/os.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <stats/stats.h>

#if MYNEWT_VAL(TDMA_ENABLED)
#include <tdma/tdma.h>
#endif
#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif
#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif
#if MYNEWT_VAL(NRNG_ENABLED)
#include <uwb_rng/uwb_rng.h>
#include <nrng/nrng.h>
#include <uwb_rng/slots.h>
#endif

#if MYNEWT_VAL(SURVEY_ENABLED)
#include <survey/survey.h>

#if MYNEWT_VAL(SURVEY_VERBOSE)
static void survey_complete_cb(struct dpl_event *ev);
#include <survey/survey_encode.h>
#endif

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

STATS_NAME_START(survey_stat_section)
    STATS_NAME(survey_stat_section, request)
    STATS_NAME(survey_stat_section, listen)
    STATS_NAME(survey_stat_section, rx_unsolicited)
    STATS_NAME(survey_stat_section, start_tx_error)
    STATS_NAME(survey_stat_section, start_rx_error)
    STATS_NAME(survey_stat_section, broadcaster)
    STATS_NAME(survey_stat_section, receiver)
    STATS_NAME(survey_stat_section, rx_timeout)
    STATS_NAME(survey_stat_section, reset)
STATS_NAME_END(survey_stat_section)

survey_status_t survey_request(survey_instance_t * survey, uint64_t dx_time);
survey_status_t survey_listen(survey_instance_t * survey, uint64_t dx_time);
survey_status_t survey_broadcaster(survey_instance_t * survey, uint64_t dx_time);
survey_status_t survey_receiver(survey_instance_t * survey, uint64_t dx_time);

/**
 *
 * @return survey_instance_t *
 */
survey_instance_t *
survey_init(struct uwb_dev * inst, uint16_t nnodes, uint16_t nframes){
    assert(inst);

    survey_instance_t * survey = (survey_instance_t*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_SURVEY);
    if (survey == NULL) {
        survey = (survey_instance_t *) malloc(sizeof(survey_instance_t) + nframes * sizeof(survey_nrngs_t * ));
        assert(survey);
        memset(survey, 0, sizeof(survey_instance_t) + nframes * sizeof(survey_nrngs_t * ));
/*
        for (uint16_t j = 0; j < nframes; j++){
            survey->nrngs[j] = (survey_nrngs_t *) malloc(sizeof(survey_nrngs_t) + nnodes * sizeof(survey_nrng_t * )); // Variable array alloc
            assert(survey->nrngs[j]);
            memset(survey->nrngs[j], 0, sizeof(survey_nrngs_t) + nnodes * sizeof(survey_nrng_t * ));

            for (uint16_t i = 0; i < nnodes; i++){
                survey->nrngs[j]->nrng[i] = (survey_nrng_t * ) malloc(sizeof(survey_nrng_t) + nnodes * sizeof(dpl_float32_t));
                assert(survey->nrngs[j]->nrng[i]);
                memset(survey->nrngs[j]->nrng[i], 0, sizeof(survey_nrng_t) + nnodes * sizeof(dpl_float32_t));
            }
        }
*/
//        survey->frame = (survey_broadcast_frame_t *) malloc(sizeof(survey_broadcast_frame_t) + nnodes * sizeof(dpl_float32_t));
//        assert(survey->frame);
        memset(survey->frame, 0, sizeof(survey_broadcast_frame_t));
        survey_broadcast_frame_t frame = {
            .PANID = 0xDECA,
            .fctrl = FCNTL_IEEE_RANGE_16,
            .dst_address = 0xffff,      //broadcast
            .src_address = inst->my_short_address,
            .code = UWB_DATA_CODE_SURVEY_BROADCAST
        };

        memcpy(survey->frame, &frame, sizeof(survey_broadcast_frame_t));
        survey->status.selfmalloc = 1;
        survey->nnodes = nnodes;
        survey->nframes = nframes;

        /* Lookup the other instances we will need */
        survey->dev_inst = inst;
        survey->ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
        survey->nrng = (struct nrng_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_NRNG);

        dpl_error_t err = dpl_sem_init(&survey->sem, 0x1);
        assert(err == DPL_OK);
    }else{
        assert(survey->nnodes == nnodes);
    }
    survey->status.initialized = 1;
    survey->config = (survey_config_t){
        .rx_timeout_delay = MYNEWT_VAL(SURVEY_RX_TIMEOUT)
    };

    survey->cbs = (struct uwb_mac_interface){
        .id = UWBEXT_SURVEY,
        .inst_ptr = (void*)survey,
        .rx_complete_cb = rx_complete_cb,
        .tx_complete_cb = tx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
        .reset_cb = reset_cb
    };

#if MYNEWT_VAL(SURVEY_VERBOSE)
    survey->survey_complete_cb = survey_complete_cb;
#endif
    uwb_mac_append_interface(inst, &survey->cbs);

    int rc = stats_init(
                STATS_HDR(survey->stat),
                STATS_SIZE_INIT_PARMS(survey->stat, STATS_SIZE_32),
                STATS_NAME_INIT_PARMS(survey_stat_section)
            );
    assert(rc == 0);

    rc = stats_register("survey", STATS_HDR(survey->stat));
    assert(rc == 0);

    return survey;
}

/**
 * Deconstructor
 *
 * @param inst   Pointer to survey_instance_t *
 * @return void
 */
void
survey_free(survey_instance_t * survey)
{
    assert(survey);
    uwb_mac_remove_interface(survey->dev_inst, survey->cbs.id);

    if (survey->status.selfmalloc){
        free(survey->frame);
        free(survey);
    }else{
        survey->status.initialized = 0;
    }
}


#if MYNEWT_VAL(SURVEY_VERBOSE)
/**
 * API for verbose logging of survey results.
 *
 * @param struct os_event
 * @return none
 */
static void survey_complete_cb(struct dpl_event *ev) {
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev) != NULL);

    survey_instance_t * survey = (survey_instance_t *) dpl_event_get_arg(ev);
    survey_encode(survey, survey->seq_num, survey->idx);
}
#endif

/**
 * Callback to schedule nrng request survey
 *
 * @param struct os_event
 * @return none
 * @brief This callback is call from a TDMA slot (SURVEY_BROADCAST_SLOT) assigned in the toplevel main.
 * The variable SURVEY_MASK is used to derive the sequencing of the node.
 */
void
survey_slot_range_cb(struct dpl_event *ev)
{
    assert(ev);
    assert(dpl_event_get_arg(ev));

    tdma_slot_t * slot = (tdma_slot_t *) dpl_event_get_arg(ev);
    tdma_instance_t * tdma = slot->parent;
    struct uwb_ccp_instance * ccp = tdma->ccp;
    survey_instance_t * survey = (survey_instance_t *)slot->arg;
    survey->seq_num = (ccp->seq_num & ((uint32_t)~0UL << MYNEWT_VAL(SURVEY_MASK))) >> MYNEWT_VAL(SURVEY_MASK);

    if(ccp->seq_num % survey->nnodes == tdma->dev_inst->slot_id){
        uint64_t dx_time = tdma_tx_slot_start(tdma, slot->idx) & 0xFFFFFFFE00UL;
        survey_request(survey, dx_time);
    }else{
        uint64_t dx_time = tdma_rx_slot_start(tdma, slot->idx) & 0xFFFFFFFE00UL;
        survey_listen(survey, dx_time);
    }
}


/**
 * Callback to schedule survey broadcasts
 *
 * @param struct os_event
 * @return none
 * @brief This callback is call from a TDMA slot (SURVEY_BROADCAST_SLOT) assigned in the toplevel main.
 * The variable SURVEY_MASK is used to derive the sequencing of the node.
 */
static struct dpl_event survey_complete_event;

void
survey_slot_broadcast_cb(struct dpl_event *ev)
{
    assert(ev);
    assert(dpl_event_get_arg(ev));

    tdma_slot_t * slot = (tdma_slot_t *) dpl_event_get_arg(ev);
    tdma_instance_t * tdma = slot->parent;
    struct uwb_ccp_instance * ccp = tdma->ccp;
    survey_instance_t * survey = (survey_instance_t *)slot->arg;
    survey->seq_num = (ccp->seq_num & ((uint32_t)~0UL << MYNEWT_VAL(SURVEY_MASK))) >> MYNEWT_VAL(SURVEY_MASK);

    if(ccp->seq_num % survey->nnodes == tdma->dev_inst->slot_id){
        uint64_t dx_time = tdma_tx_slot_start(tdma, slot->idx) & 0xFFFFFFFE00UL;
        survey_broadcaster(survey, dx_time);
    }else{
        uint64_t dx_time = tdma_rx_slot_start(tdma, slot->idx) & 0xFFFFFFFE00UL;
        survey_receiver(survey, dx_time);
    }
    if((ccp->seq_num % survey->nnodes == survey->nnodes - 1) && survey->survey_complete_cb){
        dpl_event_init(&survey_complete_event, survey->survey_complete_cb, (void *) survey);
        dpl_eventq_put(dpl_eventq_dflt_get(), &survey_complete_event);
    }
}

/**
 * API to initiaate a nrng request from a node to node survey
 *
 * @param inst pointer to survey_instance_t.
 * @param dx_time time to start suevey
 * @return survey_status_t
 *
 */
survey_status_t
survey_request(survey_instance_t * survey, uint64_t dx_time)
{
    assert(survey);
    uint16_t slot_id = survey->dev_inst->slot_id;

    STATS_INC(survey->stat, request);

    uint32_t slot_mask = ~(~0UL << (survey->nnodes));
    nrng_request_delay_start(survey->nrng, 0xffff, dx_time, UWB_DATA_CODE_SS_TWR_NRNG, slot_mask, 0);

    survey_nrngs_t * nrngs = survey->nrngs[(survey->idx)%survey->nframes];
    dpl_float32_t range[survey->nnodes];
    uint16_t uid[survey->nnodes];

    nrngs->nrng[slot_id].mask = nrng_get_ranges(survey->nrng,
                                range,
                                survey->nnodes,
                                survey->nrng->idx
        );
    nrng_get_uids(survey->nrng,
                                uid,
                                survey->nnodes,
                                survey->nrng->idx
        );
    for (uint16_t i = 0;i < survey->nnodes; i++){
        nrngs->nrng[slot_id].rng[i] = range[i];
        nrngs->nrng[slot_id].uid[i] = uid[i];
    }

    return survey->status;
}


survey_status_t
survey_listen(survey_instance_t * survey, uint64_t dx_time){
    assert(survey);

    STATS_INC(survey->stat, listen);

    uwb_set_delay_start(survey->dev_inst, dx_time);
    uint16_t timeout = uwb_phy_frame_duration(survey->dev_inst, sizeof(nrng_request_frame_t))
                        + survey->nrng->config.rx_timeout_delay;
    uwb_set_rx_timeout(survey->dev_inst, timeout + 0x1000);
    nrng_listen(survey->nrng, UWB_BLOCKING);

    return survey->status;
}


/**
 * API to broadcasts survey results
 *
 * @param inst pointer to survey_instance_t * survey.
 * @param dx_time time to start broadcast
 * @return survey_status_t
 *
 */
survey_status_t
survey_broadcaster(survey_instance_t * survey, uint64_t dx_time){

    assert(survey);

    dpl_error_t err = dpl_sem_pend(&survey->sem,  DPL_TIMEOUT_NEVER);
    assert(err == DPL_OK);
    STATS_INC(survey->stat, broadcaster);

    struct uwb_dev * inst = survey->dev_inst;
    survey_nrngs_t * nrngs = survey->nrngs[survey->idx%survey->nframes];

    survey->frame->mask = nrngs->nrng[inst->slot_id].mask;
    survey->frame->seq_num = survey->seq_num;
    survey->frame->slot_id = inst->slot_id;

    uint16_t nnodes = NumberOfBits(survey->frame->mask);
    survey->status.empty = nnodes == 0;
    if (survey->status.empty){
        err = dpl_sem_release(&survey->sem);
        assert(err == DPL_OK);
        return survey->status;
    }

    assert(nnodes < survey->nnodes);

//    memcpy(survey->frame->rng, nrngs->nrng[inst->slot_id].rng, nnodes * sizeof(dpl_float32_t));
//    memcpy(survey->frame->uid, nrngs->nrng[inst->slot_id].uid, nnodes * sizeof(uint16_t));

    uint16_t n = sizeof(struct _survey_broadcast_frame_t)
        + sizeof(uint16_t)
        + nnodes * sizeof(dpl_float32_t)
        + nnodes * sizeof(uint16_t);

    uwb_write_tx(inst, survey->frame->array, 0, n);
    uwb_write_tx_fctrl(inst, n, 0);
    uwb_set_delay_start(inst, dx_time);

    survey->status.start_tx_error = uwb_start_tx(inst).start_tx_error;
    if (survey->status.start_tx_error){
        STATS_INC(survey->stat, start_tx_error);
        if (dpl_sem_get_count(&survey->sem) == 0)
            dpl_sem_release(&survey->sem);
    }else{
        err = dpl_sem_pend(&survey->sem, DPL_TIMEOUT_NEVER); // Wait for completion of transactions
        assert(err == DPL_OK);
        err = dpl_sem_release(&survey->sem);
        assert(err == DPL_OK);
    }
    return survey->status;
}

/**
 * API to receive survey broadcasts
 *
 * @param inst pointer to survey_instance_t * survey.
 * @param dx_time time to start received
 * @return survey_status_t
 *
 */
survey_status_t
survey_receiver(survey_instance_t * survey, uint64_t dx_time){
    assert(survey);

    struct uwb_dev * inst = survey->dev_inst;
    dpl_error_t err = dpl_sem_pend(&survey->sem,  DPL_TIMEOUT_NEVER);
    assert(err == DPL_OK);
    STATS_INC(survey->stat, receiver);

    uint16_t n = sizeof(struct _survey_broadcast_frame_t) + sizeof(struct survey_nrng);
    uint16_t timeout = uwb_phy_frame_duration(inst, n)
                        + survey->config.rx_timeout_delay;
    uwb_set_rx_timeout(inst, timeout);

    survey->status.start_rx_error = uwb_start_rx(inst).start_rx_error;
    if(survey->status.start_rx_error){
        STATS_INC(survey->stat, start_rx_error);
        err = dpl_sem_release(&survey->sem);
        assert(err == DPL_OK);
    }else{
        err = dpl_sem_pend(&survey->sem, DPL_TIMEOUT_NEVER); // Wait for completion of transactions
        assert(err == DPL_OK);
        err = dpl_sem_release(&survey->sem);
        assert(err == DPL_OK);
    }
    return survey->status;
}

/**
 * API for receive survey broadcasts.
 *
 * @param inst pointer to struct uwb_dev
 * @param cbs pointer tostruct uwb_mac_interface
 * @return true on sucess
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    survey_instance_t * survey = (survey_instance_t *)cbs->inst_ptr;

    if(inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(dpl_sem_get_count(&survey->sem) == 1){ // unsolicited inbound
        STATS_INC(survey->stat, rx_unsolicited);
        return false;
    }

    if(inst->frame_len < sizeof(survey_broadcast_frame_t))
       return false;

    survey_broadcast_frame_t * frame = ((survey_broadcast_frame_t * ) inst->rxbuf);

    if(frame->dst_address != 0xffff)
        return false;
    if(survey->ccp->seq_num % survey->nnodes == 0)
        survey->idx++;  // advance the nrngs idx at begining of sequence.

    switch(frame->code) {
        case UWB_DATA_CODE_SURVEY_BROADCAST:
            {
                if (frame->cell_id != inst->cell_id)
                    return false;
                if (frame->seq_num != survey->seq_num)
                    break;
           //     uint16_t n = sizeof(survey_broadcast_frame_t) + survey->nnodes * sizeof(float);
           //     if (inst->frame_len > n || frame->slot_id > survey->nnodes - 1) {
           //         return false;
           //     }
                survey_nrngs_t * nrngs = survey->nrngs[survey->idx%survey->nframes];
                uint16_t nnodes = NumberOfBits(frame->mask);
                survey->status.empty = nnodes == 0;
                if(survey->status.empty == 0){
                    nrngs->mask |= 1U << frame->slot_id;
//                    nrngs->nrng[frame->slot_id].mask = frame->mask;
//                    memcpy(nrngs->nrng[frame->slot_id].rng, frame->rng, nnodes * sizeof(dpl_float32_t));
//                    memcpy(nrngs->nrng[frame->slot_id].uid, frame->uid, nnodes * sizeof(uint16_t));
                    break;
                }else{
                    nrngs->nrng[frame->slot_id].mask = 0;
                    break;
                }
            }
            break;
        default:
            return false;
    }
    dpl_error_t err = dpl_sem_release(&survey->sem);
    assert(err == DPL_OK);
    return true;
}


/**
 * API for transmission complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    survey_instance_t * survey = (survey_instance_t *)cbs->inst_ptr;

    if(dpl_sem_get_count(&survey->sem) == 1)
        return false;

    dpl_error_t err = dpl_sem_release(&survey->sem);
    assert(err == DPL_OK);

    return false;
}


/**
 * API for timeout complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return true on sucess
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    survey_instance_t * survey = (survey_instance_t *)cbs->inst_ptr;

    if(dpl_sem_get_count(&survey->sem) == 1)
        return false;

    dpl_error_t err = dpl_sem_release(&survey->sem);
    assert(err == DPL_OK);
    STATS_INC(survey->stat, rx_timeout);

    return true;
}


/**
 * API for reset_cb of survey interface
 *
 * @param inst   Pointer to struct uwb_dev.
 * @return true on sucess
 */
static bool
reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    survey_instance_t * survey = (survey_instance_t *)cbs->inst_ptr;

    if(dpl_sem_get_count(&survey->sem) == 1)
        return false;

    dpl_error_t err = dpl_sem_release(&survey->sem);
    assert(err == DPL_OK);
    STATS_INC(survey->stat, reset);

    return true;
}

#endif // MYNEWT_VAL(SURVEY_ENABLED)

/**
 * API to initialise the package
 *
 * @return void
 */
void
survey_pkg_init(void)
{
#if MYNEWT_VAL(SURVEY_ENABLED)
    printf("{\"utime\": %lu,\"msg\": \"survey_pkg_init\"}\n",os_cputime_ticks_to_usecs(os_cputime_get32()));

#if MYNEWT_VAL(UWB_DEVICE_0)
    survey_init(uwb_dev_idx_lookup(0), MYNEWT_VAL(SURVEY_NNODES), MYNEWT_VAL(SURVEY_NFRAMES));
#endif
#endif // MYNEWT_VAL(SURVEY_ENABLED)
}
