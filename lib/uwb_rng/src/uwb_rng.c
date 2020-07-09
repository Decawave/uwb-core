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
 * @file rng.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Range
 *
 * @details This is the rng base class which utilises the functions to enable/disable the configurations related to rng.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <syscfg/syscfg.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <stats/stats.h>
#include <rng_math/rng_math.h>

#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <dsp/polyval.h>

#if MYNEWT_VAL(UWB_RNG_ENABLED)
#include <uwb_rng/uwb_rng.h>
#include <uwb_rng/rng_encode.h>
#endif
#if MYNEWT_VAL(UWB_WCS_ENABLED)
#include <uwb_wcs/uwb_wcs.h>
#endif
#if MYNEWT_VAL(CIR_ENABLED)
#include <cir/cir.h>
#endif

#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>

void uwbrng_sysfs_init(struct uwb_rng_instance *rng);
void uwbrng_sysfs_deinit(int idx);
int rng_chrdev_create(int idx);
void rng_chrdev_destroy(int idx);

#define slog(fmt, ...)                                                  \
    pr_info("uwbcore: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define slog(fmt, ...) {}
#endif

#if MYNEWT_VAL(RNG_STATS)
STATS_NAME_START(rng_stat_section)
    STATS_NAME(rng_stat_section, rng_request)
    STATS_NAME(rng_stat_section, rng_listen)
    STATS_NAME(rng_stat_section, tx_complete)
    STATS_NAME(rng_stat_section, rx_complete)
    STATS_NAME(rng_stat_section, rx_unsolicited)
    STATS_NAME(rng_stat_section, rx_other_frame)
    STATS_NAME(rng_stat_section, rx_error)
    STATS_NAME(rng_stat_section, tx_error)
    STATS_NAME(rng_stat_section, rx_timeout)
    STATS_NAME(rng_stat_section, complete_cb)
    STATS_NAME(rng_stat_section, reset)
    STATS_NAME(rng_stat_section, superframe_reset)
STATS_NAME_END(rng_stat_section)

#define RNG_STATS_INC(__X) STATS_INC(rng->stat, __X)
#else
#define RNG_STATS_INC(__X) {}
#endif

//#define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool superframe_reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool tx_final_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
#if MYNEWT_VAL(RNG_VERBOSE)
static bool complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
#endif

#ifndef __KERNEL__
/*
  % From APS011 Table 2
  rls = [-61,-63,-65,-67,-69,-71,-73,-75,-77,-79,-81,-83,-85,-87,-89,-91,-93];
  bias = [-11,-10.4,-10.0,-9.3,-8.2,-6.9,-5.1,-2.7,0,2.1,3.5,4.2,4.9,6.2,7.1,7.6,8.1]./100;
  p=polyfit(rls,bias,3)
  mat2c(p,'rng_bias_poly_PRF64')
  bias = [-19.8,-18.7,-17.9,-16.3,-14.3,-12.7,-10.9,-8.4,-5.9,-3.1,0,3.6,6.5,8.4,9.7,10.6,11.0]./100;
  p=polyfit(rls,bias,3)
  mat2c(p,'rng_bias_poly_PRF16')
*/
static float rng_bias_poly_PRF64[] ={
    1.404476e-03, 3.208478e-01, 2.349322e+01, 5.470342e+02,
};
static float rng_bias_poly_PRF16[] ={
    1.754924e-05, 4.106182e-03, 3.061584e-01, 7.189425e+00,
};
#endif

static struct uwb_rng_config g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(RNG_TX_HOLDOFF),       // Send Time delay in usec.
    .rx_timeout_delay = MYNEWT_VAL(RNG_RX_TIMEOUT)       // Receive response timeout in usec
};

static twr_frame_t g_twr_frames[][4] = {
#if MYNEWT_VAL(UWB_DEVICE_0)
    {
        [0] = {.fctrl = 0, 0},
        [1] = {.fctrl = 0, 0},
        [2] = {.fctrl = 0, 0},
        [3] = {.fctrl = 0, 0},
    },
#endif
#if MYNEWT_VAL(UWB_DEVICE_1)
    {
        [0] = {.fctrl = 0, 0},
        [1] = {.fctrl = 0, 0},
        [2] = {.fctrl = 0, 0},
        [3] = {.fctrl = 0, 0},
    },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    {
        [0] = {.fctrl = 0, 0},
        [1] = {.fctrl = 0, 0},
        [2] = {.fctrl = 0, 0},
        [3] = {.fctrl = 0, 0},
    },
#endif
};

static struct uwb_mac_interface g_cbs[] = {
    [0] = {
        .id = UWBEXT_RNG,
        .inst_ptr = 0,
        .rx_complete_cb = rx_complete_cb,
        .tx_complete_cb = tx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
        .final_cb = tx_final_cb,
#if MYNEWT_VAL(RNG_VERBOSE)
        .complete_cb  = complete_cb,
#endif
        .reset_cb = reset_cb,
        .superframe_cb = superframe_reset_cb
    },
#if MYNEWT_VAL(UWB_DEVICE_1)
    [1] = {
        .id = UWBEXT_RNG,
        .inst_ptr = 0,
        .rx_complete_cb = rx_complete_cb,
        .tx_complete_cb = tx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
        .final_cb = tx_final_cb,
#if MYNEWT_VAL(RNG_VERBOSE)
        .complete_cb  = complete_cb,
#endif
        .reset_cb = reset_cb,
        .superframe_cb = superframe_reset_cb
    },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    [2] = {
        .id = UWBEXT_RNG,
        .inst_ptr = 0,
        .rx_complete_cb = rx_complete_cb,
        .tx_complete_cb = tx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
        .final_cb = tx_final_cb,
#if MYNEWT_VAL(RNG_VERBOSE)
        .complete_cb  = complete_cb,
#endif
        .reset_cb = reset_cb,
        .superframe_cb = superframe_reset_cb
    }
#endif
};

/**
 * @fn uwb_rng_init(struct uwb_dev * inst, struct uwb_rng_config * config, uint16_t nframes)
 * @brief API to initialise the ranging by setting all the required configurations and callbacks.
 *
 * @param inst      Pointer to struct uwb_dev.
 * @param config    Pointer to the structure struct uwb_rng_config.
 * @param nframes   Number of buffers defined to store the ranging data.
 *
 * @return struct uwb_rng_instance
 */
struct uwb_rng_instance *
uwb_rng_init(struct uwb_dev * dev, struct uwb_rng_config * config, uint16_t nframes)
{
    dpl_error_t err;
    struct uwb_rng_instance *rng;
    assert(dev);
    rng = (struct uwb_rng_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_RNG);
    if (rng == NULL ) {
        /* struct + flexible array member */
        rng = (struct uwb_rng_instance *) calloc(1, sizeof(*rng) + nframes * sizeof(twr_frame_t *));
        assert(rng);
        rng->status.selfmalloc = 1;
        rng->nframes = nframes;
    }
    rng->dev_inst = dev;
#if MYNEWT_VAL(UWB_WCS_ENABLED)
    rng->ccp_inst = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_CCP);
    assert(rng->ccp_inst);
#endif
    err = dpl_sem_init(&rng->sem, 0x1);
    assert(err == DPL_OK);

    if (config != NULL ) {
        uwb_rng_config(rng, config);
    }

    rng->control = (uwb_rng_control_t){
        .delay_start_enabled = 0,
    };
    rng->idx = 0xFFFF;
    rng->status.initialized = 1;

#if MYNEWT_VAL(RNG_STATS)
    {
        int rc = stats_init(
            STATS_HDR(rng->stat),
            STATS_SIZE_INIT_PARMS(rng->stat, STATS_SIZE_32),
            STATS_NAME_INIT_PARMS(rng_stat_section)
            );

#if  MYNEWT_VAL(UWB_DEVICE_0) && !MYNEWT_VAL(UWB_DEVICE_1)
        rc |= stats_register("rng", STATS_HDR(rng->stat));
#elif  MYNEWT_VAL(UWB_DEVICE_0) && MYNEWT_VAL(UWB_DEVICE_1)
        if (dev->idx == 0)
            rc |= stats_register("rng0", STATS_HDR(rng->stat));
        else
            rc |= stats_register("rng1", STATS_HDR(rng->stat));
#endif
        assert(rc == 0);
    }
#endif
    return rng;
}

/**
 * @fn uwb_rng_free(struct uwb_rng_instance * inst)
 * @brief API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 *
 * @return void
 */
void
uwb_rng_free(struct uwb_rng_instance * rng)
{
    assert(rng);
    if (rng->status.selfmalloc)
        free(rng);
    else
        rng->status.initialized = 0;
}

/**
 * @fn uwb_rng_set_frames(struct uwb_rng_instance * inst, twr_frame_t twr[], uint16_t nframes)
 * @brief API to set the pointer to the twr buffers.
 *
 * @param inst      Pointer to struct uwb_rng_instance.
 * @param twr[]     Pointer to twr buffers.
 * @param nframes   Number of buffers defined to store the ranging data.
 *
 * @return void
 */
inline void
uwb_rng_set_frames(struct uwb_rng_instance * rng, twr_frame_t twr[], uint16_t nframes)
{
    uint16_t i;
    assert(nframes <= rng->nframes);
    for (i = 0; i < nframes; i++)
        rng->frames[i] = &twr[i];
}

/**
 * @fn uwb_rng_config(struct uwb_rng_instance * inst, struct uwb_rng_config * config)
 * @brief API to assign the config parameters to range instance.
 *
 * @param inst    Pointer to struct uwb_rng_instance.
 * @param config  Pointer to struct uwb_rng_config.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_rng_config(struct uwb_rng_instance * rng, struct uwb_rng_config * config){
    assert(config);

    memcpy(&rng->config, config, sizeof(struct uwb_rng_config));
    return rng->dev_inst->status;
}

/**
 * @fn uwb_rng_get_config(struct uwb_rng_instance * inst, uwb_dataframe_code_t code)
 * @brief API to get configuration using uwb_dataframe_code_t.
 *
 * @param inst          Pointer to struct uwb_rng_instance.
 * @param code          Code representing the required mode of ranging.
 *                      E.g. UWB_DATA_CODE_SS_TWR enables single sided two way ranging.
 *
 * @return struct uwb_rng_config
 */
struct uwb_rng_config *
uwb_rng_get_config(struct uwb_rng_instance * rng, uwb_dataframe_code_t code)
{
    struct rng_config_list * cfgs;

    if(!(SLIST_EMPTY(&rng->rng_configs))){
        SLIST_FOREACH(cfgs, &rng->rng_configs, next){
            if (cfgs != NULL && cfgs->rng_code == code) {
                return cfgs->config;
            }
        }
    }
    return &g_config;
}

/**
 * Add config extension different rng services.
 *
 * @param rng        Pointer to struct uwb_rng_instance.
 * @param callbacks  callback instance.
 * @return void
 */
void
uwb_rng_append_config(struct uwb_rng_instance * rng, struct rng_config_list *cfgs)
{
    assert(rng);

    if(!(SLIST_EMPTY(&rng->rng_configs))) {
        struct rng_config_list * prev_cfgs = NULL;
        struct rng_config_list * cur_cfgs = NULL;
        SLIST_FOREACH(cur_cfgs, &rng->rng_configs, next){
            prev_cfgs = cur_cfgs;
        }
        SLIST_INSERT_AFTER(prev_cfgs, cfgs, next);
    } else {
        SLIST_INSERT_HEAD(&rng->rng_configs, cfgs, next);
    }
}


/**
 * API to remove specified callbacks.
 *
 * @param inst  Pointer to struct uwb_rng_instance.
 * @param id    ID of the service.
 * @return void
 */
void
uwb_rng_remove_config(struct uwb_rng_instance * rng, uwb_dataframe_code_t code)
{
    struct rng_config_list * cfgs = NULL;
    assert(rng);
    SLIST_FOREACH(cfgs, &rng->rng_configs, next){
        if(cfgs->rng_code == code){
            SLIST_REMOVE(&rng->rng_configs, cfgs, rng_config_list, next);
            break;
        }
    }
}

/**
 * @fn uwb_rng_calc_rel_tx(struct uwb_rng_instance * inst, struct uwb_rng_txd *ret, struct uwb_rng_config *cfg, uint64_t ts)
 * @brief Calculate when to send response
 *
 * @param rng         Pointer to struct uwb_rng_instance.
 * @param ret         Pointer to struct uwb_rng_txd which will contain the results
 * @param ts          Timestamp of received frame relative which the tx-timestamps will be given
 * @param rx_data_len Length of received frame
 *
 * @return void
 */
void
uwb_rng_calc_rel_tx(struct uwb_rng_instance * rng, struct uwb_rng_txd *ret,
                    struct uwb_rng_config *cfg, uint64_t ts, uint16_t rx_data_len)
{
    /* Rx-frame data duration, as this is after the r-marker it eats into the time
     * we have to respond. */
    uint16_t data_duration = uwb_usecs_to_dwt_usecs(uwb_phy_data_duration(rng->dev_inst, rx_data_len));
    ret->response_tx_delay = ts + (((uint64_t) cfg->tx_holdoff_delay + rng->frame_shr_duration + data_duration) << 16);
    ret->response_timestamp = (ret->response_tx_delay & 0xFFFFFFFE00UL) + rng->dev_inst->tx_antenna_delay;
    return;
}

/**
 * @fn uwb_rng_clear_twr_data(struct _twr_data_t *s)
 * @brief Clear values in a twr_data structure
 *
 * @param s          Pointer to struct_twr_data_t.
 *
 * @return void
 */
void
uwb_rng_clear_twr_data(struct _twr_data_t *s)
{
    s->spherical.array[0] = DPL_FLOAT64_NAN();
    s->spherical.array[1] = DPL_FLOAT64_NAN();
    s->spherical.array[2] = DPL_FLOAT64_NAN();
    s->rssi = DPL_FLOAT32_NAN();
    s->fppl = DPL_FLOAT32_NAN();
    s->pdoa = DPL_FLOAT32_NAN();
    s->flags = (struct _rng_frame_flags){0};
}

/**
 * @fn uwb_rng_request(struct uwb_rng_instance * inst, uint16_t dst_address, uwb_dataframe_code_t code)
 * @brief API to initialise range request.
 *
 * @param inst          Pointer to struct uwb_rng_instance.
 * @param dst_address   Address of the receiver to whom range request to be sent.
 * @param code          Represents mode of ranging UWB_DATA_CODE_SS_TWR enables single sided two way ranging UWB_DATA_CODE_DS_TWR enables double sided
 * two way ranging UWB_DATA_CODE_DS_TWR_EXT enables double sided two way ranging with extended frame.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_rng_request(struct uwb_rng_instance * rng, uint16_t dst_address, uwb_dataframe_code_t code)
{
    // This function executes on the device that initiates a request
    dpl_error_t err;
    uint16_t data_duration, frame_duration;
    struct uwb_dev * inst = rng->dev_inst;
    twr_frame_t * frame  = rng->frames[(rng->idx+1)%rng->nframes];
    struct uwb_rng_config * config = uwb_rng_get_config(rng, code);
    if (!config) {
        slog("No such rng type: %d\n", code);
        goto early_exit;
    }
    RNG_STATS_INC(rng_request);
    err = dpl_sem_pend(&rng->sem,  DPL_TIMEOUT_NEVER);
    if (err != DPL_OK) {
        goto early_exit;
    }

    /* Prevent any IRQs from interfering */
    err = dpl_sem_pend(&inst->irq_sem,  DPL_TIMEOUT_NEVER);
    if (err != DPL_OK) {
        goto irqsem_error_exit;
    }

    if (code == UWB_DATA_CODE_SS_TWR || code == UWB_DATA_CODE_SS_TWR_EXT)
        rng->seq_num+=1;
    else
        rng->seq_num+=2;

    frame->fctrl = FCNTL_IEEE_RANGE_16;
    frame->fctrl |=  (config->fctrl_req_ack)? UWB_FCTRL_ACK_REQUESTED : 0;
    frame->seq_num = rng->seq_num;
    frame->code = code;
    frame->PANID = inst->pan_id;
    frame->src_address = inst->my_short_address;
    frame->dst_address = dst_address;

    uwb_rng_clear_twr_data(&frame->remote);
    uwb_rng_clear_twr_data(&frame->local);

    // Download the CIR on the response
#if MYNEWT_VAL(CIR_ENABLED)
    cir_enable(inst->cir, true);
#endif

    uwb_write_tx(inst, frame->array, 0, sizeof(ieee_rng_request_frame_t));
    uwb_write_tx_fctrl(inst, sizeof(ieee_rng_request_frame_t), 0);
    uwb_set_wait4resp(inst, true);

    data_duration = uwb_usecs_to_dwt_usecs(uwb_phy_data_duration(inst, sizeof(ieee_rng_response_frame_t)));
    frame_duration = uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, sizeof(ieee_rng_response_frame_t)));
    rng->frame_shr_duration = frame_duration - data_duration;

    // The wait for response counter starts on the completion of the entire outgoing frame.
    // The responding unit will take the data-duration and the shr duration into account
    // when calculating the holdoff, so we only need to compensate for the time it takes
    // for our receiver to wake up.
    if (!config->fctrl_req_ack) {
        uwb_set_wait4resp_delay(inst, config->tx_holdoff_delay -
                                inst->config.rx.timeToRxStable);
        uwb_set_rxauto_disable(inst, true);
    } else {
        uwb_set_wait4resp_delay(inst, 0);
        rng->status.rx_ack_expected = 1;
        uwb_set_rxauto_disable(inst, false);
    }

    // The timeout counter starts when the receiver in re-enabled. The timeout event
    // should occur just after the inbound frame is received
    if (code == UWB_DATA_CODE_SS_TWR_EXT) {
        frame_duration = uwb_usecs_to_dwt_usecs(uwb_phy_frame_duration(inst, TWR_EXT_FRAME_SIZE));
    }
    uwb_set_rx_timeout(inst, frame_duration + config->rx_timeout_delay +
                       inst->config.rx.timeToRxStable);

    if (rng->control.delay_start_enabled)
        uwb_set_delay_start(inst, rng->delay);

    /* Release hold on irq sem */
    dpl_sem_release(&inst->irq_sem);
    if (uwb_start_tx(inst).start_tx_error) {
        dpl_sem_release(&rng->sem);
        RNG_STATS_INC(tx_error);
    }

    err = dpl_sem_pend(&rng->sem, DPL_TIMEOUT_NEVER); // Wait for completion of transactions
    if (err != DPL_OK) {
        goto early_exit;
    }

irqsem_error_exit:
    if(dpl_sem_get_count(&rng->sem) == 0){
        err = dpl_sem_release(&rng->sem);
        if (err != DPL_OK) {
            goto early_exit;
        }
    }

early_exit:
    return inst->status;
}

/**
 * @fn uwb_rng_listen(struct uwb_rng_instance * inst, uwb_dev_modes_t mode)
 * @brief API to listen rng request.
 *
 * @param inst          Pointer to struct uwb_rng_instance.
 * @param timeout       -1 don't set a timeout, 0 listen forever, >0 timeout in uus
 * @param mode          uwb_dev_modes_t of UWB_BLOCKING and UWB_NONBLOCKING
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_rng_listen(struct uwb_rng_instance * rng, int32_t timeout, uwb_dev_modes_t mode)
{
    struct uwb_dev * inst = rng->dev_inst;
    dpl_error_t err = dpl_sem_pend(&rng->sem,  DPL_TIMEOUT_NEVER);
    if (err != DPL_OK) {
        goto early_exit;
    }
    // Download the CIR on the response
#if MYNEWT_VAL(CIR_ENABLED)
    cir_enable(rng->dev_inst->cir, true);
#endif
    uwb_set_rxauto_disable(rng->dev_inst, true);

    /* Precalculate shr duration */
    rng->frame_shr_duration = uwb_usecs_to_dwt_usecs(uwb_phy_SHR_duration(rng->dev_inst));

    if (rng->control.delay_start_enabled)
        uwb_set_delay_start(inst, rng->delay);

    if (timeout > -1) {
        uwb_set_rx_timeout(inst, timeout);
    }

    RNG_STATS_INC(rng_listen);
    if(uwb_start_rx(rng->dev_inst).start_rx_error){
        err = dpl_sem_release(&rng->sem);
        assert(err == DPL_OK);
        RNG_STATS_INC(rx_error);
    }

    if (mode == UWB_BLOCKING){
        err = dpl_sem_pend(&rng->sem, DPL_TIMEOUT_NEVER); // Wait for completion of transactions
        if (err != DPL_OK) {
            goto early_exit;
        }
        err = dpl_sem_release(&rng->sem);
        assert(err == DPL_OK);
    }

early_exit:
    return rng->dev_inst->status;
}

/**
 * @fn uwb_rng_request_delay_start(struct uwb_rng_instance * inst, uint16_t dst_address, uint64_t delay, uwb_dataframe_code_t code)
 * @brief API to configure dw1000 to start transmission after certain delay.
 *
 * @param inst          Pointer to struct uwb_rng_instance.
 * @param dst_address   Address of the receiver to whom range request to be sent.
 * @param delay         Time until which request has to be resumed.
 * @param code          Represents mode of ranging UWB_DATA_CODE_SS_TWR enables single sided two way ranging UWB_DATA_CODE_DS_TWR enables double sided
 * two way ranging UWB_DATA_CODE_DS_TWR_EXT enables double sided two way ranging with extended frame.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_rng_request_delay_start(struct uwb_rng_instance * rng, uint16_t dst_address, uint64_t delay, uwb_dataframe_code_t code)
{
    rng->control.delay_start_enabled = 1;
    rng->delay = delay;
    uwb_rng_request(rng, dst_address, code);
    rng->control.delay_start_enabled = 0;

    return rng->dev_inst->status;
}

/**
 * @fn uwb_rng_request_delay_start(struct uwb_rng_instance * inst, uint16_t dst_address, uint64_t delay, uwb_dataframe_code_t code)
 * @brief API to configure dw1000 to start transmission after certain delay.
 *
 * @param inst          Pointer to struct uwb_rng_instance.
 * @param dx_time       Time when listen should start
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_rng_listen_delay_start(struct uwb_rng_instance * rng, uint64_t dx_time, uint32_t timeout, uwb_dev_modes_t mode)
{
    rng->control.delay_start_enabled = 1;
    rng->delay = dx_time;
    uwb_rng_listen(rng, timeout, mode);
    rng->control.delay_start_enabled = 0;

    return rng->dev_inst->status;
}

/**
 * @fn uwb_rng_bias_correction(struct uwb_dev * inst, float Pr)
 * @brief API for bias correction polynomial.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param pr     Variable that calculates range path loss.
 *
 * @return Bias value
 */
#ifndef __KERNEL__
float
uwb_rng_bias_correction(struct uwb_dev * dev, float Pr){
    float bias;
    switch(dev->config.prf){
        case DWT_PRF_16M:
            bias = polyval(rng_bias_poly_PRF16, Pr, sizeof(rng_bias_poly_PRF16)/sizeof(float));
            break;
        case DWT_PRF_64M:
            bias = polyval(rng_bias_poly_PRF64, Pr, sizeof(rng_bias_poly_PRF64)/sizeof(float));
            break;
        default:
            assert(0);
    }
    return bias;
}
#endif

/**
 * @fn uwb_rng_twr_to_tof(struct uwb_rng_instance * rng, uint16_t idx)
 * @brief API to calculate time of flight based on type of ranging.
 *
 * @param rng  Pointer to struct uwb_rng_instance.
 * @param idx  Position of rng frame
 *
 * @return Time of flight in float
 */
dpl_float64_t
uwb_rng_twr_to_tof(struct uwb_rng_instance * rng, uint16_t idx)
{
    dpl_float64_t skew;
    dpl_float64_t ToF = DPL_FLOAT64_I32_TO_F64(0.0);

    twr_frame_t * first_frame = rng->frames[(uint16_t)(idx-1)%rng->nframes];
    twr_frame_t * frame = rng->frames[(idx)%rng->nframes];

#if MYNEWT_VAL(UWB_WCS_ENABLED)
    skew = DPL_FLOAT64_INIT(0.0);
#else
    skew = uwb_calc_clock_offset_ratio(rng->dev_inst,
                    frame->carrier_integrator, UWB_CR_CARRIER_INTEGRATOR);
#endif

    switch(frame->code) {
        case UWB_DATA_CODE_SS_TWR ... UWB_DATA_CODE_SS_TWR_END:
        case UWB_DATA_CODE_SS_TWR_ACK ... UWB_DATA_CODE_SS_TWR_ACK_END:
        case UWB_DATA_CODE_SS_TWR_EXT ... UWB_DATA_CODE_SS_TWR_EXT_END:
            ToF = calc_tof_ss(frame->response_timestamp, frame->request_timestamp,
                              frame->transmission_timestamp, frame->reception_timestamp, skew);
            break;
        case UWB_DATA_CODE_DS_TWR ... UWB_DATA_CODE_DS_TWR_END:
        case UWB_DATA_CODE_DS_TWR_EXT ... UWB_DATA_CODE_DS_TWR_EXT_END:
            ToF = calc_tof_ds(first_frame->response_timestamp, first_frame->request_timestamp,
                              first_frame->transmission_timestamp, first_frame->reception_timestamp,
                              frame->response_timestamp, frame->request_timestamp,
                              frame->transmission_timestamp, frame->reception_timestamp);
            break;
    }
    return ToF;
}

/**
 * @fn rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for receive timeout callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true on sucess
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&rng->sem) == 1)
        return false;

    if(dpl_sem_get_count(&rng->sem) == 0){
        dpl_error_t err = dpl_sem_release(&rng->sem);
        assert(err == DPL_OK);
        RNG_STATS_INC(rx_timeout);
        switch(rng->code){
            case UWB_DATA_CODE_SS_TWR ... UWB_DATA_CODE_DS_TWR_EXT_FINAL:
                {
                    RNG_STATS_INC(rx_timeout);
                    return true;
                }
                break;
            default:
                return false;
        }
    }
    return false;
}

/**
 * @fn reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for reset_cb of rng interface
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return true on sucess
 */
static bool
reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&rng->sem) == 0){
        dpl_error_t err = dpl_sem_release(&rng->sem);
        assert(err == DPL_OK);
        RNG_STATS_INC(reset);
        rng->status.rx_ack_expected = 0;
        rng->status.tx_ack_expected = 0;
        return true;
    }
    else
        return false;
}

/**
 * @fn superframe_reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for reset_cb of rng interface
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return true on sucess
 */
static bool
superframe_reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    if(dpl_sem_get_count(&rng->sem) == 0){
        dpl_error_t err = dpl_sem_release(&rng->sem);
        assert(err == DPL_OK);
        RNG_STATS_INC(superframe_reset);
        printf("{\"utime\": %"PRIu32",\"msg\": \"superframe_reset\"}\n",
               dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
        rng->status.rx_ack_expected = 0;
        rng->status.tx_ack_expected = 0;
    }
    return false;
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
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    ieee_rng_request_frame_t * req_frame;

    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED) &&
        inst->fctrl != UWB_FCTRL_FRAME_TYPE_ACK) {
        if(dpl_sem_get_count(&rng->sem) == 0) {
            /* We were expecting a packet but something else came in.
             * --> Release sem */
            RNG_STATS_INC(rx_other_frame);
            dpl_sem_release(&rng->sem);
        }
        return false;
    }

    if(dpl_sem_get_count(&rng->sem) == 1){
        // unsolicited inbound
        RNG_STATS_INC(rx_unsolicited);
        return false;
    }

    if (inst->frame_len < sizeof(ieee_rng_request_frame_t))
       return false;

    req_frame = (ieee_rng_request_frame_t * ) inst->rxbuf;
    rng->code = req_frame->code;
    switch(rng->code) {
        case UWB_DATA_CODE_SS_TWR ... UWB_DATA_CODE_DS_TWR_EXT_END:
            {
                twr_frame_t * frame = rng->frames[(rng->idx+1)%rng->nframes]; // speculative frame advance
                /* Clear meta and angle-information */
                uwb_rng_clear_twr_data(&frame->remote);
                uwb_rng_clear_twr_data(&frame->local);

                if (inst->frame_len <= sizeof(frame->array))
                    memcpy(frame->array, inst->rxbuf, inst->frame_len);
                else
                    break;
                // IEEE 802.15.4 standard ranging frames, software MAC filtering
                if (inst->config.rx.frameFilter == 0 && frame->dst_address != inst->my_short_address){
                    if(dpl_sem_get_count(&rng->sem) == 0){
                        dpl_sem_release(&rng->sem);
                    }
                    return true;
                }else{
                    RNG_STATS_INC(rx_complete);
                    rng->idx++;     // confirmed frame advance
                    return false;   // Allow sub extensions to handle event
                }
            }
            break;
        default:
            return false;
    }
    return false;
}

/**
 * @fn rng_issue_complete(struct uwb_dev * inst)
 * @brief Calls all complete_cb present in the struct uwb_mac_interface list
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return void
 */
void
rng_issue_complete(struct uwb_dev * inst)
{
    struct uwb_mac_interface * cbs_i;
    if(!(SLIST_EMPTY(&inst->interface_cbs))) {
        SLIST_FOREACH(cbs_i, &inst->interface_cbs, next) {
            if (cbs_i != NULL && cbs_i->complete_cb)
                if(cbs_i->complete_cb(inst, cbs_i)) continue;
        }
    }
}

/**
 * @fn tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for transmission complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return true on sucess
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    if (inst->fctrl != FCNTL_IEEE_RANGE_16)
        return false;

    if(dpl_sem_get_count(&rng->sem) == 1) {
        // unsolicited inbound
        return false;
    }

    switch(rng->code) {
        case UWB_DATA_CODE_SS_TWR ... UWB_DATA_CODE_DS_TWR_EXT_END:
            RNG_STATS_INC(tx_complete);
            if (rng->control.complete_after_tx) {
                dpl_sem_release(&rng->sem);
                rng_issue_complete(inst);
            }
            rng->control.complete_after_tx = 0;
            return true;
            break;
        default:
            return false;
    }
}

#if MYNEWT_VAL(RNG_VERBOSE)

/**
 * @fn complete_ev_cb(struct dpl_event *ev)
 * @brief API for rng complete event callback and print rng logs into json format.
 *
 * @param ev    Pointer to os_event.
 *
 * @return true on sucess
 */
static void
complete_ev_cb(struct dpl_event *ev)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)dpl_event_get_arg(ev);
    assert(ev != NULL);
    assert(rng);
    rng_encode(rng);
}

/**
 * @fn complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief Range complete callback, calculates local parameters for the completed rng op
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return true on sucess
 */
static bool
complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    twr_frame_t * frame;
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    RNG_STATS_INC(complete_cb);
    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED))
        return false;

    /* Calculate Local results and diagnostics.
     * XXX TODO: Generalise antenna distance */
    rng->idx_current = (rng->idx)%rng->nframes;
    frame = rng->frames[rng->idx_current];
    if (inst->capabilities.single_receiver_pdoa) {
        dpl_float32_t tmp_pdoa = uwb_calc_pdoa(inst, inst->rxdiag);
        if (!DPL_FLOAT32_ISNAN(tmp_pdoa)) {
            frame->local.pdoa = tmp_pdoa;
            frame->local.spherical.azimuth = DPL_FLOAT64_FROM_F32(
                uwb_calc_aoa(frame->local.pdoa, inst->config.channel, inst->rx_ant_separation)
                );
        }
    }

    frame->local.vrssi[0] = DPL_FLOAT32_NAN();
    frame->local.rssi = uwb_calc_rssi(inst, inst->rxdiag);
    frame->local.fppl = uwb_calc_fppl(inst, inst->rxdiag);

    if (inst->capabilities.sts) {
        frame->local.flags.has_sts = inst->config.rx.stsMode != DWT_STS_MODE_OFF;
        frame->local.flags.has_valid_sts = !(inst->status.sts_ts_error || inst->status.sts_pream_error);
        frame->local.vrssi[0] = frame->local.rssi;
        frame->local.vrssi[1] = uwb_calc_seq_rssi(inst, inst->rxdiag, UWB_RXDIAG_STS);
        frame->local.vrssi[2] = uwb_calc_seq_rssi(inst, inst->rxdiag, UWB_RXDIAG_STS2);
    }

    /* Postprocess in thread context */
    dpl_eventq_put(dpl_eventq_dflt_get(), &rng->complete_event);
    return false;
}
#endif

/**
 * API for final transmission to calculate range etc
 *
 * @param inst   Pointer to struct uwb_dev.
 *
 * @return void
 */
static bool
tx_final_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)cbs->inst_ptr;
    twr_frame_t * frame = rng->frames[(rng->idx)%rng->nframes];

    frame->remote.cartesian.x = DPL_FLOAT64_INIT(MYNEWT_VAL(LOCAL_COORDINATE_X));
    frame->remote.cartesian.y = DPL_FLOAT64_INIT(MYNEWT_VAL(LOCAL_COORDINATE_Y));
    frame->remote.cartesian.z = DPL_FLOAT64_INIT(MYNEWT_VAL(LOCAL_COORDINATE_Z));

    if (inst->capabilities.single_receiver_pdoa) {
        frame->remote.pdoa = uwb_calc_pdoa(inst, inst->rxdiag);
    }
    if (inst->capabilities.sts) {
        frame->remote.flags.has_sts = inst->config.rx.stsMode != DWT_STS_MODE_OFF;
        frame->local.flags.has_valid_sts = !(inst->status.sts_ts_error || inst->status.sts_pream_error);
    }
    frame->remote.rssi = uwb_calc_rssi(inst, inst->rxdiag);
    frame->remote.fppl = uwb_calc_fppl(inst, inst->rxdiag);

    if (frame->code != UWB_DATA_CODE_SS_TWR_EXT_T1) {
        /* TODO: Generalise to all tranceivers */
#if MYNEWT_VAL(DW1000_BIAS_CORRECTION_ENABLED)
        if (inst->config.bias_correction_enable){
            float range = uwb_rng_tof_to_meters(uwb_rng_twr_to_tof(rng,rng->idx));
            float bias = 2 * uwb_rng_bias_correction(inst,
                                                     uwb_rng_path_loss(
                                                         MYNEWT_VAL(DW1000_DEVICE_TX_PWR),
                                                         MYNEWT_VAL(DW1000_DEVICE_ANT_GAIN),
                                                         MYNEWT_VAL(DW1000_DEVICE_FREQ),
                                                         range)
                );
            frame->spherical.range = range - bias;
        }
#else
        frame->remote.spherical.range = uwb_rng_tof_to_meters(uwb_rng_twr_to_tof(rng,rng->idx));
#endif
        frame->remote.spherical_variance.range = DPL_FLOAT64_I32_TO_F64(MYNEWT_VAL(RANGE_VARIANCE));
    } else {
        frame->remote.spherical.range = DPL_FLOAT64_NAN();
    }
    frame->remote.spherical_variance.azimuth = DPL_FLOAT64_INIT(-1.0);
    frame->remote.spherical_variance.zenith = DPL_FLOAT64_INIT(-1.0);
    if (!DPL_FLOAT32_ISNAN(frame->remote.pdoa)) {
        frame->remote.spherical.azimuth = DPL_FLOAT64_FROM_F32(uwb_calc_aoa(
            frame->remote.pdoa, inst->config.channel, inst->rx_ant_separation)
            );
        frame->remote.spherical_variance.azimuth = DPL_FLOAT64_INIT(MYNEWT_VAL(AZIMUTH_VARIANCE));
    }

    return true;
}


/**
 * @fn uwb_rng_pkg_init(void)
 * @brief API to initialise the rng package.
 *
 * @return void
 */

void
uwb_rng_pkg_init(void)
{
    int i;
    struct uwb_rng_instance *rng;
    struct uwb_dev *udev;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"rng_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i=0;i<ARRAY_SIZE(g_cbs);i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) continue;
        g_cbs[i].inst_ptr = rng = uwb_rng_init(udev, &g_config, sizeof(g_twr_frames[i])/sizeof(g_twr_frames[i][0]));
        uwb_rng_set_frames(rng, g_twr_frames[i], sizeof(g_twr_frames[0])/sizeof(g_twr_frames[0][0]));
        uwb_mac_append_interface(udev, &g_cbs[i]);

#if MYNEWT_VAL(RNG_VERBOSE)
        dpl_event_init(&rng->complete_event, complete_ev_cb, (void*)rng);
#endif
#if __KERNEL__
        rng_chrdev_create(udev->idx);
#endif
    }

}

void
uwb_rng_pkg_init2(void)
{
#ifdef __KERNEL__
    int i;
    for (i=0;i<ARRAY_SIZE(g_cbs);i++) {
        if (!g_cbs[i].inst_ptr) continue;
        uwbrng_sysfs_init((struct uwb_rng_instance *)g_cbs[i].inst_ptr);
    }
#endif
}

int
uwb_rng_pkg_down(int reason)
{
    int i;
    struct uwb_rng_instance * rng;

    for (i=0;i<ARRAY_SIZE(g_cbs);i++) {
        rng = (struct uwb_rng_instance *)g_cbs[i].inst_ptr;
        if (!rng) continue;
#if __KERNEL__
        rng_chrdev_destroy(rng->dev_inst->idx);
        uwbrng_sysfs_deinit(rng->dev_inst->idx);
#endif
        uwb_mac_remove_interface(rng->dev_inst, g_cbs[i].id);
        uwb_rng_free(g_cbs[i].inst_ptr);
        g_cbs[i].inst_ptr = 0;
    }

    return 0;
}
