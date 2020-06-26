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

#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#include <linux/signal.h>
#else
#include <linux/sched/signal.h>
#endif
#include "uwbcore.h"
#include <uwb_ccp/uwb_ccp.h>

#define slog(fmt, ...) \
    pr_info("uwb_ccp_sysfs: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

const char* g_uwbccp_str[] = {
    "enable",
    "role",
    "period"
};

struct ccp_sysfs_data {
    struct kobject *kobj;
    char device_name[16];
    struct mutex local_lock;
    struct uwb_ccp_instance *ccp;

    struct dev_ext_attribute *dev_attr_cfg;
    struct attribute** attrs;
    struct attribute_group attribute_group;
};

static struct ccp_sysfs_data ccp_sysfs_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};


static ssize_t ccp_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct ccp_sysfs_data *ed = (struct ccp_sysfs_data *)ext_dev->var;

    if (!strcmp(attr->attr.name, "enable")) {
        return sprintf(buf,"%d\n", ed->ccp->status.enabled);
    }
    else if (!strcmp(attr->attr.name, "role")) {
        return sprintf(buf,"%d\n", ed->ccp->config.role);
    }
    else if (!strcmp(attr->attr.name, "period")) {
        return sprintf(buf,"0x%X\n", ed->ccp->period);
    }
    return 0;
}

static ssize_t ccp_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc;
    long param;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct ccp_sysfs_data *ed = (struct ccp_sysfs_data *)ext_dev->var;

    rc = kstrtol(buf, 0, &param);
    if (rc!=0) {
        slog("Error, Invalid param %s\n", buf);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&ed->local_lock)) {
        return -ERESTARTSYS;
    }

    if (!strcmp(attr->attr.name, "enable")) {
        if(param){
            if (ed->ccp->status.enabled){
                pr_info("uwbccp: Stopping service\n");
                uwb_ccp_stop(ed->ccp);
            }
            pr_info("uwbccp: Starting service\n");
            uwb_ccp_start(ed->ccp, ed->ccp->config.role);
        }
        else
        {
            if (ed->ccp->status.enabled){
                pr_info("uwbccp: Stopping service\n");
                uwb_ccp_stop(ed->ccp);
            }
        }
    }
    else if (!strcmp(attr->attr.name, "role")) {
        switch(param){
            case CCP_ROLE_MASTER...CCP_ROLE_RELAY:
                pr_info("uwbccp: role set\n");
                ed->ccp->config.role = param;
                break;
            default:
                break;
        }
    }
    else if (!strcmp(attr->attr.name, "period")) {
        if (param & 0xFFFF){
            pr_info("uwbccp: Period not set. Period should be a multiple of 0x10000\n");
        }
        else{
            pr_info("uwbccp: Period set\n");
            ed->ccp->period=param;
        }
    }

    mutex_unlock(&ed->local_lock);
    return count;
}

#define DEV_ATTR_ROW(__X, __Y)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = ccp_show, .store = ccp_store},                 \
            .var=__Y }


int ccp_sysfs_init(struct uwb_ccp_instance *ccp)
{
    int i, ret;
    struct ccp_sysfs_data *ed;
    if (ccp->dev_inst->idx >= ARRAY_SIZE(ccp_sysfs_inst)) {
        return -EINVAL;
    }
    ed = &ccp_sysfs_inst[ccp->dev_inst->idx];
    ed->ccp = ccp;

    mutex_init(&ed->local_lock);

    snprintf(ed->device_name, sizeof(ed->device_name)-1, "uwbccp%d", ccp->dev_inst->idx);

    ed->kobj = kobject_create_and_add(ed->device_name, uwbcore_get_kobj());
    if (!ed->kobj) {
        slog("Failed to create uwb_ccp_kobj\n");
        return -ENOMEM;
    }

    ed->dev_attr_cfg = kzalloc((ARRAY_SIZE(g_uwbccp_str))*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!ed->dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    ed->attrs = kzalloc((ARRAY_SIZE(g_uwbccp_str)+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!ed->attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    for (i=0;i<ARRAY_SIZE(g_uwbccp_str);i++) {
        ed->dev_attr_cfg[i] = DEV_ATTR_ROW(g_uwbccp_str[i], ed);
        ed->attrs[i] = &ed->dev_attr_cfg[i].attr.attr;
    }

    ed->attribute_group.attrs = ed->attrs;
    ret = sysfs_create_group(ed->kobj, &ed->attribute_group);
    if (ret) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }

    return 0;

err_sysfs:
    kfree(ed->attrs);
err_sysfs_attrs:
    kfree(ed->dev_attr_cfg);
err_dev_attr_cfg:
    kobject_put(ed->kobj);
    return -ENOMEM;
}

void ccp_sysfs_deinit(int idx)
{
    struct ccp_sysfs_data *ed;
    if (idx >= ARRAY_SIZE(ccp_sysfs_inst)) {
        return;
    }
    ed = &ccp_sysfs_inst[idx];

    if (ed->kobj) {
        mutex_destroy(&ed->local_lock);
        sysfs_remove_group(ed->kobj, &ed->attribute_group);
        kfree(ed->attrs);
        kfree(ed->dev_attr_cfg);
        kobject_put(ed->kobj);
    }
}

#endif  /* __KERNEL__ */
