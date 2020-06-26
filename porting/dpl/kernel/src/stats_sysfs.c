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
#include <inttypes.h>
#include <dpl/dpl.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <stats/stats.h>
#include "uwbcore.h"

#define slog(fmt, ...) \
    pr_info("uwbcore:stats: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static struct kobject *stats_kobj = 0;

struct display_entry_data {
    const char *name;
    char *buf;
    size_t length;
};

static int
stats_display_entry(struct stats_hdr *hdr, void *arg, char *name,
                    uint16_t stat_off)
{
    struct display_entry_data *d = (struct display_entry_data *)arg;
    void *stat_val;

    stat_val = (uint8_t *)hdr + stat_off;
    if (strcmp(d->name, name) != 0) {
        return 0;
    }
    switch (hdr->s_size) {
        case sizeof(uint16_t):
            d->length = sprintf(d->buf, "%s: %d\n", name,
                                *(uint16_t *) stat_val);
            break;
        case sizeof(uint32_t):
            d->length = sprintf(d->buf, "%s: %u\n", name,
                                *(uint32_t *) stat_val);
            break;
        case sizeof(uint64_t):
            d->length = sprintf(d->buf, "%s: %llu\n", name,
                                *(uint64_t *) stat_val);
            break;
        default:
            d->length = sprintf(d->buf, "Unknown stat size for %s %u\n", name,
                                hdr->s_size);
            break;
    }

    return (1);
}

static ssize_t
stat_show(struct device *dev,
          struct device_attribute *attr, char *buf)
{
    int rc;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct stats_hdr *hdr = (struct stats_hdr *)ext_dev->var;
    const char errmsg[] = "err, no such stat\n";
    struct display_entry_data d = {
        .name = attr->attr.name,
        .buf = buf,
        .length = 0};

    rc = stats_walk(hdr, stats_display_entry, &d);

    if (!d.length) {
        memcpy(buf, errmsg, strlen(errmsg));
        d.length=strlen(errmsg);
    }
    return d.length;
}

#define DEV_ATTR_ROW(__X, __Y)                                              \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X, .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO)}, \
                 .show = stat_show},                                    \
            .var=__Y }

static int
stats_init_entry(struct stats_hdr *hdr, void *arg, char *name,
                 uint16_t stat_off)
{
    int i = (stat_off-sizeof(struct stats_hdr)) / hdr->s_size;
    hdr->entry_names_cpy[i] = kzalloc(strlen(name)+1, GFP_KERNEL);
    if (!hdr->entry_names_cpy[i]) {
        slog("Failed to add entry with name '%s' at offset %d\n", name, stat_off);
        return -ENOMEM;
    }
    memcpy(hdr->entry_names_cpy[i], name, strlen(name)+1);
    hdr->dev_attr_cfg[i] = DEV_ATTR_ROW(hdr->entry_names_cpy[i], (void*)hdr);
    hdr->sysfs_attrs[i] = &hdr->dev_attr_cfg[i].attr.attr;
    return 0;
}

static int
stats_deinit_entry(struct stats_hdr *hdr, void *arg, char *name,
        uint16_t stat_off)
{
    int i = (stat_off-sizeof(struct stats_hdr)) / hdr->s_size;
    if (hdr->entry_names_cpy[i]) {
        kfree(hdr->entry_names_cpy[i]);
    }
    return 0;
}

static int
stats_init_group(struct stats_hdr *hdr, void *arg)
{
    int rc;
    struct kobject *parent_kobj = (struct kobject *)arg;
    hdr->kobj = kobject_create_and_add(hdr->s_name, parent_kobj);
    if (!hdr->kobj) {
        slog("Failed to create %s\n", hdr->s_name);
        return -1;
    }

    hdr->dev_attr_cfg = kzalloc((hdr->s_cnt)*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!hdr->dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg for %s\n", hdr->s_name);
        goto err_dev_attr_cfg;
    }
    /* Is this really correct below? Allocating room for a vector of pointers */
    hdr->sysfs_attrs = kzalloc((hdr->s_cnt+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!hdr->sysfs_attrs) {
        slog("Failed to create sysfs_attrs for %s\n", hdr->s_name);
        goto err_sysfs_attrs;
    }

    hdr->entry_names_cpy = kzalloc((hdr->s_cnt+1)*sizeof(char*), GFP_KERNEL);
    if (!hdr->entry_names_cpy) {
        slog("Failed to create entry_names_cpy for %s\n", hdr->s_name);
        goto err_sysfs_attrs;
    }
    rc = stats_walk(hdr, stats_init_entry, 0);

    hdr->attribute_group.attrs = hdr->sysfs_attrs;
    rc = sysfs_create_group(hdr->kobj, &hdr->attribute_group);
    if (rc) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }
    return (0);

err_sysfs:
    kfree(hdr->sysfs_attrs);
err_sysfs_attrs:
    kfree(hdr->dev_attr_cfg);
err_dev_attr_cfg:
    kobject_put(hdr->kobj);
    return -1;
}

static int
stats_deinit_group(struct stats_hdr *hdr, void *arg)
{
    int rc;
    rc = stats_walk(hdr, stats_deinit_entry, 0);

    sysfs_remove_group(hdr->kobj, &hdr->attribute_group);
    kfree(hdr->entry_names_cpy);
    kfree(hdr->sysfs_attrs);
    kfree(hdr->dev_attr_cfg);
    kobject_put(hdr->kobj);
    return 0;
}

void
stats_sysfs_update(void)
{
    if (stats_kobj) {
        slog("updating stats_sysfs\n");
        stats_group_walk(stats_deinit_group, stats_kobj);
        stats_group_walk(stats_init_group, stats_kobj);
    }
}

void
stats_sysfs_init(void)
{
    stats_kobj = kobject_create_and_add("stats", uwbcore_get_kobj());
    if (!stats_kobj) {
        slog("Failed to create stats_kobj\n");
        return;
    }
    stats_group_walk(stats_init_group, stats_kobj);
}

void stats_sysfs_down(void)
{
    if (stats_kobj) {
        slog("Removing stats sysfs\n");
        stats_group_walk(stats_deinit_group, stats_kobj);
        kobject_put(stats_kobj);
    }
}
#endif
