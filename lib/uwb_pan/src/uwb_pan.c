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
 * @file uwb_pan.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Personal Area Network
 *
 * @details This is the pan base class which utilises the functions to allocate/deallocate the resources on pan_master,sets callbacks, enables  * blink_requests.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <os/os.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>

#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif
#if MYNEWT_VAL(TDMA_ENABLED)
#include <tdma/tdma.h>
#endif
#if MYNEWT_VAL(UWB_PAN_VERSION_ENABLED)
#include <bootutil/image.h>
#include <imgmgr/imgmgr.h>
#endif

// #define DIAGMSG(s,u) printf(s,u)
#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif

#if MYNEWT_VAL(UWB_PAN_ENABLED)
#include <uwb_pan/uwb_pan.h>

//! Buffers for pan frames
#if MYNEWT_VAL(UWB_DEVICE_0)
static union pan_frame_t g_pan_0[] = {
    [0] = {
        .fctrl = FCNTL_IEEE_BLINK_TAG_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 16-bit addressing).
        .seq_num = 0x0,
    },
    [1] = {
        .fctrl = FCNTL_IEEE_BLINK_TAG_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 16-bit addressing).
    }
};
#endif
#if MYNEWT_VAL(UWB_DEVICE_1)
static union pan_frame_t g_pan_1[] = {
    [0] = {
        .fctrl = FCNTL_IEEE_BLINK_TAG_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 16-bit addressing).
        .seq_num = 0x0,
    },
    [1] = {
        .fctrl = FCNTL_IEEE_BLINK_TAG_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 16-bit addressing).
    }
};
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
static union pan_frame_t g_pan_2[] = {
    [0] = {
        .fctrl = FCNTL_IEEE_BLINK_TAG_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 16-bit addressing).
        .seq_num = 0x0,
    },
    [1] = {
        .fctrl = FCNTL_IEEE_BLINK_TAG_64,    // frame control (FCNTL_IEEE_BLINK_64 to indicate a data frame using 16-bit addressing).
    }
};
#endif

STATS_SECT_DECL(pan_stat_section) g_stat;
STATS_NAME_START(pan_stat_section)
    STATS_NAME(pan_stat_section, pan_request)
    STATS_NAME(pan_stat_section, pan_listen)
    STATS_NAME(pan_stat_section, pan_reset)
    STATS_NAME(pan_stat_section, relay_tx)
    STATS_NAME(pan_stat_section, lease_expiry)
    STATS_NAME(pan_stat_section, tx_complete)
    STATS_NAME(pan_stat_section, rx_complete)
    STATS_NAME(pan_stat_section, rx_unsolicited)
    STATS_NAME(pan_stat_section, rx_other_frame)
    STATS_NAME(pan_stat_section, rx_error)
    STATS_NAME(pan_stat_section, tx_error)
    STATS_NAME(pan_stat_section, rx_timeout)
    STATS_NAME(pan_stat_section, reset)
STATS_NAME_END(pan_stat_section)

static struct uwb_pan_config_t g_config = {
    .tx_holdoff_delay = MYNEWT_VAL(UWB_PAN_TX_HOLDOFF),         // Send Time delay in usec.
    .rx_timeout_period = MYNEWT_VAL(UWB_PAN_RX_TIMEOUT),        // Receive response timeout in usec.
    .lease_time = MYNEWT_VAL(UWB_PAN_LEASE_TIME),               // Lease time in seconds
    .network_role = MYNEWT_VAL(UWB_PAN_NETWORK_ROLE)            // Role in the network (Anchor/Tag/...)
};

static bool rx_complete_cb(struct uwb_dev * udev, struct uwb_mac_interface * cbs);
static bool tx_complete_cb(struct uwb_dev * udev, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * udev, struct uwb_mac_interface * cbs);
static bool reset_cb(struct uwb_dev * udev, struct uwb_mac_interface * cbs);
static void pan_postprocess(struct dpl_event * ev);
static void lease_expiry_cb(struct dpl_event * ev);

static struct uwb_mac_interface g_cbs[] = {
        [0] = {
            .id = UWBEXT_PAN,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .reset_cb = reset_cb
        },
#if MYNEWT_VAL(UWB_DEVICE_1)
        [1] = {
            .id = UWBEXT_PAN,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .reset_cb = reset_cb
        },
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
        [2] = {
            .id = UWBEXT_PAN,
            .rx_complete_cb = rx_complete_cb,
            .tx_complete_cb = tx_complete_cb,
            .rx_timeout_cb = rx_timeout_cb,
            .reset_cb = reset_cb
        }
#endif
};

/**
 * @fn uwb_pan_init(struct uwb_dev * inst,  uwb_pan_config_t * config, uint16_t nframes)
 * @brief API to initialise pan parameters.
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param config   Pointer to uwb_pan_config_t.
 * @param nrfames  number of frames defined to store pan frames.
 *
 * @return struct uwb_pan_instance
 */
struct uwb_pan_instance *
uwb_pan_init(struct uwb_dev * inst,  struct uwb_pan_config_t * config, uint16_t nframes)
{
    assert(inst);

    struct uwb_pan_instance *pan = (struct uwb_pan_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_PAN);
    if (pan == NULL ) {
        pan = (struct uwb_pan_instance *) malloc(sizeof(struct uwb_pan_instance) + nframes * sizeof(union pan_frame_t *));
        assert(pan);
        memset(pan, 0, sizeof(struct uwb_pan_instance));
        pan->status.selfmalloc = 1;
        pan->nframes = nframes;
    }

    pan->dev_inst = inst;
    pan->config = config;
    pan->control = (struct uwb_pan_control_t){0};

    dpl_error_t err = dpl_sem_init(&pan->sem, 0x1);
    assert(err == DPL_OK);

    uwb_pan_set_postprocess(pan, pan_postprocess);
    pan->request_cb = 0;

    pan->status.valid = true;
    pan->status.initialized = 1;
    return pan;
}

/**
 * @fn uwb_pan_set_frames(struct uwb_dev * inst, pan_frame_t pan[], uint16_t nframes)
 * @brief API to set the pointer to the frame buffers.
 *
 * @param inst      Pointer to struct uwb_dev.
 * @param twr[]     Pointer to frame buffers.
 * @param nframes   Number of buffers defined to store the discovery data.
 *
 * @return void
 */
void
uwb_pan_set_frames(struct uwb_pan_instance *pan, union pan_frame_t pan_f[], uint16_t nframes)
{
    assert(nframes <= pan->nframes);
    for (uint16_t i = 0; i < nframes; i++)
        pan->frames[i] = &pan_f[i];
}

/**
 * @fn pan_pkg_init(void)
 * @brief API to initialise the pan package.
 *
 * @return void
 */
void
uwb_pan_pkg_init(void)
{
    struct uwb_dev *udev;
    struct uwb_pan_instance *pan;
    printf("{\"utime\": %"PRIu32",\"msg\": \"pan_pkg_init\"}\n", dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));

    dpl_error_t rc = stats_init(
        STATS_HDR(g_stat),
        STATS_SIZE_INIT_PARMS(g_stat, STATS_SIZE_32),
        STATS_NAME_INIT_PARMS(pan_stat_section)
    );
    rc |= stats_register("pan", STATS_HDR(g_stat));
    assert(rc == DPL_OK);

#if MYNEWT_VAL(UWB_DEVICE_0)
    udev = uwb_dev_idx_lookup(0);
    g_cbs[0].inst_ptr = pan = uwb_pan_init(udev, &g_config, sizeof(g_pan_0)/sizeof(union pan_frame_t));
    uwb_pan_set_frames(pan, g_pan_0, sizeof(g_pan_0)/sizeof(union pan_frame_t));
    uwb_mac_append_interface(udev, &g_cbs[0]);
    dpl_callout_init(&pan->pan_lease_callout_expiry, dpl_eventq_dflt_get(), lease_expiry_cb, (void *) pan);
#endif
#if MYNEWT_VAL(UWB_DEVICE_1)
    udev = uwb_dev_idx_lookup(1);
    g_cbs[1].inst_ptr = pan = uwb_pan_init(udev, &g_config, sizeof(g_pan_1)/sizeof(union pan_frame_t));
    uwb_pan_set_frames(pan, g_pan_1, sizeof(g_pan_1)/sizeof(union pan_frame_t));
    uwb_mac_append_interface(udev, &g_cbs[1]);
    dpl_callout_init(&pan->pan_lease_callout_expiry, dpl_eventq_dflt_get(), lease_expiry_cb, (void *) pan);
#endif
#if MYNEWT_VAL(UWB_DEVICE_2)
    udev = uwb_dev_idx_lookup(2);
    g_cbs[2].inst_ptr = pan = uwb_pan_init(udev, &g_config, sizeof(g_pan_2)/sizeof(union pan_frame_t));
    uwb_pan_set_frames(pan, g_pan_2, sizeof(g_pan_2)/sizeof(union pan_frame_t));
    uwb_mac_append_interface(udev, &g_cbs[2]);
    dpl_callout_init(&pan->pan_lease_callout_expiry, dpl_eventq_dflt_get(), lease_expiry_cb, (void *) pan);
#endif
}

/**
 * @fn uwb_pan_free(struct uwb_dev * inst)
 * @brief API to free pan resources.
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return void
 */
void
uwb_pan_free(struct uwb_pan_instance *pan)
{
    assert(pan);
    uwb_mac_remove_interface(pan->dev_inst, pan->cbs.id);
    if (pan->status.selfmalloc) {
        free(pan);
    } else {
        pan->status.initialized = 0;
    }
}

/**
 * @fn pan_postprocess(struct dpl_event * ev)
 * @brief This a template which should be replaced by the pan_master by a event that tracks UUIDs
 * and allocated PANIDs and SLOTIDs.
 *
 * @param ev  Pointer to dpl_events.
 *
 * @return void
 */
static void
pan_postprocess(struct dpl_event * ev){
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));


#if MYNEWT_VAL(UWB_PAN_VERBOSE)
    struct uwb_pan_instance * pan = (struct uwb_pan_instance *)ev->ev_arg;
    struct uwb_dev * inst = pan->dev_inst;
    union pan_frame_t * frame = pan->frames[(pan->idx)%pan->nframes];
    if(pan->status.valid && frame->long_address == inst->my_long_address)
        printf("{\"utime\": %lu,\"UUID\": \"%llX\",\"ID\": \"%X\",\"PANID\": \"%X\",\"slot\": %d}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()),
            frame->long_address,
            frame->short_address,
            frame->pan_id,
            frame->slot_id
        );
    else if (frame->code == DWT_PAN_REQ)
        printf("{\"utime\": %lu,\"UUID\": \"%llX\",\"seq_num\": %d}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()),
            frame->long_address,
            frame->seq_num
        );
    else if (frame->code == DWT_PAN_RESP)
        printf("{\"utime\": %lu,\"UUID\": \"%llX\",\"ID\": \"%X\",\"PANID\": \"%X\",\"slot\": %d}\n",
            dpl_cputime_ticks_to_usecs(dpl_cputime_get32()),
            frame->long_address,
            frame->short_address,
            frame->pan_id,
            frame->slot_id
        );
#endif
}

/**
 * @fn lease_expiry_cb(struct dpl_event * ev)
 * @brief Function called when our lease is about to expire
 *
 * @param ev  Pointer to dpl_events.
 *
 * @return void
 */
static void
lease_expiry_cb(struct dpl_event * ev)
{
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev));

    struct uwb_pan_instance * pan = (struct uwb_pan_instance *) dpl_event_get_arg(ev);
    STATS_INC(g_stat, lease_expiry);
    pan->status.valid = false;
    pan->status.lease_expired = true;
    pan->dev_inst->slot_id = 0xffff;

    DIAGMSG("{\"utime\": %lu,\"msg\": \"pan_lease_expired\"}\n",dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    if (pan->control.postprocess) {
        dpl_eventq_put(&pan->dev_inst->eventq, &pan->postprocess_event);
    }
}


static void
handle_pan_request(struct uwb_pan_instance * pan, union pan_frame_t * request)
{
    if (!pan->request_cb) {
        return;
    }

    union pan_frame_t * response = pan->frames[(pan->idx)%pan->nframes];
    response->code = DWT_PAN_RESP;

    if (pan->request_cb(request->long_address, &request->req, &response->req)) {
        uwb_set_wait4resp(pan->dev_inst, false);
        uwb_write_tx_fctrl(pan->dev_inst, sizeof(union pan_frame_t), 0);
        uwb_write_tx(pan->dev_inst, response->array, 0, sizeof(union pan_frame_t));
        pan->status.start_tx_error = uwb_start_tx(pan->dev_inst).start_tx_error;
    }
}

/**
 * @fn rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief This is an internal static function that executes on both the pan_master Node and the TAG/ANCHOR
 * that initiated the blink. On the pan_master the postprocess function should allocate a PANID and a SLOTID,
 * while on the TAG/ANCHOR the returned allocations are assigned and the PAN discover event is stopped. The pan
 * discovery resources can be released.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param cbs     Pointer to struct uwb_mac_interface.
 *
 * @return bool
 */
static bool
rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_pan_instance * pan = (struct uwb_pan_instance *)cbs->inst_ptr;
    if(inst->fctrl_array[0] != FCNTL_IEEE_BLINK_TAG_64) {
        if (dpl_sem_get_count(&pan->sem) == 0) {
            STATS_INC(g_stat, rx_other_frame);
            dpl_sem_release(&pan->sem);
            return false;
        }
        return false;
    }

    if (dpl_sem_get_count(&pan->sem) == 1){
        /* Unsolicited */
        STATS_INC(g_stat, rx_unsolicited);
        return false;
    }

    STATS_INC(g_stat, rx_complete);
    union pan_frame_t * frame = pan->frames[(pan->idx)%pan->nframes];

    /* Ignore frames that are too long */
    if (inst->frame_len > sizeof(union pan_frame_t)) {
        return false;
    }
    memcpy(frame->array, inst->rxbuf, inst->frame_len);

    if (pan->config->role == UWB_PAN_ROLE_RELAY &&
        frame->rpt_count < frame->rpt_max &&
        frame->long_address != inst->my_long_address) {
        frame->rpt_count++;
        uwb_set_wait4resp(inst, true);
        uwb_write_tx_fctrl(inst, inst->frame_len, 0);
        pan->status.start_tx_error = uwb_start_tx(inst).start_tx_error;
        uwb_write_tx(inst, frame->array, 0, inst->frame_len);
        STATS_INC(g_stat, relay_tx);
    }

    switch(frame->code) {
    case DWT_PAN_REQ:
        STATS_INC(g_stat, pan_request);
        if (pan->config->role == UWB_PAN_ROLE_MASTER) {
            /* Prevent another request coming in whilst processing this one */
            uwb_stop_rx(inst);
            handle_pan_request(pan, frame);
        } else {
            return true;
        }
        break;
    case DWT_PAN_RESP:
        if(frame->long_address == inst->my_long_address){
            /* TAG/ANCHOR side */
            inst->uid = frame->req.short_address;
            inst->pan_id = frame->req.pan_id;
            inst->slot_id = frame->req.slot_id;
            pan->status.valid = true;
            pan->status.lease_expired = false;
            dpl_callout_stop(&pan->pan_lease_callout_expiry);
            if (frame->req.lease_time > 0) {
                /* Calculate when our lease expires */
                uint32_t exp_tics;
                uint32_t lease_us = 1000000;
                lease_us = (uint32_t)(frame->req.lease_time)*1000000;
#if MYNEWT_VAL(UWB_CCP_ENABLED)
                struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
                lease_us -= (inst->rxtimestamp>>16) - (ccp->local_epoch>>16);
#endif
                dpl_time_ms_to_ticks(lease_us/1000, &exp_tics);
                dpl_callout_reset(&pan->pan_lease_callout_expiry, exp_tics);
            }
        } else {
            return true;
        }
        break;
    case DWT_PAN_RESET:
        STATS_INC(g_stat, pan_reset);
        if (pan->config->role != UWB_PAN_ROLE_MASTER) {
            pan->status.valid = false;
            pan->status.lease_expired = true;
            inst->slot_id = 0xffff;
            dpl_callout_stop(&pan->pan_lease_callout_expiry);
        } else {
            return false;
        }
    default:
        return false;
        break;
    }

    /* Postprocess, all roles */
    if (pan->control.postprocess) {
        dpl_eventq_put(&inst->eventq, &pan->postprocess_event);
    }

    /* Release sem */
    if (dpl_sem_get_count(&pan->sem) == 0) {
        dpl_error_t err = dpl_sem_release(&pan->sem);
        assert(err == DPL_OK);
    }
    return true;
}

/**
 * @fn tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for transmit complete callback.
 *
 * @param inst  Pointer to struct uwb_dev.
 * @param cbs   Pointer to struct uwb_mac_interface.
 *
 * @return bool
 */
static bool
tx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_pan_instance * pan = (struct uwb_pan_instance *)cbs->inst_ptr;
    if(inst->fctrl_array[0] != FCNTL_IEEE_BLINK_TAG_64){
        return false;
    }
    pan->idx++;
    STATS_INC(g_stat, tx_complete);
    return true;
}

/**
 * @fn reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for reset callback.
 *
 * @param inst   Pointer to struct uwb_dev.
 * @param cbs    Pointer to struct uwb_mac_interface.
 *
 * @return bool
 */
static bool
reset_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_pan_instance * pan = (struct uwb_pan_instance *)cbs->inst_ptr;
    if (dpl_sem_get_count(&pan->sem) == 0){
        STATS_INC(g_stat, reset);
        dpl_error_t err = dpl_sem_release(&pan->sem);
        assert(err == DPL_OK);
        return true;
    }
    return false;
}

/**
 * @fn rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 * @brief API for receive timeout callback.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param cbs     Pointer to struct uwb_mac_interface.
 *
 * @return bool
 */
static bool
rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_pan_instance * pan = (struct uwb_pan_instance *)cbs->inst_ptr;
    if (dpl_sem_get_count(&pan->sem) == 0){
        STATS_INC(g_stat, rx_timeout);
        dpl_error_t err = dpl_sem_release(&pan->sem);
        assert(err == DPL_OK);
        return true;
    }
    return false;
}

/**
 * @fn uwb_pan_listen(struct uwb_dev * inst, uwb_dev_modes_t mode)
 * @brief Listen for PAN requests / resets
 *
 * @param inst          Pointer to struct uwb_dev.
 * @param mode          uwb_dev_modes_t of UWB_BLOCKING and UWB_NONBLOCKING.
 *
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
uwb_pan_listen(struct uwb_pan_instance * pan, uwb_dev_modes_t mode)
{
    dpl_error_t err = dpl_sem_pend(&pan->sem,  DPL_TIMEOUT_NEVER);
    assert(err == DPL_OK);

    STATS_INC(g_stat, pan_listen);

    if(uwb_start_rx(pan->dev_inst).start_rx_error){
        STATS_INC(g_stat, rx_error);
        err = dpl_sem_release(&pan->sem);
        assert(err == DPL_OK);
    }

    if (mode == UWB_BLOCKING){
        err = dpl_sem_pend(&pan->sem, DPL_TIMEOUT_NEVER);
        assert(err == DPL_OK);
        err = dpl_sem_release(&pan->sem);
        assert(err == DPL_OK);
    }

    return pan->dev_inst->status;
}

/**
 * @fn uwb_pan_blink(struct uwb_dev * inst, uint16_t role, uwb_dev_modes_t mode, uint64_t delay)
 * @brief A Personal Area Network blink request is a discovery phase in which a TAG/ANCHOR seeks to discover
 * an available PAN Master. The outcome of this process is a PANID and SLOTID assignment.
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param role     Requested role in the network
 * @param mode     BLOCKING and NONBLOCKING modes of uwb_dev_modes_t.
 * @param delay    When to send this blink
 *
 * @return uwb_pan_status_t
 */
struct uwb_pan_status_t
uwb_pan_blink(struct uwb_pan_instance *pan, uint16_t role,
                 uwb_dev_modes_t mode, uint64_t delay)
{
    dpl_error_t err = dpl_sem_pend(&pan->sem,  DPL_TIMEOUT_NEVER);
    assert(err == DPL_OK);

    STATS_INC(g_stat, pan_request);
    union pan_frame_t * frame = pan->frames[(pan->idx)%pan->nframes];

    frame->seq_num += pan->nframes;
    frame->long_address = pan->dev_inst->euid;
    frame->code = DWT_PAN_REQ;
    frame->rpt_count = 0;
    frame->rpt_max = MYNEWT_VAL(UWB_PAN_RPT_MAX);
    frame->req.role = role;
    frame->req.lease_time = pan->config->lease_time;

#if MYNEWT_VAL(UWB_PAN_VERSION_ENABLED)
    struct image_version iv;
    imgr_my_version(&iv);
    frame->req.fw_ver.iv_major = iv.iv_major;
    frame->req.fw_ver.iv_minor = iv.iv_minor;
    frame->req.fw_ver.iv_revision = iv.iv_revision;
    frame->req.fw_ver.iv_build_num = iv.iv_build_num;
#endif

    uwb_set_delay_start(pan->dev_inst, delay);
    uwb_write_tx_fctrl(pan->dev_inst, sizeof(union pan_frame_t), 0);
    uwb_write_tx(pan->dev_inst, frame->array, 0, sizeof(union pan_frame_t));
    uwb_set_wait4resp(pan->dev_inst, true);
    uwb_set_rx_timeout(pan->dev_inst, pan->config->rx_timeout_period);
    pan->status.start_tx_error = uwb_start_tx(pan->dev_inst).start_tx_error;

    if (pan->status.start_tx_error){
        STATS_INC(g_stat, tx_error);
        DIAGMSG("{\"utime\": %lu,\"msg\": \"pan_blnk_txerr\"}\n",dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
        // Half Period Delay Warning occured try for the next epoch
        // Use seq_num to detect this on receiver size
        dpl_sem_release(&pan->sem);
    }
    else if(mode == UWB_BLOCKING){
        err = dpl_sem_pend(&pan->sem, DPL_TIMEOUT_NEVER); // Wait for completion of transactions
        dpl_sem_release(&pan->sem);
        assert(err == DPL_OK);
    }
    return pan->status;
}

/**
 * @fn uwb_pan_reset(struct uwb_dev * inst, uint64_t delay)
 * @brief A Pan reset message is a broadcast to all nodes having a pan assigned address
 * instructing them to reset and renew their address. Normally issued by a restarted
 * master.
 *
 * @param inst     Pointer to struct uwb_dev.
 * @param delay    When to send this reset
 *
 * @return uwb_pan_status_t
 */
struct uwb_pan_status_t
uwb_pan_reset(struct uwb_pan_instance * pan, uint64_t delay)
{
    union pan_frame_t * frame = pan->frames[(pan->idx)%pan->nframes];

    frame->seq_num += pan->nframes;
    frame->long_address = pan->dev_inst->euid;
    frame->code = DWT_PAN_RESET;

    uwb_set_delay_start(pan->dev_inst, delay);
    uwb_write_tx_fctrl(pan->dev_inst, sizeof(union pan_frame_t), 0);
    uwb_write_tx(pan->dev_inst, frame->array, 0, sizeof(union pan_frame_t));
    uwb_set_wait4resp(pan->dev_inst, false);
    pan->status.start_tx_error = uwb_start_tx(pan->dev_inst).start_tx_error;

    if (pan->status.start_tx_error){
        STATS_INC(g_stat, tx_error);
        DIAGMSG("{\"utime\": %lu,\"msg\": \"pan_reset_tx_err\"}\n", dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
    }
    return pan->status;
}

/**
 * @fn uwb_pan_start(struct uwb_dev * inst, uwb_pan_role_t role)
 * @brief A Personal Area Network blink is a discovery phase in which a TAG/ANCHOR seeks to discover
 * an available PAN Master. The pan_master does not
 * need to call this function.
 *
 * @param inst    Pointer to struct uwb_dev.
 * @param role    uwb_pan_role_t of UWB_PAN_ROLE_MASTER, UWB_PAN_ROLE_SLAVE ,PAN_ROLE_RELAY.
 * @param network_role network_role_t, The role in the network application (NETWORK_ROLE_ANCHOR, NETWORK_ROLE_TAG)
 *
 * @return void
 */
void
uwb_pan_start(struct uwb_pan_instance * pan, enum uwb_pan_role_t role, network_role_t network_role)
{
    pan->config->role = role;
    pan->config->network_role = network_role;

    if (pan->config->role == UWB_PAN_ROLE_MASTER) {
        /* Nothing for now */
    } else if (pan->config->role == UWB_PAN_ROLE_SLAVE) {
        pan->idx = 0x1;
        pan->status.valid = false;

#if MYNEWT_VAL(UWB_PAN_VERBOSE)
        printf("{\"utime\": %lu,\"PAN\": \"%s\"}\n",
               dpl_cputime_ticks_to_usecs(dpl_cputime_get32()),
               "Provisioning"
            );
#endif
    }
}

/**
 * @fn uwb_pan_lease_remaining(struct uwb_dev * inst)
 * @brief Checks time to lease expiry
 *
 * @param inst    Pointer to struct uwb_dev.
 *
 * @return uint32_t ms to expiry, 0 if already expired
 */
uint32_t
uwb_pan_lease_remaining(struct uwb_pan_instance * pan)
{
    dpl_time_t rt = dpl_callout_remaining_ticks(&pan->pan_lease_callout_expiry, dpl_time_get());
    return dpl_time_ticks_to_ms32(rt);
}


#if MYNEWT_VAL(TDMA_ENABLED)
/**
 * @fn uwb_pan_slot_timer_cb
 * @brief tdma slot handler for pan slots
 *
 * @param struct dpl_event* event pointer with argument set to the pan instance
 *
 * @return void
 */
void
uwb_pan_slot_timer_cb(struct dpl_event * ev)
{
    assert(ev);
    tdma_slot_t * slot = (tdma_slot_t *) dpl_event_get_arg(ev);

    tdma_instance_t * tdma = slot->parent;
    struct uwb_ccp_instance *ccp = tdma->ccp;
    struct uwb_pan_instance *pan = (struct uwb_pan_instance*)slot->arg;
    assert(pan);
    uint16_t idx = slot->idx;

    /* Check if we are to act as a Master Node in the network */
    if (tdma->dev_inst->role&UWB_ROLE_PAN_MASTER) {
        static uint8_t _pan_cycles = 0;

        /* Broadcast an initial reset message to clear all leases */
        if (_pan_cycles < 8) {
            _pan_cycles++;
            uwb_pan_reset(pan, tdma_tx_slot_start(tdma, idx));
        } else {
            uint64_t dx_time = tdma_rx_slot_start(tdma, idx);
            uwb_set_rx_timeout(tdma->dev_inst, 3*ccp->period/tdma->nslots/4);
            uwb_set_delay_start(tdma->dev_inst, dx_time);
            uwb_set_on_error_continue(tdma->dev_inst, true);
            uwb_pan_listen(pan, UWB_BLOCKING);
        }
    } else {
        /* Act as a slave Node in the network */
        if (pan->status.valid && uwb_pan_lease_remaining(pan)>MYNEWT_VAL(UWB_PAN_LEASE_EXP_MARGIN)) {
            /* Our lease is still valid - just listen */
            uint16_t timeout;
            if (pan->config->role == UWB_PAN_ROLE_RELAY) {
                timeout = 3*ccp->period/tdma->nslots/4;
            } else {
                /* Only listen long enough to get any resets from master */
                timeout = uwb_phy_frame_duration(tdma->dev_inst, sizeof(sizeof(union pan_frame_t)))
                    + MYNEWT_VAL(XTALT_GUARD);
            }
            uwb_set_rx_timeout(tdma->dev_inst, timeout);
            uwb_set_delay_start(tdma->dev_inst, tdma_rx_slot_start(tdma, idx));
            uwb_set_on_error_continue(tdma->dev_inst, true);
            if (uwb_pan_listen(pan, UWB_BLOCKING).start_rx_error) {
                STATS_INC(g_stat, rx_error);
            }
        } else {
            /* Subslot 0 is for master reset, subslot 1 is for sending requests */
            uint64_t dx_time = tdma_tx_slot_start(tdma, (float)idx+1.0f/16);
            uwb_pan_blink(pan, pan->config->network_role, UWB_BLOCKING, dx_time);
        }
    }
}
#endif // MYNEWT_VAL(TDMA_ENABLED)


#endif // PAN_ENABLED
