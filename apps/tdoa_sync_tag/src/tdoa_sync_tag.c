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
 * @file tdoa_sync_tag.c
 * @date 2020
 * @brief TDMA tag synchronised test
 *
 * @details Test for emitting tdoa packages with current timestamp
 *
 */

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <dpl/dpl_eventq.h>
#include <dpl/dpl_cputime.h>
#include <uwb/uwb.h>

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("Tdma tag (sync)");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("");

#define slog(fmt, ...)                                                \
    pr_info("uwb_tdma_tag: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static struct dpl_callout tdoa_callout;
static int16_t g_blink_rate = 1;
static struct kobject *kobj = 0;
struct mutex local_lock;

/*
 * Event callback function for timer events.
*/
struct tdoa_blink_frame {
    uint8_t fctrl;              //!< Frame type (0xC5 for a blink) using 64-bit addressing
    uint8_t seq_num;            //!< Sequence number, incremented for each new frame
    union {
        uint64_t long_address;  //!< Device ID TODOs::depreciated nomenclature
        uint64_t euid;          //!< extended unique identifier
    };
    uint8_t spacer;
    uint64_t ts;
} __attribute__((__packed__,aligned(1)));

static struct tdoa_blink_frame frame = {
    .fctrl = 0xC5,  /* frame type (0xC5 for a blink) using 64-bit addressing */
    .seq_num = 0,   /* sequence number, incremented for each new frame. */
    .long_address = 0,  /* device ID */
};

static
void tdoa_timer_ev_cb(struct dpl_event *ev)
{
    struct uwb_dev *udev;
    uint64_t frame_duration;

    if (!g_blink_rate) {
        return;
    }
    assert(ev != NULL);
    udev = uwb_dev_idx_lookup(0);
    if (uwb_tx_wait(udev, dpl_cputime_usecs_to_ticks(10000)) != DPL_OK) {
        slog("tx_wait timeout\n");
        goto early_ret;
    }

    frame_duration = uwb_phy_frame_duration(udev, sizeof(frame));
    frame.ts = (uwb_read_systime(udev) + ((frame_duration + MYNEWT_VAL(OS_LATENCY))<<16));
    frame.ts &= 0xfffffffffffffe00ULL;
    uwb_set_delay_start(udev, frame.ts);
    frame.ts += udev->tx_antenna_delay;
    frame.ts &= UWB_DTU_40BMASK;

    frame.euid = udev->euid;

    uwb_write_tx(udev, (u8*)&frame, 0, sizeof(frame));
    uwb_write_tx_fctrl(udev, sizeof(frame), 0);

    if (uwb_start_tx(udev).start_tx_error){
        uwb_phy_forcetrxoff(udev);
        slog("start tx err. Rate may be too high\n");
    } else {
        frame.seq_num++;
    }

early_ret:
    if (g_blink_rate) {
        dpl_callout_reset(&tdoa_callout, DPL_TICKS_PER_SEC/g_blink_rate);
    }
}


static ssize_t tdoa_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    char tmpbuf[64];

    if (!strcmp(attr->attr.name, "rate")) {
        snprintf(tmpbuf, sizeof(tmpbuf), "%d", g_blink_rate);
        memcpy(buf, tmpbuf, strlen(tmpbuf));
        return strlen(tmpbuf);
    }
    return 0;
}

static ssize_t tdoa_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc;
    long param;

    rc = kstrtol(buf, 0, &param);
    if (rc!=0) {
        slog("Error, rx-timeout not valid: %s\n", buf);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&local_lock)) {
        return -ERESTARTSYS;
    }

    if (!strcmp(attr->attr.name, "rate")) {
        g_blink_rate = param;
    }

    mutex_unlock(&local_lock);
    return count;
}

#define DEV_ATTR_ROW(__X, __Y)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = tdoa_show, .store = tdoa_store},                 \
            .var=__Y }

static struct dev_ext_attribute *dev_attr_cfg = 0;

static struct attribute** sysfs_attrs;

static struct attribute_group tdoa_attribute_group = {0};

void tdoa_sysfs_init(int suffix)
{
    int i;
    int rc;

    mutex_init(&local_lock);

    kobj = kobject_create_and_add("uwb_tdoa_tag", kernel_kobj);
    if (!kobj) {
        slog("Failed to create uwb_tdoa_tag kobj\n");
        return;
    }

    dev_attr_cfg = kzalloc((1)*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    sysfs_attrs = kzalloc((1+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!sysfs_attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    i=0;
    slog("Adding rate\n");
    dev_attr_cfg[i] = DEV_ATTR_ROW("rate", 0);
    sysfs_attrs[i] = &dev_attr_cfg[i].attr.attr;
    i++;

    tdoa_attribute_group.attrs = sysfs_attrs;
    rc = sysfs_create_group(kobj, &tdoa_attribute_group);
    if (rc) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }
    return;

err_sysfs:
    kfree(sysfs_attrs);
err_sysfs_attrs:
    kfree(dev_attr_cfg);
err_dev_attr_cfg:
    kobject_put(kobj);
    return;
}

static void sysfs_deinit(void)
{
    if (kobj) {
        mutex_destroy(&local_lock);
        sysfs_remove_group(kobj, &tdoa_attribute_group);
        kfree(sysfs_attrs);
        kfree(dev_attr_cfg);
        kobject_put(kobj);
    }
}


/**
 * @fn tdoa_sync_tag_pkg_init(void)
 * @brief API to initialise the listener package.
 *
 * @return void
 */

void
tdoa_sync_tag_pkg_init(void)
{
    struct uwb_dev *udev;

    pr_info("uwb_tp_test: Init\n");
    udev = uwb_dev_idx_lookup(0);
    if (!udev) {
        slog("ERROR, no uwb_dev found");
        return;
    }
    /*
     * Initialize the callout for a timer event.
     */
    dpl_callout_init(&tdoa_callout, dpl_eventq_dflt_get(), tdoa_timer_ev_cb, NULL);
    if (g_blink_rate) {
        dpl_callout_reset(&tdoa_callout, DPL_TICKS_PER_SEC/g_blink_rate);
    }

    tdoa_sysfs_init(0);
    pr_info("uwb_tdoa_sync_tag: To change rate: echo 60 > /sys/kernel/uwb_tdoa_tag/rate\n");
}

int
tdoa_sync_tag_pkg_down(int reason)
{
    g_blink_rate = 0;
    dpl_callout_stop(&tdoa_callout);
    return 0;
}


static int __init tdoa_sync_tag_entry(void)
{
    tdoa_sync_tag_pkg_init();
    return 0;
}

static void __exit tdoa_sync_tag_exit(void)
{
    sysfs_deinit();
    tdoa_sync_tag_pkg_down(0);
}

module_init(tdoa_sync_tag_entry);
module_exit(tdoa_sync_tag_exit);
