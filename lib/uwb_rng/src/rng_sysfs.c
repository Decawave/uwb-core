#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#include <linux/signal.h>
#else
#include <linux/sched/signal.h>
#endif

#include "uwbcore.h"
#include <uwb_rng/uwb_rng.h>

#define slog(fmt, ...) \
    pr_info("uwbcore:rng: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

struct rng_sysfs_data {
    struct kobject *kobj;
    char device_name[16];
    struct mutex lock;
    struct uwb_rng_instance *rng;

    struct dev_ext_attribute *dev_attr_cfg;
    struct attribute** attrs;
    struct attribute_group attribute_group;
};

static struct rng_sysfs_data rng_sysfs_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};

static ssize_t rng_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    const char errmsg_req[] = "Write an address to range to instead of reading.\n";
    const char errmsg_lsn[] = "Write a timeout in us to listen for ranges," \
        "\nor 0 to listen until a msg is received.\n";

    if (!strcmp(attr->attr.name, "listen")) {
        memcpy(buf, errmsg_lsn, strlen(errmsg_lsn));
        return strlen(errmsg_lsn);
    }
    memcpy(buf, errmsg_req, strlen(errmsg_req));
    return strlen(errmsg_req);
}

static ssize_t rng_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc;
    long param;
    struct rng_config_list * cfgs;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct rng_sysfs_data *ed = (struct rng_sysfs_data *)ext_dev->var;

    rc = kstrtol(buf, 0, &param);
    if (rc!=0) {
        slog("Error, address / rx-timeout not valid: %s\n", buf);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&ed->lock)) {
        return -ERESTARTSYS;
    }

    if (!strcmp(attr->attr.name, "listen")) {
        slog("rng listen: timeout:%lduwb-us\n", param);
        uwb_rng_listen(ed->rng, param, UWB_BLOCKING);
        if (signal_pending(current)) {
            slog("signal received");
            uwb_phy_forcetrxoff(ed->rng->dev_inst);
        }
    }

    if(!(SLIST_EMPTY(&ed->rng->rng_configs))){
        SLIST_FOREACH(cfgs, &ed->rng->rng_configs, next){
            if (cfgs->name == attr->attr.name) {
                slog("rng request: type:%s dest:0x%lx\n", cfgs->name, param);
                if (dpl_sem_get_count(&ed->rng->sem) == 0) {
                    slog("WARNING: rng-sem blocked, issuing forcetrxoff\n");
                    uwb_phy_forcetrxoff(ed->rng->dev_inst);
                }
                uwb_rng_request(ed->rng, param, cfgs->rng_code);
                if (signal_pending(current)) {
                    slog("signal received");
                    uwb_phy_forcetrxoff(ed->rng->dev_inst);
                }
            }
        }
    }

    mutex_unlock(&ed->lock);
    return count;
}

#define DEV_ATTR_ROW(__X, __Y)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = rng_show, .store = rng_store},                 \
            .var=__Y }

int uwbrng_sysfs_init(struct uwb_rng_instance *rng)
{
    int i, n_rngtypes = 1;      /* 1 as listen is the first option */
    int rc;
    struct rng_config_list * cfgs;
    struct rng_sysfs_data *ed;
    if (rng->dev_inst->idx >= ARRAY_SIZE(rng_sysfs_inst)) {
        return -EINVAL;
    }
    ed = &rng_sysfs_inst[rng->dev_inst->idx];
    ed->rng = rng;

    mutex_init(&ed->lock);

    snprintf(ed->device_name, sizeof(ed->device_name)-1, "uwbrng%d", rng->dev_inst->idx);

    if(!(SLIST_EMPTY(&rng->rng_configs))){
        SLIST_FOREACH(cfgs, &rng->rng_configs, next){
            n_rngtypes++;
        }
    }

    ed->kobj = kobject_create_and_add(ed->device_name, uwbcore_get_kobj());
    if (!ed->kobj) {
        slog("Failed to create uwbrng_kobj\n");
        return -ENOMEM;
    }

    ed->dev_attr_cfg = kzalloc((n_rngtypes)*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!ed->dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    ed->attrs = kzalloc((n_rngtypes+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!ed->attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    i=0;
    ed->dev_attr_cfg[i] = DEV_ATTR_ROW("listen", (void*)ed);
    ed->attrs[i] = &ed->dev_attr_cfg[i].attr.attr;
    i++;
    if(!(SLIST_EMPTY(&rng->rng_configs))){
        SLIST_FOREACH(cfgs, &rng->rng_configs, next){
            ed->dev_attr_cfg[i] = DEV_ATTR_ROW(cfgs->name, (void*)ed);
            ed->attrs[i] = &ed->dev_attr_cfg[i].attr.attr;
            i++;
        }
    }

    ed->attribute_group.attrs = ed->attrs;
    rc = sysfs_create_group(ed->kobj, &ed->attribute_group);
    if (rc) {
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

void uwbrng_sysfs_deinit(int idx)
{
    struct rng_sysfs_data *ed;
    if (idx >= ARRAY_SIZE(rng_sysfs_inst)) {
        return;
    }
    ed = &rng_sysfs_inst[idx];

   if (ed->kobj) {
        mutex_destroy(&ed->lock);
        sysfs_remove_group(ed->kobj, &ed->attribute_group);
        kfree(ed->attrs);
        kfree(ed->dev_attr_cfg);
        kobject_put(ed->kobj);
    }
}

#endif  /* __KERNEL__ */
