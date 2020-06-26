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

/**
 * @file wcs_timescale.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date Oct 20 2018
 * @brief Wireless Clock Synchronization
 *
 * @details This is a top-level package for managing Clock Calibration using Clock Calibration Packet (CCP).
 * In an RTLS system the Clock Master send a periodic blink which is received by the anchor nodes. The device driver model on the node
 * handles the ccp frame and schedules a callback for post-processing of the event. The Clock Calibration herein is an example
 * of this post-processing. In TDOA-base RTLS system clock synchronization is essential, this can be either wired or wireless depending on the requirements.
 * In the case of wireless clock synchronization clock skew is estimated from the CCP packets. Depending on the accuracy required and the
 * available computational resources two model is available; timescale or linear interpolation.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <os/os.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <dpl/dpl_cputime.h>
#include <os/os_dev.h>

#if MYNEWT_VAL(TIMESCALE_ENABLED)
#include <uwb/uwb.h>
#include <uwb_wcs/uwb_wcs.h>
#if MYNEWT_VAL(UWB_WCS_VERBOSE)
#include <uwb_wcs/wcs_json.h>
#endif
#include <timescale/timescale.h>
void wcs_encode(uint64_t epoch, struct uwb_wcs_instance * wcs);
static bool wcs_timescale_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
void wcs_timescale_ev(struct dpl_event * ev);


/*!
 * @fn wcs_timescale_init(struct uwb_wcs_instance * wcss)
 *
 * @brief Constructor.
 *
 * input parameters
 * @param wcs - struct uwb_wcs_instance *,
 *
 * output parameters
 *
 * returns struct uwb_wcs_instance *
 */

struct uwb_wcs_instance *
wcs_timescale_init(struct uwb_wcs_instance * wcs){

    struct uwb_ccp_instance * ccp = wcs->ccp;
    double q[] = { MYNEWT_VAL(TIMESCALE_QVAR) * 1.0l, MYNEWT_VAL(TIMESCALE_QVAR) * 0.1l, MYNEWT_VAL(TIMESCALE_QVAR) * 0.01l};
    double T = 1e-6l * MYNEWT_VAL(UWB_CCP_PERIOD);
    wcs->timescale = timescale_init(NULL, NULL, q, T);

    wcs->cbs = (struct uwb_mac_interface){
        .id = UWBEXT_WCS,
        .inst_ptr = (void*)wcs,
        .superframe_cb = wcs_timescale_cb
    };
    wcs->normalized_skew = (dpl_float64_t) 1.0l;
    wcs->fractional_skew = (dpl_float64_t) 0.0l;
    uwb_mac_append_interface(ccp->dev_inst, &wcs->cbs);
    uwb_wcs_set_postprocess(wcs, &wcs_timescale_ev);      // Using default process
    return wcs;
}

/**
 * @fn wcs_timescale_free(struct uwb_wcs_instance * inst)
 * @brief Deconstructor
 *
 * @param inst   Pointer to struct uwb_wcs_instance.
 * @return void
 */
void
wcs_timescale_free(struct uwb_wcs_instance * wcs)
{
    uwb_mac_remove_interface(wcs->ccp->dev_inst, wcs->cbs.id);
    timescale_free(wcs->timescale);
}

/*!
 * @fn wcs_timescale_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
 *
 * @brief Timescale processing interruprt context
 *
 * input parameters
 * @param inst - struct os_event * ev *
 *
 * output parameters
 *
 * returns none
 */
static bool
wcs_timescale_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs)
{
    struct uwb_wcs_instance * wcs = (struct uwb_wcs_instance *)cbs->inst_ptr;
    struct uwb_ccp_instance * ccp = wcs->ccp;

    uwb_ccp_frame_t * frame = ccp->frames[(ccp->idx)%ccp->nframes];
    wcs->carrier_integrator = frame->carrier_integrator;
    wcs->observed_interval = (ccp->local_epoch - wcs->local_epoch.lo) & 0x0FFFFFFFFFFUL; // Observed ccp interval
    wcs->master_epoch.timestamp = ccp->master_epoch.timestamp;
    wcs->local_epoch.timestamp += wcs->observed_interval;

    if(ccp->status.valid){
        if (wcs->config.postprocess == true)
            dpl_eventq_put(dpl_eventq_dflt_get(), &wcs->postprocess_ev);
    }else{
        wcs->normalized_skew = (dpl_float64_t) 1.0l;
        wcs->fractional_skew = (dpl_float64_t) 0.0l;
        wcs->status.initialized = 0;
    }
    return true;

}


/*!
 * @fn wcs_timescale_ev(struct dpl_event * ev)
 *
 * @brief Timescale processing thread context
 *
 * input parameters
 * @param inst - struct os_event * ev *
 *
 * output parameters
 *
 * returns none
 */
void
wcs_timescale_ev(struct dpl_event * ev)
{
    assert(ev != NULL);
    assert(dpl_event_get_arg(ev) != NULL);

    struct uwb_wcs_instance * wcs = (struct uwb_wcs_instance *)dpl_event_get_arg(ev);
    struct uwb_ccp_instance * ccp = wcs->ccp;
    uwb_wcs_states_t * states = (uwb_wcs_states_t *) wcs->states.array;
    timescale_instance_t * timescale = wcs->timescale;

    if(ccp->status.valid){
        double q[] = { MYNEWT_VAL(TIMESCALE_QVAR) * 1.0l, MYNEWT_VAL(TIMESCALE_QVAR) * 0.1l, MYNEWT_VAL(TIMESCALE_QVAR) * 0.01l};

        if (wcs->status.initialized == 0){
            states->time = (double) wcs->master_epoch.lo;
            states->skew = (1.0l + (double ) uwb_calc_clock_offset_ratio(
                                ccp->dev_inst, wcs->carrier_integrator,
                                UWB_CR_CARRIER_INTEGRATOR)) * MYNEWT_VAL(UWB_WCS_DTU) ;
            states->drift = 0;
            double x0[] = {states->time, states->skew, states->drift};
            double T = 1e-6l * MYNEWT_VAL(UWB_CCP_PERIOD);
            timescale = timescale_init(timescale, x0, q, T);
            ((timescale_states_t * )timescale->eke->x)->time = states->time;
            ((timescale_states_t * )timescale->eke->x)->skew = states->skew;
            ((timescale_states_t * )timescale->eke->x)->drift =states->drift;
            wcs->status.valid = wcs->status.initialized = 1;
        }else{
            double z[] ={(double) wcs->master_epoch.lo,
                        (1.0l + (double ) uwb_calc_clock_offset_ratio(
                               ccp->dev_inst, wcs->carrier_integrator,
                               UWB_CR_CARRIER_INTEGRATOR)) * MYNEWT_VAL(UWB_WCS_DTU)
            };
            double T = wcs->observed_interval / MYNEWT_VAL(UWB_WCS_DTU) ; // observed interval in seconds, master reference
            double r[] = {MYNEWT_VAL(TIMESCALE_RVAR), MYNEWT_VAL(UWB_WCS_DTU) * 1e20};
            wcs->status.valid = timescale_main(timescale, z, q, r, T).valid;
        }

        if (wcs->status.valid){
            states->time = ((timescale_states_t * )timescale->eke->x)->time;
            states->skew = ((timescale_states_t * )timescale->eke->x)->skew;
            states->drift = ((timescale_states_t * )timescale->eke->x)->drift;
            wcs->normalized_skew = states->skew / MYNEWT_VAL(UWB_WCS_DTU);
            wcs->fractional_skew = (dpl_float64_t) 1.0l - wcs->normalized_skew;
        }else{
            wcs->normalized_skew = (dpl_float64_t) 1.0l;
            wcs->fractional_skew = (dpl_float64_t) 0.0l;
        }
#if MYNEWT_VAL(UWB_WCS_VERBOSE)
    wcs_json_t json = {
        .utime = ccp->master_epoch.timestamp,
        .wcs = {states->time, states->skew, states->drift},
        .ppm = DPL_FLOAT64_MUL(wcs->fractional_skew, DPL_FLOAT64_INIT(1e6l))
    };
    wcs_json_write_uint64(&json);
#endif
    }
}


/**
 * @fn uwb_timescale_pkg_init(void)
 * @brief API to initialise the package, only one ccp service required in the system.
 *
 * @return void
 */
void
wcs_timescale_pkg_init(void)
{
    int i;
    struct uwb_dev * udev;
    struct uwb_ccp_instance * ccp;

#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %lu,\"msg\": \"wcs_timescale_pkg_init\"}\n",dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_CCP);
        wcs_timescale_init(ccp->wcs);
    }
}

/**
 * @fn uwb_timwscale_pkg_down(void)
 * @brief Uninitialise ccp
 *
 * @return void
 */
int
wcs_timescale_pkg_down(int reason)
{
    int i;
    struct uwb_dev *udev;
    struct uwb_wcs_instance * wcs;

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        wcs = (struct uwb_wcs_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_WCS);
        if (!wcs) {
            continue;
        }
        wcs_timescale_free(wcs);
    }

    return 0;
}

#else

void wcs_timescale_pkg_init(void){return;};
int wcs_timescale_pkg_down(int reason){return 0;};

#endif  //TIMESCALE_ENABLED
