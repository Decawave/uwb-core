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
 * @file wire_listener.c
 * @date 2020
 * @brief Wireshark compatible listener / sniffer for uwb packages
 *
 * @details Listener / Sniffer for uwb packages, printing them out as wireshark compatible format
 *
 */

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <uwb_wire_listener/uwb_wire_listener.h>

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("UWB Wireshark Listener");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("");

void uwb_wire_listener_sysfs_init(struct uwb_wire_listener_instance *listener);
void uwb_wire_listener_sysfs_deinit(int idx);

#define slog(fmt, ...)                                                \
    pr_info("uwb_wlistener: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static bool rx_complete_cb(struct uwb_dev *inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev *inst, struct uwb_mac_interface * cbs);

static struct uwb_mac_interface g_cbs[] = {
    [0] = {
        .id = UWBEXT_LISTENER,
        .inst_ptr = 0,
        .rx_complete_cb = rx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
    },
    [1] = {
        .id = UWBEXT_LISTENER,
        .inst_ptr = 0,
        .rx_complete_cb = rx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
    },
    [2] = {
        .id = UWBEXT_LISTENER,
        .inst_ptr = 0,
        .rx_complete_cb = rx_complete_cb,
        .rx_timeout_cb = rx_timeout_cb,
    }
};

/**
 * @fn uwb_wire_listener_init(struct uwb_dev *inst)
 * @brief API to initialize wireshark listener
 *
 * @param inst          Pointer to struct uwb_dev
 *
 * @return struct uwb_wire_listener_instance
 */
struct uwb_wire_listener_instance *
uwb_wire_listener_init(struct uwb_dev *dev)
{
    struct uwb_wire_listener_instance *listener = (struct uwb_wire_listener_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_LISTENER);
    if (listener == NULL) {
        listener = (struct uwb_wire_listener_instance *) kzalloc(1, sizeof(*listener));
        if (!listener) {
            return 0;
        }
        listener->status.selfmalloc = 1;
    }

    listener->dev_inst = dev;
    listener->status.initialized = 1;

    return listener;
}

/**
 * @fn uwb_wire_listener_free(struct uwb_wire_listener_instance *inst)
 * @brief API to free the allocated resources
 *
 * @param inst Pointer to struct uwb_listener instance
 *
 * @return void
 */
void
uwb_wire_listener_free(struct uwb_wire_listener_instance *listener)
{
    if (listener->status.selfmalloc) {
        kfree(listener);
    } else {
        listener->status.initialized = 0;
    }
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
    uwb_set_rx_timeout(inst, 0);
    uwb_start_rx(inst);
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
    struct timeval tv;
    struct uwb_wire_listener_instance *listener = (struct uwb_wire_listener_instance *)cbs->inst_ptr;

    if (inst != listener->dev_inst)
        return false;

    do_gettimeofday(&tv);
    listener->pcap_header.ts_sec = tv.tv_sec;
    listener->pcap_header.ts_usec = tv.tv_usec;
    listener->pcap_header.incl_len = inst->frame_len;
    listener->pcap_header.orig_len = inst->frame_len;

    memcpy(listener->pcap_header.buf_arr, inst->rxbuf, inst->frame_len);

    uwb_wire_listener_chrdev_output(inst->idx, (uint8_t *)&listener->pcap_header,
                                    sizeof(listener->pcap_header) -
                                    sizeof(listener->pcap_header.buf_arr) +
                                    inst->frame_len);

    return false;
}

/**
 * @fn uwb_wire_listener_pkg_init(void)
 * @brief API to initialise the listener package.
 *
 * @return void
 */
void
uwb_wire_listener_pkg_init(void)
{
    int i;
    struct uwb_wire_listener_instance *listener;
    struct uwb_dev *udev;

    pr_info("uwb_listener: Init\n");

    for (i = 0; i < ARRAY_SIZE(g_cbs); i++) {
        udev = uwb_dev_idx_lookup(i);

        if (!udev)
            continue;

        g_cbs[i].inst_ptr = listener = uwb_wire_listener_init(udev);
        uwb_mac_append_interface(udev, &g_cbs[i]);
        uwb_wire_listener_sysfs_init(g_cbs[i].inst_ptr);
        uwb_wire_listener_chrdev_create(udev->idx);
    }

    pr_info("uwb_listener: To start listening: \
                echo 0 >/sys/kernel/uwb_listener/listen;cat /dev/uwb_lstnr0\n");
}

int
uwb_wire_listener_pkg_down(int reason)
{
    int i;

    slog("");
    for(i = 0; i < ARRAY_SIZE(g_cbs); i++) {
        if (g_cbs[i].inst_ptr) {
            struct uwb_wire_listener_instance *listener = (struct uwb_wire_listener_instance *)g_cbs[i].inst_ptr;
            uwb_mac_remove_interface(listener->dev_inst, g_cbs[i].id);

            uwb_wire_listener_chrdev_destroy(listener->dev_inst->idx);
            uwb_wire_listener_sysfs_deinit(listener->dev_inst->idx);

            uwb_wire_listener_free(g_cbs[i].inst_ptr);
            g_cbs[i].inst_ptr = 0;
        }
    }
    return 0;
}

static int __init uwb_wire_listener_entry(void)
{
    uwb_wire_listener_pkg_init();
    return 0;
}

static void __exit uwb_wire_listener_exit(void)
{
    uwb_wire_listener_pkg_down(0);
}

module_init(uwb_wire_listener_entry);
module_exit(uwb_wire_listener_exit);
