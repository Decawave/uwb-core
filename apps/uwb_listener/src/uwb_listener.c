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
 * @file uwb_listener.c
 * @date 2020
 * @brief Listener / Sniffer for uwb packages
 *
 * @details Listener / Sniffer for uwb packages, printing them to standard out / a character device.
 *
 */

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <uwb_listener/uwb_listener.h>
#include "listener_json.h"

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("UWB Listener");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("");

void uwb_listener_sysfs_init(struct uwb_listener_instance *listener);
void uwb_listener_sysfs_deinit(int idx);

#define slog(fmt, ...)                                                \
    pr_info("uwb_listener: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static struct listener_json _json;

static bool rx_complete_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);
static bool rx_timeout_cb(struct uwb_dev * inst, struct uwb_mac_interface * cbs);

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
 * @fn uwb_listener_init(struct uwb_dev * inst)
 * @brief API to initialise the listener
 *
 * @param inst      Pointer to struct uwb_dev.
 *
 * @return struct uwb_listener_instance
 */
struct uwb_listener_instance *
uwb_listener_init(struct uwb_dev * dev)
{
    struct uwb_listener_instance *listener = (struct uwb_listener_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_LISTENER);
    if (listener == NULL ) {
        listener = (struct uwb_listener_instance *) kzalloc(1, sizeof(*listener));
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
 * @fn uwb_listener_free(struct uwb_listener_instance * inst)
 * @brief API to free the allocated resources.
 *
 * @param inst  Pointer to struct uwb_listener_instance.
 *
 * @return void
 */
void
uwb_listener_free(struct uwb_listener_instance * listener)
{
    if (listener->status.selfmalloc)
        kfree(listener);
    else
        listener->status.initialized = 0;
}

int
listener_write_line(void *buf, char* data, int len)
{
    uint16_t i;
    struct listener_json * json = buf;
    for (i=0; i < len; i++) {
        json->iobuf[json->idx++] = data[i];
        if (data[i]=='\0'){
            break;
        }
    }
    if (json->iobuf[json->idx-1]=='\0') {
        json->idx = 0;
    }

    return len;
}

int
listener_json_write(struct listener_json * json)
{
    int rc;
    struct json_value value;

    json->encoder.je_write = listener_write_line;
    json->encoder.je_arg = (void *)json;
    json->decoder.json_buf.jb_read_next = json_read_next;
    json->decoder.json_buf.jb_read_prev = json_read_prev;
    json->decoder.json_buf.jb_readn = json_readn;

    json->encoder.je_wr_commas = 0;
    rc = json_encode_object_start(&json->encoder);
    JSON_VALUE_UINT(&value, json->utime);
    rc |= json_encode_object_entry(&json->encoder, "utime", &value);

    if (json->wcs_utime) {
        JSON_VALUE_UINT(&value, json->wcs_utime);
        rc |= json_encode_object_entry(&json->encoder, "wcs_utime", &value);
    }

    if (!DPL_FLOAT64_ISNAN(json->rssi)) {
        JSON_VALUE_FLOAT64(&value, json->rssi);
        rc |= json_encode_object_entry(&json->encoder, "rssi", &value);
    }

    if (json->data_len) {
        JSON_VALUE_UINT(&value, json->data_len);
        rc |= json_encode_object_entry(&json->encoder, "dlen", &value);

        JSON_VALUE_STRING(&value, json->data);
        rc |= json_encode_object_entry(&json->encoder, "d", &value);
    }

    rc |= json_encode_object_finish(&json->encoder);
    json->encoder.je_wr_commas = 0;
    assert(rc == 0);

    listener_write_line(json->encoder.je_arg, "\0", 1);
    return rc;
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
    /* Restart receiver */
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
    int i;
    char *wp;
    uint8_t *rp;
    size_t n;
    unsigned long flags;
    struct uwb_listener_instance * listener = (struct uwb_listener_instance *)cbs->inst_ptr;
    if (inst != listener->dev_inst) return false;

    spin_lock_irqsave(&_json.lock, flags);

    _json.utime = inst->rxtimestamp;
    _json.wcs_utime = 0;
    _json.rssi = DPL_FLOAT64_FROM_F32(uwb_get_rssi(inst));
    _json.data_len = inst->frame_len;

    if (2*_json.data_len > sizeof(_json.data)) {
        _json.data_len = sizeof(_json.data)/2;
    }

    rp = inst->rxbuf;
    wp = _json.data;
    for (i=0;i<_json.data_len;i++) {
        sprintf(wp, "%02X", *(rp++));
        wp += 2;
    }

    listener_json_write(&_json);

    n = strlen(_json.iobuf);
    _json.iobuf[n]='\n';
    _json.iobuf[n+1]='\0';
    listener_chrdev_output(inst->idx, _json.iobuf, n+1);
    spin_unlock_irqrestore(&_json.lock, flags);

    /* Never "grab" a package */
    return false;
}


/**
 * @fn uwb_listener_pkg_init(void)
 * @brief API to initialise the listener package.
 *
 * @return void
 */
void
uwb_listener_pkg_init(void)
{
    int i;
    struct uwb_listener_instance *listener;
    struct uwb_dev *udev;

    pr_info("uwb_listener: Init\n");
    spin_lock_init(&_json.lock);

    for(i=0;i<ARRAY_SIZE(g_cbs);i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) continue;
        g_cbs[i].inst_ptr = listener = uwb_listener_init(udev);
        uwb_mac_append_interface(udev, &g_cbs[i]);
        uwb_listener_sysfs_init(g_cbs[i].inst_ptr);
        listener_chrdev_create(udev->idx);
        pr_info("uwb_listener: To start listening: echo 0 >/sys/kernel/uwb_listener%d/listen;cat /dev/uwb_lstnr%d\n",
                i, i);
    }
}

int
uwb_listener_pkg_down(int reason)
{
    int i;
    slog("");
    for(i=0;i<ARRAY_SIZE(g_cbs);i++) {
        if (g_cbs[i].inst_ptr) {
            struct uwb_listener_instance *listener = (struct uwb_listener_instance *)g_cbs[i].inst_ptr;
            uwb_mac_remove_interface(listener->dev_inst, g_cbs[i].id);

            listener_chrdev_destroy(listener->dev_inst->idx);
            uwb_listener_sysfs_deinit(listener->dev_inst->idx);

            uwb_listener_free(g_cbs[i].inst_ptr);
            g_cbs[i].inst_ptr = 0;
        }
    }

    return 0;
}


static int __init uwb_listener_entry(void)
{
    uwb_listener_pkg_init();
    return 0;
}

static void __exit uwb_listener_exit(void)
{
    uwb_listener_pkg_down(0);
}

module_init(uwb_listener_entry);
module_exit(uwb_listener_exit);
