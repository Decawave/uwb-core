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
 * @file uwb_tp_test.c
 * @date 2020
 * @brief Test module for uwb_transport
 *
 * @details Test for streaming data over uwb_transport
 *
 */

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <dpl/dpl_eventq.h>
#include <uwb/uwb.h>
#include <uwb_ccp/uwb_ccp.h>
#include <tdma/tdma.h>
#include <uwb_transport/uwb_transport.h>

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("UWB Transport Test");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("");

int uwb_tp_chrdev_create(int idx, struct _uwb_transport_instance *uwb_transport);
void uwb_tp_chrdev_destroy(int idx);

#define slog(fmt, ...)                                                \
    pr_info("uwb_tp_test: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

struct aloha_tp {
    int idx;
    struct dpl_callout callout;
    struct uwb_ccp_instance *ccp;
    struct _uwb_transport_instance *tp;
};

static int _ok_to_run = 1;
static struct aloha_tp aloha_inst[MYNEWT_VAL(UWB_DEVICE_MAX)];

/*!
 * @fn slot_cb(struct dpl_event * ev)
 *
 * input parameters
 * @param inst - struct dpl_event *
 *
 * output parameters
 *
 * returns none
 */
static void
stream_slot_cb(struct dpl_event * ev)
{
    uint64_t dxtime, dxtime_end;
    uint64_t preamble_duration;
    assert(ev);
    tdma_slot_t * slot = (tdma_slot_t *) dpl_event_get_arg(ev);
    tdma_instance_t * tdma = slot->parent;

    int32_t idx = slot->idx;
    uwb_transport_instance_t * uwb_transport = (uwb_transport_instance_t *)slot->arg;
    if (!_ok_to_run) {
        return;
    }

    preamble_duration = (uint64_t) uwb_usecs_to_dwt_usecs(uwb_phy_SHR_duration(tdma->dev_inst));
    /* If there's something in our tx queue try TX */
    dxtime = tdma_tx_slot_start(tdma, DPL_FLOAT32_I32_TO_F32(idx));
    dxtime_end = (tdma_tx_slot_start(tdma, DPL_FLOAT32_I32_TO_F32(idx+1)) - (preamble_duration << 16));
    dxtime_end &= UWB_DTU_40BMASK;
    if (uwb_transport_dequeue_tx(uwb_transport, dxtime, dxtime_end) == false) {
        /* Fallback to RX */
        dxtime = tdma_rx_slot_start(tdma, DPL_FLOAT32_I32_TO_F32(idx));
        dxtime_end = (tdma_rx_slot_start(tdma, DPL_FLOAT32_I32_TO_F32(idx+1)) - (preamble_duration << 16));
        dxtime_end &= UWB_DTU_40BMASK;
        uwb_transport_listen(uwb_transport, UWB_BLOCKING, dxtime, dxtime_end);
    }
}

static void
aloha_cb(struct dpl_event * ev)
{
    uint64_t dxtime, dxtime_end;
    struct aloha_tp * aloha = (struct aloha_tp *)dpl_event_get_arg(ev);
    struct uwb_ccp_instance *ccp = aloha->ccp;
    struct _uwb_transport_instance * uwb_transport = aloha->tp;

    if (!_ok_to_run) {
        return;
    }

    dpl_callout_reset(&aloha->callout, DPL_TICKS_PER_SEC/16);
    /* Don't keep going if ccp is active and valid */
    if (ccp) {
        if (ccp->status.valid) {
            return;
        }
    }

    /* If there's something in our tx queue try TX */
    dxtime = 0;
    dxtime_end = ((1000000ULL/16) << 16);
    dxtime_end &= UWB_DTU_40BMASK;
    if (uwb_transport_dequeue_tx(uwb_transport, dxtime, dxtime_end) == false) {
        /* Fallback to RX */
        uwb_transport_listen(uwb_transport, UWB_BLOCKING, dxtime, dxtime_end);
    }
}


/**
 * @fn uwb_listener_pkg_init(void)
 * @brief API to initialise the uwb transport test module.
 *
 * @return void
 */

void
uwb_tp_test_pkg_init(void)
{
    int i, idx;
    struct uwb_dev *udev;
    struct uwb_ccp_instance * ccp;
    tdma_instance_t * tdma;
    struct _uwb_transport_instance * uwb_transport;

    pr_info("uwb_tp_test: Init\n");
    for (idx = 0;idx < MYNEWT_VAL(UWB_DEVICE_MAX); idx++) {
        udev = uwb_dev_idx_lookup(idx);
        if (!udev) {
            continue;
        }
#if 0
        /* Enable double buffring if not already active */
        if (!udev->config.dblbuffon_enabled || udev->config.rxauto_enable) {
            udev->config.dblbuffon_enabled = 1;
            udev->config.rxauto_enable = 0;
            uwb_set_dblrxbuff(udev, udev->config.dblbuffon_enabled);
            uwb_mac_config(udev, NULL);
        }
#endif
        ccp = (struct uwb_ccp_instance*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_CCP);
        if (!ccp) {
            slog("ERROR, uwb_ccp not found");
            return;
        }

        tdma = (tdma_instance_t*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_TDMA);
        if (!tdma) {
            slog("ERROR, tdma not found");
            return;
        }

        uwb_transport = (struct _uwb_transport_instance *)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_TRANSPORT);
        if (!uwb_transport) {
            slog("ERROR, uwb_transport not found");
            continue;
        }

        /* Request ACKs */
        uwb_transport->config.request_acks = 1;

        /* Slot 0:ccp, 1-160 stream */
        for (i = 1; i < MYNEWT_VAL(TDMA_NSLOTS) - 1; i++) {
            tdma_assign_slot(tdma, stream_slot_cb,  i, uwb_transport);
        }

        uwb_tp_chrdev_create(idx, uwb_transport);

        aloha_inst[idx] = (struct aloha_tp){
            .idx = idx,
            .ccp = ccp,
            .tp = uwb_transport
        };
        dpl_callout_init(&aloha_inst[idx].callout, dpl_eventq_dflt_get(), aloha_cb, &aloha_inst[idx]);
        dpl_callout_reset(&aloha_inst[idx].callout, DPL_TICKS_PER_SEC/16);

        pr_info("uwb_tp_test: How to use:");
        pr_info("RX: cat /dev/uwbtp%d", idx);
        pr_info("TX: echo <dest addr> /sys/kernel/uwbtp%d/dest_uid; cat test.txt > /dev/uwbtp%d", idx, idx);
    }

}

int
uwb_tp_test_pkg_down(int reason)
{
    int i, idx;
    struct uwb_dev *udev;
    tdma_instance_t * tdma;

    for (idx = 0;idx < MYNEWT_VAL(UWB_DEVICE_MAX); idx++) {
        udev = uwb_dev_idx_lookup(idx);
        if (!udev) {
            continue;
        }
        tdma = (tdma_instance_t*)uwb_mac_find_cb_inst_ptr(udev, UWBEXT_TDMA);
        if (!tdma) {
            slog("ERROR, tdma not found");
            return -1;
        }

        for (i = 1; i < MYNEWT_VAL(TDMA_NSLOTS) - 1; i++) {
            tdma_release_slot(tdma, i);
        }

        uwb_tp_chrdev_destroy(idx);
    }
    return 0;
}


static int __init uwb_listener_entry(void)
{
    uwb_tp_test_pkg_init();
    return 0;
}

static void __exit uwb_listener_exit(void)
{
    _ok_to_run = 0;
    uwb_tp_test_pkg_down(0);
}

module_init(uwb_listener_entry);
module_exit(uwb_listener_exit);
