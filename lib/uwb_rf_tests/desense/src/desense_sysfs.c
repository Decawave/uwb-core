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

#include <syscfg/syscfg.h>

#ifdef __KERNEL__
#include <dpl/dpl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include "uwbcore.h"
#include <desense/desense.h>

#define slog(fmt, ...) \
    pr_info("uwb_desense: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

struct desense_sysfs_data {
    struct kobject *kobj;
    struct kobject *kobj_params;
    char device_name[16];
    struct uwb_desense_instance *desense;

    struct {
        struct dev_ext_attribute *dev_attr_cfg;
        struct attribute** attrs;
        struct attribute_group attribute_group;
    } cmd;
    struct {
        struct dev_ext_attribute *dev_attr_cfg;
        struct attribute** attrs;
        struct attribute_group attribute_group;
    } param;
    struct desense_test_parameters test_params;
    uint16_t txon_packet_length;
    int ret;
};

static struct desense_sysfs_data sysfs_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};

static const char* cmd_str[] = {
    "rx",
    "req",
    "txon",
    "txoff",
    /* Return val from cmds */
    "ret",
};

static const char* param_str[] = {
    "ms_start_delay",
    "ms_test_delay",
    "strong_coarse_power",
    "strong_fine_power",
    "n_strong",
    "test_coarse_power",
    "test_fine_power",
    "n_test",
    "txon_packet_length",
};

static ssize_t cmd_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    unsigned int copied = 0;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct desense_sysfs_data *ed = (struct desense_sysfs_data *)ext_dev->var;

    if (!strcmp(attr->attr.name, "rx")) {
        copied = snprintf(buf, PAGE_SIZE, "Listening on 0x%04X\n", ed->desense->dev_inst->my_short_address);
    }

    if (!strcmp(attr->attr.name, "ret")) {
        copied = snprintf(buf, PAGE_SIZE, "%d\n", ed->ret);
    }

    if (!strcmp(attr->attr.name, "ms_start_delay")) {
        copied = snprintf(buf, PAGE_SIZE, "ms_start_delay: %d\n", ed->test_params.ms_start_delay);
    }
    if (!strcmp(attr->attr.name, "ms_test_delay")) {
        copied = snprintf(buf, PAGE_SIZE, "ms_test_delay: %d\n", ed->test_params.ms_test_delay);
    }
    if (!strcmp(attr->attr.name, "strong_coarse_power")) {
        copied = snprintf(buf, PAGE_SIZE, "strong_coarse_power: %d\n", ed->test_params.strong_coarse_power);
    }
    if (!strcmp(attr->attr.name, "strong_fine_power")) {
        copied = snprintf(buf, PAGE_SIZE, "strong_fine_power: %d\n", ed->test_params.strong_fine_power);
    }
    if (!strcmp(attr->attr.name, "n_strong")) {
        copied = snprintf(buf, PAGE_SIZE, "n_strong: %d\n", ed->test_params.n_strong);
    }
    if (!strcmp(attr->attr.name, "test_coarse_power")) {
        copied = snprintf(buf, PAGE_SIZE, "test_coarse_power: %d\n", ed->test_params.test_coarse_power);
    }
    if (!strcmp(attr->attr.name, "test_fine_power")) {
        copied = snprintf(buf, PAGE_SIZE, "test_fine_power: %d\n", ed->test_params.test_fine_power);
    }
    if (!strcmp(attr->attr.name, "n_test")) {
        copied = snprintf(buf, PAGE_SIZE, "n_test: %d\n", ed->test_params.n_test);
    }
    if (!strcmp(attr->attr.name, "txon_packet_length")) {
        copied = snprintf(buf, PAGE_SIZE, "txon_packet_length: %d\n", ed->txon_packet_length);
    }

    return copied;
}

static ssize_t cmd_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret;
    u64 res;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct desense_sysfs_data *ed = (struct desense_sysfs_data *)ext_dev->var;

    ed->ret = -EINVAL;

    ret = kstrtoll(buf, 0, &res);
    if (ret) {
        slog("Error reading number from: '%s'", buf);
        return ret;
    }

    if (!strcmp(attr->attr.name, "rx")) {
        desense_listen(ed->desense);
        slog("Listening on 0x%04X\n", ed->desense->dev_inst->my_short_address);
        ed->ret = 0;
    }

    if (!strcmp(attr->attr.name, "req")) {
        desense_send_request(ed->desense, (u16)res, &ed->test_params);
        slog("request sent to 0x%04X\n", (u16)res);
        ed->ret = 0;
    }

    if (!strcmp(attr->attr.name, "txon")) {
        desense_txon(ed->desense, ed->txon_packet_length, res);
        slog("txon with data length: %d and start-to-start delay of %"PRIu64"ns\n",
             ed->txon_packet_length, res);
        ed->ret = 0;
    }

    if (!strcmp(attr->attr.name, "txoff")) {
        desense_txoff(ed->desense);
        slog("txoff issues");
        ed->ret = 0;
    }

    if (!strcmp(attr->attr.name, "ms_start_delay")) {
        ed->test_params.ms_start_delay = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "ms_test_delay")) {
        ed->test_params.ms_test_delay = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "strong_coarse_power")) {
        ed->test_params.strong_coarse_power = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "strong_fine_power")) {
        ed->test_params.strong_fine_power = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "n_strong")) {
        ed->test_params.n_strong = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "test_coarse_power")) {
        ed->test_params.test_coarse_power = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "test_fine_power")) {
        ed->test_params.test_fine_power = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "n_test")) {
        ed->test_params.n_test = res;
        ed->ret = 0;
    }
    if (!strcmp(attr->attr.name, "txon_packet_length")) {
        ed->txon_packet_length = res;
        ed->ret = 0;
    }

    return count;
}

#define DEV_ATTR_ROW(__X, __Y)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = cmd_show, .store = cmd_store},                 \
            .var=__Y }


void desense_sysfs_init(struct uwb_desense_instance *desense)
{
    int i;
    int ret;
    struct desense_sysfs_data *ed;
    if (desense->dev_inst->idx >= ARRAY_SIZE(sysfs_inst)) {
        return;
    }
    ed = &sysfs_inst[desense->dev_inst->idx];
    ed->desense = desense;

    /* Set some default values */
    ed->test_params = (struct desense_test_parameters){
        .ms_start_delay = 1000,
        .ms_test_delay = 10,
        .strong_coarse_power = 9,
        .strong_fine_power = 31,
        .n_strong = 5,
        .test_coarse_power = 3,
        .test_fine_power = 9,
        .n_test = 100
    };
    ed->txon_packet_length = 32;

    snprintf(ed->device_name, sizeof(ed->device_name)-1, "uwb_desense%d", desense->dev_inst->idx);

    ed->kobj = kobject_create_and_add(ed->device_name, kernel_kobj);
    if (!ed->kobj) {
        slog("Failed to create uwb_desense\n");
        return;
    }

    ed->cmd.dev_attr_cfg = kzalloc((ARRAY_SIZE(cmd_str))*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!ed->cmd.dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    ed->cmd.attrs = kzalloc((ARRAY_SIZE(cmd_str)+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!ed->cmd.attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    for (i=0;i<ARRAY_SIZE(cmd_str);i++) {
        ed->cmd.dev_attr_cfg[i] = DEV_ATTR_ROW(cmd_str[i], (void*)ed);
        ed->cmd.attrs[i] = &ed->cmd.dev_attr_cfg[i].attr.attr;
    }

    ed->cmd.attribute_group.attrs = ed->cmd.attrs;
    ret = sysfs_create_group(ed->kobj, &ed->cmd.attribute_group);
    if (ret) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }

    ed->kobj_params = kobject_create_and_add("params", ed->kobj);
    if (!ed->kobj_params) {
        slog("Failed to create uwb_desense/params\n");
        return;
    }

    ed->param.dev_attr_cfg = kzalloc((ARRAY_SIZE(param_str))*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!ed->param.dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    ed->param.attrs = kzalloc((ARRAY_SIZE(param_str)+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!ed->param.attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    for (i=0;i<ARRAY_SIZE(param_str);i++) {
        ed->param.dev_attr_cfg[i] = DEV_ATTR_ROW(param_str[i], (void*)ed);
        ed->param.attrs[i] = &ed->param.dev_attr_cfg[i].attr.attr;
    }

    ed->param.attribute_group.attrs = ed->param.attrs;
    ret = sysfs_create_group(ed->kobj_params, &ed->param.attribute_group);
    if (ret) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }
    return;

err_sysfs:
    kfree(ed->cmd.attrs);
    kfree(ed->param.attrs);
err_sysfs_attrs:
    kfree(ed->cmd.dev_attr_cfg);
    kfree(ed->param.dev_attr_cfg);
err_dev_attr_cfg:
    kobject_put(ed->kobj_params);
    kobject_put(ed->kobj);
    return;
}

void desense_sysfs_deinit(int idx)
{
    struct desense_sysfs_data *ed;
    if (idx >= ARRAY_SIZE(sysfs_inst)) {
        return;
    }
    ed = &sysfs_inst[idx];

    if (ed->kobj) {
        slog("");
        sysfs_remove_group(ed->kobj, &ed->cmd.attribute_group);
        sysfs_remove_group(ed->kobj_params, &ed->param.attribute_group);
        kfree(ed->cmd.attrs);
        kfree(ed->cmd.dev_attr_cfg);
        kfree(ed->param.attrs);
        kfree(ed->param.dev_attr_cfg);
        kobject_put(ed->kobj_params);
        kobject_put(ed->kobj);
    }
}
#endif
