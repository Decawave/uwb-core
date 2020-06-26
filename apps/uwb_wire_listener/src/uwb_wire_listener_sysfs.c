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

#include <uwb_wire_listener/uwb_wire_listener.h>

#define slog(fmt, ...) \
    pr_info("uwb_wire_listener: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

struct uwb_wire_listener_sysfs_data {
    struct kobject *kobj;
    char device_name[21];
    struct mutex local_lock;
    struct uwb_wire_listener_instance *listener;

    struct dev_ext_attribute *dev_attr_cfg;
    struct attribute** attrs;
    struct attribute_group attribute_group;
};

static struct uwb_wire_listener_sysfs_data uwb_wire_listener_sysfs_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};

static ssize_t uwb_wire_listener_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    const char errmsg_lsn[] = "Write a timeout in us to listen for packages," \
                                "\nor 0 to listen until a msg is received.\n";

    if (!strcmp(attr->attr.name, "listen")) {
        memcpy(buf, errmsg_lsn, strlen(errmsg_lsn));
        return strlen(errmsg_lsn);
    }
    memcpy(buf, errmsg_lsn, strlen(errmsg_lsn));
    return strlen(errmsg_lsn);
}

static ssize_t uwb_wire_listener_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc;
    long param;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct uwb_wire_listener_sysfs_data *ed = (struct uwb_wire_listener_sysfs_data *)ext_dev->var;

    rc = kstrtol(buf, 0, &param);
    if (rc!=0) {
        slog("Error, rx-timeout not valid: %s\n", buf);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&ed->local_lock)) {
        return -ERESTARTSYS;
    }

    if (!strcmp(attr->attr.name, "listen")) {
        uwb_set_rx_timeout(ed->listener->dev_inst, param);
        uwb_start_rx(ed->listener->dev_inst);
        if (signal_pending(current)) {
            slog("signal received");
            uwb_phy_forcetrxoff(ed->listener->dev_inst);
        }
    }

    mutex_unlock(&ed->local_lock);
    return count;
}

#define DEV_ATTR_ROW(__X, __Y)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = uwb_wire_listener_show, .store = uwb_wire_listener_store},                 \
            .var=__Y }

void uwb_wire_listener_sysfs_init(struct uwb_wire_listener_instance *listener)
{
    int i;
    int rc;
    struct uwb_wire_listener_sysfs_data *ed;
    if (listener->dev_inst->idx >= ARRAY_SIZE(uwb_wire_listener_sysfs_inst)) {
        return;
    }
    ed = &uwb_wire_listener_sysfs_inst[listener->dev_inst->idx];
    ed->listener = listener;

    mutex_init(&ed->local_lock);

    snprintf(ed->device_name, sizeof(ed->device_name)-1,
                            "uwb_wire_listener%d", listener->dev_inst->idx);

    ed->kobj = kobject_create_and_add(ed->device_name, kernel_kobj);
    if (!ed->kobj) {
        slog("Failed to create uwb_wire_listener_kobj\n");
        return;
    }

    ed->dev_attr_cfg = kzalloc((1)*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!ed->dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    ed->attrs = kzalloc((1+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!ed->attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    i = 0;
    slog("Adding listen\n");
    ed->dev_attr_cfg[i] = DEV_ATTR_ROW("listen", (void*)ed);
    ed->attrs[i] = &ed->dev_attr_cfg[i].attr.attr;
    i++;

    ed->attribute_group.attrs = ed->attrs;
    rc = sysfs_create_group(ed->kobj, &ed->attribute_group);
    if (rc) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }

    return;

err_sysfs:
    kfree(ed->attrs);
err_sysfs_attrs:
    kfree(ed->dev_attr_cfg);
err_dev_attr_cfg:
    kobject_put(ed->kobj);
    return;
}

void uwb_wire_listener_sysfs_deinit(int idx)
{
    struct uwb_wire_listener_sysfs_data *ed;
    if (idx >= ARRAY_SIZE(uwb_wire_listener_sysfs_inst)) {
        return;
    }
    ed = &uwb_wire_listener_sysfs_inst[idx];

    if (ed->kobj) {
        slog("");
        mutex_destroy(&ed->local_lock);
        sysfs_remove_group(ed->kobj, &ed->attribute_group);
        kfree(ed->attrs);
        kfree(ed->dev_attr_cfg);
        kobject_put(ed->kobj);
    }
}

#endif  /* __KERNEL__ */
