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
 * @file uwb_wcs.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date Oct 20 2018
 * @brief Wireless Clock Synchronization
 *
 * @details This is a top-level package for managing wireless calibration using Clock Calibration Packet (CCP).
 * In an RTLS system the Clock Master send a periodic blink frame which is received by the anchor nodes. The device driver model on the node
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

#include <uwb/uwb.h>
#include <uwb_wcs/uwb_wcs.h>

#if MYNEWT_VAL(UWB_WCS_ENABLED)

#ifdef __KERNEL__
int wcs_chrdev_create(struct uwb_wcs_instance * wcs);
void wcs_chrdev_destroy(int idx);
#endif  /* __KERNEL__ */

#ifndef DIAGMSG
#define DIAGMSG(s,u)
#endif


/*!
 * @fn uwb_wcs_init(struct uwb_wcs_instance * inst,  struct uwb_ccp_instance * ccp)
 *
 * @brief Allocate resources for the wireless clock synchronization. Binds resources
 * to uwb_ccp interface and instantiate timescale instance if in use.
 *
 * input parameters
 * @param wcs -struct uwb_wcs_instance *
 * @param ccp - struct uwb_ccp_instance *
 *
 * output parameters
 *
 * returns struct uwb_wcs_instance *
 */

struct uwb_wcs_instance *
uwb_wcs_init(struct uwb_wcs_instance * wcs, struct uwb_ccp_instance * ccp)
{
    if (wcs == NULL ) {
        wcs = (struct uwb_wcs_instance *) calloc(1, sizeof(struct uwb_wcs_instance));
        assert(wcs);
        wcs->status.selfmalloc = 1;
    }
    wcs->ccp = ccp;
    wcs->normalized_skew = DPL_FLOAT64_INIT(1.0l);
    wcs->fractional_skew = DPL_FLOAT64_INIT(0.0l);

    return wcs;
}

/*!
 * @fn uwb_wcs_free(struct uwb_wcs_instance * inst)
 *
 * @brief Free resources and restore default behaviour.
 *
 * input parameters
 * @param inst - struct uwb_wcs_instance * inst
 *
 * output parameters
 *
 * returns none
 */
void
uwb_wcs_free(struct uwb_wcs_instance * inst)
{
    assert(inst);
#ifndef __KERNEL__
#if MYNEWT_VAL(TIMESCALE_ENABLED)
    timescale_free(inst->timescale);
#endif
#endif
    if (inst->status.selfmalloc)
        free(inst);
    else
        inst->status.initialized = 0;
}


/*!
 * @fn uwb_wcs_set_postprocess(struct uwb_wcs_instance *  inst * inst, os_event_fn * ccp_postprocess)
 *
 * @brief Overrides the default post-processing behaviors, replacing the JSON stream with an alternative
 * or an advanced timescale processing algorithm.
 *
 * input parameters
 * @param inst - struct uwb_wcs_instance *
 *
 * returns none
 */
void
uwb_wcs_set_postprocess(struct uwb_wcs_instance * wcs, dpl_event_fn * postprocess)
{
    if(!wcs) return;
    dpl_event_init(&wcs->postprocess_ev, postprocess, (void *)wcs);
    wcs->config.postprocess = true;
}

/**
 * Compensate for local clock to reference clock by applying skew correction
 *
 * @param inst  Pointer to struct uwb_wcs_instance * wcs.
 * @param dtu_time uint64_t time in decawave transeiver units of time (dtu)
 *
 * @return dtu_time compensated.
 *
 */
uint64_t
uwb_wcs_dtu_time_adjust(struct uwb_wcs_instance * wcs, uint64_t dtu_time)
{
    if(!wcs) return dtu_time;
    if (wcs->status.valid){
        dtu_time = (uint64_t) DPL_FLOAT64_INT(DPL_FLOAT64_MUL(wcs->normalized_skew,DPL_FLOAT64_I64_TO_F64(dtu_time)));
    }

    return dtu_time & 0x00FFFFFFFFFFUL;
}

/**
 * Compensate for clock skew and offset relative to master clock reference frame
 *
 * @param wcs  struct uwb_wcs_instance *
 * @param dtu_time local observed timestamp
 * @return Adjusted dtu_time projected to clock master reference, 64bit represenatation
 *
 */
uint64_t
uwb_wcs_local_to_master64(struct uwb_wcs_instance * wcs, uint64_t dtu_time)
{
    uint64_t delta;
    uint64_t master_lo40;
    dpl_float64_t interval;
    if(!wcs) return 0xffffffffffffffffULL;
    delta = ((dtu_time & 0x0FFFFFFFFFFUL) - wcs->local_epoch.lo) & 0x0FFFFFFFFFFUL;

    if (wcs->status.valid) {
        /* No need to take special care of 40bit overflow as the timescale forward returns
         * a double value that can exceed the 40bit. */
        interval = DPL_FLOAT64_DIV(DPL_FLOAT64_U64_TO_F64(delta), DPL_FLOAT64_INIT(MYNEWT_VAL(UWB_WCS_DTU)));
        master_lo40 = (uint64_t) DPL_FLOAT64_INT(uwb_wcs_prediction(wcs->states.array, interval));
    }else{
        master_lo40 = wcs->master_epoch.lo + delta;
    }
    return (wcs->master_epoch.timestamp & 0xFFFFFF0000000000UL) + master_lo40;
}

/**
 * Compensate for clock skew and offset relative to master clock reference frame
 *
 * @param wcs pointer to struct uwb_wcs_instance
 * @param dtu_time local observed timestamp
 * @return Adjustde dtu_time projected to clock master reference, 40bits representation
 *
 */
uint64_t
uwb_wcs_local_to_master(struct uwb_wcs_instance * wcs, uint64_t dtu_time)
{
    assert(wcs);
    return uwb_wcs_local_to_master64(wcs, dtu_time) & 0x0FFFFFFFFFFUL;
}

/**
 * Read compensated system time (DTU)
 *
 * With UWB_WCS_ENABLED the adjust_timer API compensates all local timestamps values for local skew.
 * This simplifies the TWR problem by mitigiating the need for double sided exchanges.
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 * @return time
 */
uint64_t
uwb_wcs_read_systime(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_dtu_time_adjust(wcs, uwb_read_systime(inst));
}

/**
 * Read lower 32 bits of compensated system time
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint32_t
uwb_wcs_read_systime_lo(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return (uint32_t) (uwb_wcs_dtu_time_adjust(wcs, uwb_read_systime_lo32(inst)) & 0xFFFFFFFFUL);
}

/**
 * Read compensated rx time timetamp (DTU).
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint64_t
uwb_wcs_read_rxtime(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_dtu_time_adjust(wcs, uwb_read_rxtime(inst));
}

/**
 * Read lower 32bits of compensated rx time timetamp (DTU).
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint32_t
uwb_wcs_read_rxtime_lo(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return (uint32_t) uwb_wcs_dtu_time_adjust(wcs, uwb_read_rxtime_lo32(inst)) & 0xFFFFFFFFUL;
}

/**
 * Read compensated transmission time.
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 *
 */
uint64_t
uwb_wcs_read_txtime(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_dtu_time_adjust(wcs, uwb_read_txtime(inst));
}

/**
 * Read lower 32bits of compensated transmission time.
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint32_t
uwb_wcs_read_txtime_lo(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return (uint32_t) (uwb_wcs_dtu_time_adjust(wcs, uwb_read_txtime_lo32(inst)) & 0xFFFFFFFFUL);
}

/**
 * Read systime transformed into the clock master's local frame
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 * @return time
 */
uint64_t
uwb_wcs_read_systime_master(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_local_to_master(wcs, uwb_read_systime(inst));
}

/**
 * Read systime transformed into clock master's frame, full 64bit representation
 *
 * @param inst  Pointer to struct uwb_dev.
 * @return time
 */
uint64_t
uwb_wcs_read_systime_master64(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_local_to_master64(wcs, uwb_read_systime(inst));
}

/**
 * Read lower 32 bits of systime transformed into clock master's frame
 * API to read systime adjusted for local skew, full 32bit representation
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint32_t
uwb_wcs_read_systime_lo_master(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return (uint32_t) (uwb_wcs_local_to_master(wcs, uwb_read_systime_lo32(inst)) & 0xFFFFFFFFUL);
}

/**
 * Read rxtime transformed into clock master's frame
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint64_t
uwb_wcs_read_rxtime_master(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_local_to_master(wcs, uwb_read_rxtime(inst));
}

/**
 * Read lower 32 bits of rxtime transformed into clock master's frame
 *
 * @param inst  Pointer to struct uwb_dev * inst.
 *
 * @return time
 */
uint32_t
uwb_wcs_read_rxtime_lo_master(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return (uint32_t) (uwb_wcs_local_to_master(wcs, uwb_read_rxtime_lo32(inst)) & 0xFFFFFFFFUL);
}

/**
 * Read txtime transformed into clock master's frame
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return time
 *
 */
uint64_t
uwb_wcs_read_txtime_master(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return uwb_wcs_local_to_master(wcs, uwb_read_txtime(inst));
}

/**
 * Read lower 32 bits of txtime transformed into clock master's frame
 *
 * @param inst  Pointer to struct uwb_dev.
 *
 * @return time
 */
uint32_t
uwb_wcs_read_txtime_lo_master(struct uwb_dev * inst)
{
    struct uwb_ccp_instance *ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(inst, UWBEXT_CCP);
    struct uwb_wcs_instance *wcs = ccp->wcs;
    return (uint32_t) (uwb_wcs_local_to_master(wcs, uwb_read_txtime_lo32(inst))& 0xFFFFFFFFUL);
}

/**
 * @brief API for prediction in master clock reference frame, adjusting for local offset, skew and drift.
 * @param x * dpl_float64[] of estimator states
 * @param T interval over which to predict.
 * @return Clock master prediction for local epoch T
 */
dpl_float64_t
uwb_wcs_prediction(dpl_float64_t * x, dpl_float64_t T)
{
    // x = A * x;
    dpl_float64_t A[] = { DPL_FLOAT64_INIT(1.0l), T, DPL_FLOAT64_DIV(DPL_FLOAT64_MUL(T,T),DPL_FLOAT64_INIT(2.0l))};
    dpl_float64_t tmp = DPL_FLOAT64_INIT(0.0l);
    for (uint8_t i=0 ; i < sizeof(A)/sizeof(dpl_float64_t); i++ ){
            tmp = DPL_FLOAT64_ADD(tmp,DPL_FLOAT64_MUL(A[i],x[i]));
    }
    return tmp;
}

#endif  /* MYNEWT_VAL(UWB_WCS_ENABLED) */

/**
 * @fn uwb_wcs_pkg_init(void)
 * @brief API to initialise the package, only one wcs service required in the system.
 *
 * @return void
 */
void
uwb_wcs_pkg_init(void)
{
    int i;
    struct uwb_dev * udev;
    struct uwb_ccp_instance * ccp;
#if MYNEWT_VAL(UWB_PKG_INIT_LOG)
    printf("{\"utime\": %"PRIu32",\"msg\": \"uwb_wcs_pkg_init\"}\n",
           dpl_cputime_ticks_to_usecs(dpl_cputime_get32()));
#endif

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
#if MYNEWT_VAL(UWB_CCP_ENABLED)
        ccp = (struct uwb_ccp_instance *)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_CCP);
        if (!ccp) {
            continue;
        }
        ccp->wcs = uwb_wcs_init(NULL, ccp);
#ifdef __KERNEL__
        wcs_chrdev_create(ccp->wcs);
#endif /* __KERNEL__ */
#endif
    }

}


/**
 * @fn uwb_wcs_pkg_down(void)
 * @brief Uninitialise WCS
 *
 * @return void
 */
int
uwb_wcs_pkg_down(int reason)
{
    int i;
    struct uwb_dev * udev;
    struct uwb_ccp_instance * ccp;

    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
#if MYNEWT_VAL(UWB_CCP_ENABLED)
#ifdef __KERNEL__
        wcs_chrdev_destroy(i);
#endif /* __KERNEL__ */
        ccp = (struct uwb_ccp_instance *)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_CCP);
        if (!ccp) {
            continue;
        }
        if (ccp->wcs) {
            uwb_wcs_free(ccp->wcs);
            ccp->wcs = 0;
        }
#endif
    }

    return 0;
}
