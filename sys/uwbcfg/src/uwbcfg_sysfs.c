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
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <uwbcfg/uwbcfg.h>
#include "uwbcfg_priv.h"
#include "uwbcore.h"

#define slog(fmt, ...) \
    pr_info("uwbcore: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static struct kobject *uwbcfg_kobj = 0;

static ssize_t cfg_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int i=0, len;
    const char errmsg[] = "err, no such config\n";
    const char commit_msg[] = "write to commit instead\n";

    if (!strcmp(attr->attr.name, "commit")) {
        memcpy(buf, commit_msg, strlen(commit_msg));
        return strlen(commit_msg);
    }

    for (i=0;i<CFGSTR_MAX;i++) {
        if (!strcmp(attr->attr.name, g_uwbcfg_str[i])) {
            break;
        }
    }
    if (i==CFGSTR_MAX) {
        memcpy(buf, errmsg, strlen(errmsg));
        return strlen(errmsg);
    }
    len = snprintf(buf, PAGE_SIZE-2, "uwb/%s %s\n", g_uwbcfg_str[i], g_uwb_config[i]);
    return len;
}

static ssize_t cfg_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int i=0;
    int to_cpy = (count < CFGSTR_MAX-1) ? count:CFGSTR_MAX-1;

    if (!strcmp(attr->attr.name, "commit")) {
        slog("applying config\n");
        uwbcfg_commit();
        return count;
    }

    for (i=0;i<CFGSTR_MAX;i++) {
        if (!strcmp(attr->attr.name, g_uwbcfg_str[i])) {
            break;
        }
    }
    if (i==CFGSTR_MAX) {
        return -EINVAL;
    }
    /* Trim any trailing characters */
    while (to_cpy > 0 && (buf[to_cpy-1] == '\n' || buf[to_cpy-1] == '\r')) {
        to_cpy--;
    }
    memcpy(g_uwb_config[i], buf, to_cpy);
    g_uwb_config[i][to_cpy] = 0;
    return count;
}

#define DEV_ATTR_ROW(__X)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = cfg_show, .store = cfg_store},                 \
            .var= NULL }

static struct dev_ext_attribute *dev_attr_cfg = 0;

static struct attribute** sysfs_attrs;

static struct attribute_group uwbcfg_attribute_group = {
    .attrs = 0,
};

void uwbcfg_sysfs_init(void)
{
    int i, ret;
    int num_items;

    uwbcfg_kobj = kobject_create_and_add("uwbcfg", uwbcore_get_kobj());
    if (!uwbcfg_kobj) {
        slog("Failed to create uwbcfg_kobj\n");
        return;
    }

    /* +1 for commit */
    num_items = CFGSTR_MAX + 1;
    dev_attr_cfg = kzalloc(num_items * sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    /* +1 for the last NULL element */
    sysfs_attrs = kzalloc((num_items+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!sysfs_attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    for (i=0;i<CFGSTR_MAX;i++) {
        dev_attr_cfg[i] = DEV_ATTR_ROW(g_uwbcfg_str[i]);
        sysfs_attrs[i] = &dev_attr_cfg[i].attr.attr;
    }
    dev_attr_cfg[CFGSTR_MAX] = DEV_ATTR_ROW("commit");
    sysfs_attrs[CFGSTR_MAX] = &dev_attr_cfg[CFGSTR_MAX].attr.attr;

    uwbcfg_attribute_group.attrs = sysfs_attrs;
    ret = sysfs_create_group(uwbcfg_kobj, &uwbcfg_attribute_group);
    if (ret) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }
    return;

err_sysfs:
    kfree(sysfs_attrs);
err_sysfs_attrs:
    kfree(dev_attr_cfg);
err_dev_attr_cfg:
    kobject_put(uwbcfg_kobj);
    return;
}

void uwbcfg_sysfs_deinit(void)
{
    if (uwbcfg_kobj) {
        sysfs_remove_group(uwbcfg_kobj, &uwbcfg_attribute_group);
        kfree(sysfs_attrs);
        kfree(dev_attr_cfg);
        kobject_put(uwbcfg_kobj);
    }
}
#endif
