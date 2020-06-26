/**
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <uwb/uwb.h>
#include <sysinit/sysinit.h>
#include <uwb_rng/uwb_rng.h>
#include <dpl/dpl.h>
#if MYNEWT_VAL(TDMA_ENABLED)
#include <tdma/tdma.h>
#endif
#if MYNEWT_VAL(UWB_CCP_ENABLED)
#include <uwb_ccp/uwb_ccp.h>
#endif
#include <syscfg/syscfg.h>
#include <shell/shell.h>
#include "control.h"

#include "uwbcfg/uwbcfg.h"
static bool g_uwb_config_needs_update = false;
static int
uwb_config_updated()
{
    g_uwb_config_needs_update = true;
    return 0;
}

static struct uwbcfg_cbs uwb_cb = {
    .uc_update = uwb_config_updated
};

#if defined(ANDROID_APK_BUILD)
pthread_t eventq_t;

void * read_eventq(void * p)
{
    while(1) {
        dpl_eventq_run(dpl_eventq_dflt_get());
    }
}
#endif

static int ctrl_cli_cmd(int argc, char **argv);

static struct shell_cmd shell_ctrl_cmd = {
    .sc_cmd = "ctrl",
    .sc_cmd_func = ctrl_cli_cmd,
};

static bool error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static void slot_complete_cb(struct dpl_event * ev);
static bool complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static struct uwb_mac_interface g_cbs = (struct uwb_mac_interface){
    .id = UWBEXT_APP0,
    .complete_cb = complete_cb,
};

struct dwm_cb g_dwm_cb = {NULL} ;
char json_string[128];

void * cb_register(dwm_cb_fn_ptr cb)
{
    g_dwm_cb.call_back = cb;
    return NULL;
}

/*!
 * @fn complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 *
 * @brief This callback is part of the  struct uwb_mac_interface extension interface and invoked of the completion of a range request
 * in the context of this example. The struct uwb_mac_interface is in the interrupt context and is used to schedule events an event queue.
 * Processing should be kept to a minimum giving the interrupt context. All algorithms activities should be deferred to a thread on an event queue.
 * The callback should return true if and only if it can determine if it is the sole recipient of this event.
 *
 * NOTE: The MAC extension interface is a link-list of callbacks, subsequent callbacks on the list will be not be called in the
 * event of returning true.
 *
 * @param inst  - struct uwb_dev *
 * @param cbs   - struct uwb_mac_interface *
 *
 * output parameters
 *
 * returns bool
 */
/* The timer callout */
static struct dpl_callout tx_callout;
uint8_t ranging_enabled_flag = 0;
static struct dpl_event slot_event = {0};
static uint16_t g_idx_latest;

#define ANTENNA_SEPERATION 0.0205f

static bool
complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED)) {
        return false;
    }
    struct uwb_rng_instance* rng = (struct uwb_rng_instance*)cbs->inst_ptr;
    twr_frame_t * frame = rng->frames[rng->idx_current];

    snprintf(json_string, sizeof(json_string), "{\"raz\":[%lf,%lf,%lf]}",
             frame->local.spherical.range,
             frame->local.spherical.azimuth,
             frame->local.spherical.zenith);
    LOGD("%s", json_string);
    LOGD("Stack pointer : %p", json_string);

    g_idx_latest = (rng->idx)%rng->nframes; // Store valid frame pointer
    dpl_eventq_put(dpl_eventq_dflt_get(), &slot_event);
    return true;
}


/*!
 * @fn slot_complete_cb(struct dpl_event * ev)
 *
 * @brief In the example this function represents the event context processing of the received range request.
 * In this case, a JSON string is constructed and written to stdio. See the ./apps/matlab or ./apps/python folders for examples on
 * how to parse and render these results.
 *
 * input parameters
 * @param inst - struct dpl_event *
 * output parameters
 * returns none
 */

static void slot_complete_cb(struct dpl_event *ev)
{
    assert(ev != NULL);

    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)dpl_event_get_arg(ev);
    struct uwb_dev * inst = rng->dev_inst;

    if(g_dwm_cb.call_back != NULL) {
        g_dwm_cb.call_back(json_string);
    }

    if (inst->role&UWB_ROLE_ANCHOR) {
        uwb_rng_listen(rng, 0xfffff, UWB_NONBLOCKING);
    }
}


/*!
 * @fn error_cb(struct dpl_event *ev)
 *
 * @brief This callback is in the interrupt context and is called on error event.
 * In this example just log event.
 * Note: interrupt context so overlapping IO is possible
 * input parameters
 * @param inst - struct uwb_dev * inst
 *
 * output parameters
 *
 * returns none
 */
static bool
error_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    if (inst->fctrl != FCNTL_IEEE_RANGE_16 &&
        inst->fctrl != (FCNTL_IEEE_RANGE_16|UWB_FCTRL_ACK_REQUESTED)) {
        return false;
    }

    uint32_t utime = os_cputime_ticks_to_usecs(os_cputime_get32());
    if (inst->status.start_rx_error)
        printf("{\"utime\": %"PRIu32",\"msg\": \"start_rx_error,%s:%d\"}\n",utime, __FILE__, __LINE__);
    if (inst->status.start_tx_error)
        printf("{\"utime\": %"PRIu32",\"msg\": \"start_tx_error,%s:%d\"}\n",utime, __FILE__, __LINE__);
    if (inst->status.rx_error)
        printf("{\"utime\": %"PRIu32",\"msg\": \"rx_error,%s:%d\"}\n",utime, __FILE__, __LINE__);

    return true;
}


static void
uwb_ev_cb(struct dpl_event *ev)
{
    struct uwb_rng_instance * rng = (struct uwb_rng_instance *)ev->ev_arg;
    struct uwb_dev * inst = rng->dev_inst;

    if (inst->role&UWB_ROLE_ANCHOR) {
        if(dpl_sem_get_count(&rng->sem) == 1){
            if (g_uwb_config_needs_update) {
                uwb_mac_config(inst, NULL);
                uwb_txrf_config(inst, &inst->config.txrf);

                inst->my_short_address = MYNEWT_VAL(ANCHOR_ADDRESS);
                uwb_set_uid(inst, inst->my_short_address);
                g_uwb_config_needs_update = false;
            }

            uwb_rng_listen(rng, 0xfffff, UWB_NONBLOCKING);
        }
    } else {
        int mode_v[8] = {0}, mode_i=0, mode=-1;
        static int last_used_mode = 0;

        if (g_uwb_config_needs_update) {
            uwb_mac_config(inst, NULL);
            uwb_txrf_config(inst, &inst->config.txrf);

            inst->my_short_address = inst->euid&0xffff;
            uwb_set_uid(inst, inst->my_short_address);
            g_uwb_config_needs_update = false;
        }
        /* Should only really be needed when transitioning from node to tag */
        if(dpl_sem_get_count(&rng->sem) == 0){
            uwb_phy_forcetrxoff(inst);
        }
#if MYNEWT_VAL(TWR_SS_ENABLED)
        mode_v[mode_i++] = UWB_DATA_CODE_SS_TWR;
#endif
#if MYNEWT_VAL(TWR_SS_ACK_ENABLED)
        mode_v[mode_i++] = UWB_DATA_CODE_SS_TWR_ACK;
#endif
#if MYNEWT_VAL(TWR_SS_EXT_ENABLED)
        mode_v[mode_i++] = UWB_DATA_CODE_SS_TWR_EXT;
#endif
#if MYNEWT_VAL(TWR_DS_ENABLED)
        mode_v[mode_i++] = UWB_DATA_CODE_DS_TWR;
#endif
#if MYNEWT_VAL(TWR_DS_EXT_ENABLED)
        mode_v[mode_i++] = UWB_DATA_CODE_DS_TWR_EXT;
#endif
        if (++last_used_mode >= mode_i) last_used_mode=0;
        mode = mode_v[last_used_mode];
        /* Uncomment the next line to force the range mode */
        mode = UWB_DATA_CODE_SS_TWR_ACK;
        if (mode>0) {
            uwb_rng_request(rng, MYNEWT_VAL(ANCHOR_ADDRESS), mode);
        }
    }
    if(ranging_enabled_flag) {
        dpl_callout_reset(&tx_callout, DPL_TICKS_PER_SEC/64);
    }
}


void *
init_ranging(void * arg)
{
#if defined(ANDROID_APK_BUILD)
    LOGD("Init ranging\n");
    sysinit();
    LOGD("Sys init OK\n");
#endif

    struct uwb_dev *udev = uwb_dev_idx_lookup(0);
    struct uwb_rng_instance* rng = (struct uwb_rng_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_RNG);
    assert(rng);

    uwbcfg_register(&uwb_cb);
    g_cbs.inst_ptr = rng;
    uwb_mac_append_interface(udev, &g_cbs);

    dpl_event_init(&slot_event, slot_complete_cb, rng);

    uint32_t utime = os_cputime_ticks_to_usecs(os_cputime_get32());
    LOGD("{\"utime\": %"PRIu32",\"exec\": \"%s\"}\n",utime,__FILE__);
    LOGD("{\"device_id\"=\"%"PRIx32"\"",udev->device_id);
    LOGD(",\"panid=\"%X\"",udev->pan_id);
    LOGD(",\"addr\"=\"%X\"",udev->uid);
    LOGD(",\"part_id\"=\"%"PRIx32"\"",(uint32_t)(udev->euid&0xffffffff));
    LOGD(",\"lot_id\"=\"%"PRIx32"\"}\n",(uint32_t)(udev->euid>>32));
    printf("{\"utime\": %"PRIu32",\"exec\": \"%s\"}\n",utime,__FILE__);
    printf("{\"device_id\"=\"%"PRIx32"\"",udev->device_id);
    printf(",\"panid=\"%X\"",udev->pan_id);
    printf(",\"addr\"=\"%X\"",udev->uid);
    printf(",\"part_id\"=\"%"PRIx32"\"",(uint32_t)(udev->euid&0xffffffff));
    printf(",\"lot_id\"=\"%"PRIx32"\"}\n",(uint32_t)(udev->euid>>32));

    if (udev->device_id == 0) {
        printf("### Error: could not detect device\n");
        exit(1);
    }
#if MYNEWT_VAL(RNG_VERBOSE) > 1
    udev->config.rxdiag_enable = 1;
#endif

#if MYNEWT_VAL(TWR_SS_ACK_ENABLED)
    uwb_set_autoack(udev, true);
    uwb_set_autoack_delay(udev, 12);
#endif

    dpl_callout_init(&tx_callout, dpl_eventq_dflt_get(), uwb_ev_cb, rng);

    /* Apply config */
    uwb_mac_config(udev, NULL);
    uwb_txrf_config(udev, &udev->config.txrf);

    if ((udev->role&UWB_ROLE_ANCHOR)) {
        udev->my_short_address = MYNEWT_VAL(ANCHOR_ADDRESS);
        uwb_set_uid(udev, udev->my_short_address);

        uwb_rng_listen(rng, 0xfffff, UWB_NONBLOCKING);
        dpl_callout_reset(&tx_callout, DPL_TICKS_PER_SEC/25);
    } else {
        printf("\n\n   Enter ctrl r  to start ranging \n");
        printf("\n\n   Enter ctrl q  to stop  ranging \n");
    }
    shell_cmd_register(&shell_ctrl_cmd);

#if defined(ANDROID_APK_BUILD)
    pthread_create(&eventq_t, NULL, read_eventq, NULL);
#endif
    return arg;
}

void * start_ranging(void* arg)
{
    LOGD("Start ranging\n");
    dpl_callout_reset(&tx_callout, DPL_TICKS_PER_SEC/25);
    ranging_enabled_flag = 1;
    return arg;
}

void * stop_ranging(void* arg)
{
    LOGD("Stop ranging\n");
    dpl_callout_stop(&tx_callout);
    ranging_enabled_flag = 0;
    return arg;
}

static int
ctrl_cli_cmd(int argc, char **argv)
{
    if (!strcmp(argv[1], "r")) {
        start_ranging(NULL);
    } else if (!strcmp(argv[1], "q")) {
        stop_ranging(NULL);
    }
    return 0;
}
