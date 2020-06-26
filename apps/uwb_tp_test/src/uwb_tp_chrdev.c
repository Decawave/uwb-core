#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#include <linux/signal.h>
#else
#include <linux/sched/signal.h>
#endif
#include <linux/kfifo.h>

#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <dpl/dpl_eventq.h>
#include <dpl/dpl_mbuf.h>

#include <uwb/uwb.h>
#include <uwb_transport/uwb_transport.h>

#define slog(fmt, ...)                                                  \
    pr_info("uwb_tp_test: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define UWB_TP_CHRDEV_NAME "uwbtp"
#define FIFO_SIZE PAGE_SIZE*4

typedef STRUCT_KFIFO_REC_2(FIFO_SIZE) local_record_fifo_t;

struct uwbtp_encode_data {
    struct mutex read_lock;
    struct mutex write_lock;
    struct mutex output_lock;
    local_record_fifo_t read_fifo;
    int have_init;

    char device_name[16];
    struct device *dev;
    struct cdev cdev;
    struct class *class;
    int major;
    struct fasync_struct *async_queue;
    wait_queue_head_t wait;
    struct _uwb_transport_instance *uwb_transport;
    u8 write_buffer[PAGE_SIZE];
    u8 output_buffer[PAGE_SIZE];

    struct _uwb_transport_extension extension;
    size_t mtu;

    struct kobject *kobj;
    uint16_t dest_uid;
    uint8_t retries;
    struct dev_ext_attribute *dev_attr_cfg;
    struct attribute** attrs;
    struct attribute_group attribute_group;
};

static struct uwbtp_encode_data uwbtp_encode_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};

static const char* sysfs_str[] = {
    "dest_uid",
    "retries",
    "mtu",
};


static int
uwb_tp_chrdev_open(struct inode *inode, struct file *file)
{
    struct uwbtp_encode_data *ed = NULL;

    ed = container_of(inode->i_cdev, struct uwbtp_encode_data, cdev);
    if (!ed) {
        slog("Failed to get uwbuwb_tp_encode_data private data\n");
        return -ENODEV;
    }

    file->private_data = ed;
    return 0;
}

static int
uwb_tp_fasync(int fd, struct file *filp, int mode)
{
    struct uwbtp_encode_data *ed = filp->private_data;
    return fasync_helper(fd, filp, mode, &ed->async_queue);
}

static int
uwb_tp_chrdev_release(struct inode *inode, struct file *file)
{
    uwb_tp_fasync(-1, file, 0);
    file->private_data = NULL;
    return 0;
}

static ssize_t
uwb_tp_chrdev_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    int ret;
    unsigned int copied;
    struct uwbtp_encode_data *ed = file->private_data;

    if (kfifo_is_empty(&ed->read_fifo)) {
        if (file->f_flags & O_NONBLOCK)
            // if nonblock is specified, we dont block and just return 'try again'
            return -EAGAIN;
    }

    wait_event_interruptible(ed->wait, !kfifo_is_empty(&ed->read_fifo));
    if (signal_pending(current)) {
        /* If we were interrupted whilst waiting */
        return -ERESTARTSYS;
    }

    if (mutex_lock_interruptible(&ed->read_lock))
        return -ERESTARTSYS;

    ret = kfifo_to_user(&ed->read_fifo, buf, len, &copied);

    mutex_unlock(&ed->read_lock);
    return ret ? ret : copied;
}

static bool
write_would_block(struct _uwb_transport_instance *uwb_transport)
{
    struct dpl_mbuf * mbuf;
    mbuf = uwb_transport_new_mbuf(uwb_transport);
    if (!mbuf) {
        return true;
    }
    dpl_mbuf_free_chain(mbuf);
    return false;
}

static ssize_t
uwb_tp_chrdev_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    int ret = 0, new_mtu;
    size_t input_offs = 0, write_buf_len, to_copy = 0;
    struct uwbtp_encode_data *ed;
    struct dpl_mbuf * mbuf;
    if (!file) {
        return -EAGAIN;
    }
    ed = file->private_data;
    new_mtu = uwb_transport_mtu(0, ed->uwb_transport->dev_inst->idx);
    if (!ed->mtu || new_mtu < ed->mtu) {
        ed->mtu = new_mtu;
    }

    /* Sanity check MTU */
    if (ed->mtu < 0 || ed->mtu > 497) ed->mtu = 497;

    /* Check if this write would block */
    if (write_would_block(ed->uwb_transport)) {
        if (file->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
    }

    wait_event_interruptible(ed->wait, !write_would_block(ed->uwb_transport));
    if (signal_pending(current)) {
        /* If we were interrupted whilst waiting */
        return -ERESTARTSYS;
    }

    if (mutex_lock_interruptible(&ed->write_lock)) {
        return -ERESTARTSYS;
    }

    write_buf_len = (len < sizeof(ed->write_buffer)) ? len : sizeof(ed->write_buffer);
    ret = copy_from_user(ed->write_buffer, buf, write_buf_len);
    if (ret) {
        slog("Failed to copy from userspace");
        goto early_exit;
    }

    while (input_offs < write_buf_len) {
        to_copy = write_buf_len - input_offs;
        to_copy = (to_copy > ed->mtu) ? ed->mtu : to_copy;

        mbuf = uwb_transport_new_mbuf(ed->uwb_transport);
        if (!mbuf) {
            /* Failed to get a memory buffer, abort */
            break;
        }

        if (dpl_mbuf_copyinto(mbuf, 0, ed->write_buffer + input_offs, to_copy) == 0) {
            input_offs += to_copy;
            uwb_transport_enqueue_tx(ed->uwb_transport, ed->dest_uid, 0xDEAD, ed->retries, mbuf);
            mbuf = 0;
        } else {
            /* Failed to write into memory buffer, abort */
            dpl_mbuf_free_chain(mbuf);
            slog("Failed to copy into mbuf, o:%zd c:%zd", input_offs, to_copy);
            break;
        }
    }

early_exit:
    mutex_unlock(&ed->write_lock);
    return ret ? ret : input_offs;
}

static unsigned int
uwb_tp_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct uwbtp_encode_data *ed = filp->private_data;

    poll_wait(filp, &ed->wait, wait);

    /* Serialize access to buffer */
    if (mutex_lock_interruptible(&ed->read_lock))
        return -ERESTARTSYS;

    // READ
    if(!kfifo_is_empty(&ed->read_fifo)) {// buffer not empty. read wont block
        slog(" POLLIN EVENT\n");
        mask |= POLLIN | POLLRDNORM;  /* fd is readable */
    }

    mutex_unlock(&ed->read_lock);

    if (mutex_lock_interruptible(&ed->write_lock))
        return -ERESTARTSYS;

    // WRITE
    if (!write_would_block(ed->uwb_transport)) {
        slog("wr possible");
        mask |= POLLOUT | POLLWRNORM;  /* writable */
    }
    mutex_unlock(&ed->write_lock);

    return mask;
}


static const struct file_operations uwb_tp_chrdev_fops = {
    .owner   = THIS_MODULE,
    .open    = uwb_tp_chrdev_open,
    .release = uwb_tp_chrdev_release,
    .read    = uwb_tp_chrdev_read,
    .write   = uwb_tp_chrdev_write,
    .poll    = uwb_tp_poll,
};

static char*
uwb_tp_chrdev_devnode(struct device *dev, umode_t *mode)
{
    if (!mode) {
        return NULL;
    }

    *mode = 0666;
    return NULL;
}

int
uwb_tp_output(int idx, char *buf, size_t len)
{
    int ret;
    struct uwbtp_encode_data *ed;

    if (!len) {
        return 0;
    }
    if (idx >= ARRAY_SIZE(uwbtp_encode_inst)) {
        return -EINVAL;
    }
    ed = &uwbtp_encode_inst[idx];
    if (!ed->have_init) {
        return 0;
    }

    if (mutex_lock_interruptible(&ed->write_lock))
        return -ERESTARTSYS;
retry:
    ret = kfifo_in(&ed->read_fifo, buf, len);
    if (ret == 0) {
        /* fifo full, skip one item */
        if (mutex_lock_interruptible(&ed->read_lock)) {
            return -ERESTARTSYS;
        }
        kfifo_skip(&ed->read_fifo);
        mutex_unlock(&ed->read_lock);
        goto retry;
    }
    mutex_unlock(&ed->write_lock);

    wake_up_interruptible(&ed->wait);
    if (ed->async_queue) {
        kill_fasync(&ed->async_queue, SIGIO, POLL_IN);
    }
    return ret;
}

static bool
uwb_transport_rxcb(struct uwb_dev * inst, uint16_t uid, struct dpl_mbuf * mbuf)
{
    uint16_t len = DPL_MBUF_PKTLEN(mbuf);
    struct uwbtp_encode_data *ed;
    if (inst->idx >= ARRAY_SIZE(uwbtp_encode_inst)) {
        return -EINVAL;
    }
    ed = &uwbtp_encode_inst[inst->idx];

    if (mutex_lock_interruptible(&ed->output_lock)) {
        slog("Lock failed");
        return -ERESTARTSYS;
    }
    dpl_mbuf_copydata(mbuf, 0, sizeof(ed->output_buffer), ed->output_buffer);
    dpl_mbuf_free_chain(mbuf);
    uwb_tp_output(inst->idx, ed->output_buffer, len);

    mutex_unlock(&ed->output_lock);
    return true;
}

static bool
uwb_transport_txcb(struct uwb_dev * inst)
{
    struct uwbtp_encode_data *ed;
    if (inst->idx >= ARRAY_SIZE(uwbtp_encode_inst)) {
        return -EINVAL;
    }
    ed = &uwbtp_encode_inst[inst->idx];

    wake_up_interruptible(&ed->wait);
    if (ed->async_queue) {
        kill_fasync(&ed->async_queue, SIGIO, POLL_OUT);
    }
    return true;
}

static ssize_t tp_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct uwbtp_encode_data *ed = (struct uwbtp_encode_data *)ext_dev->var;

    if (!strcmp(attr->attr.name, "dest_uid")) {
        return sprintf(buf,"0x%X\n", ed->dest_uid);
    }
    if (!strcmp(attr->attr.name, "retries")) {
        return sprintf(buf,"%d\n", ed->retries);
    }
    if (!strcmp(attr->attr.name, "mtu")) {
        return sprintf(buf,"%zd\n", ed->mtu);
    }
    return 0;
}

static ssize_t tp_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc;
    long param;
    struct dev_ext_attribute *ext_dev = (struct dev_ext_attribute *)attr;
    struct uwbtp_encode_data *ed = (struct uwbtp_encode_data *)ext_dev->var;

    rc = kstrtol(buf, 0, &param);
    if (rc!=0) {
        slog("Error, Invalid param %s\n", buf);
        return -EINVAL;
    }

    if (!strcmp(attr->attr.name, "dest_uid")) {
        ed->dest_uid = param & 0xffff;
    }

    if (!strcmp(attr->attr.name, "retries")) {
        ed->retries = param & 0xff;
    }

    if (!strcmp(attr->attr.name, "mtu")) {
        ed->mtu = param & 0xfff;
    }

    return count;
}

#define DEV_ATTR_ROW(__X, __Y)                                          \
    (struct dev_ext_attribute){                                         \
        .attr = {.attr = {.name = __X,                                  \
                          .mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR|S_IWGRP)}, \
                 .show = tp_show, .store = tp_store},                 \
            .var=__Y }


int tp_sysfs_init(struct uwbtp_encode_data *ed)
{
    int i, ret;

    ed->kobj = kobject_create_and_add(ed->device_name, kernel_kobj);
    if (!ed->kobj) {
        slog("Failed to create uwb_tp_kobj\n");
        return -ENOMEM;
    }

    ed->dev_attr_cfg = kzalloc((ARRAY_SIZE(sysfs_str))*sizeof(struct dev_ext_attribute), GFP_KERNEL);
    if (!ed->dev_attr_cfg) {
        slog("Failed to create dev_attr_cfg\n");
        goto err_dev_attr_cfg;
    }

    ed->attrs = kzalloc((ARRAY_SIZE(sysfs_str)+1)*sizeof(struct attribute *), GFP_KERNEL);
    if (!ed->attrs) {
        slog("Failed to create sysfs_attrs\n");
        goto err_sysfs_attrs;
    }

    for (i=0;i<ARRAY_SIZE(sysfs_str);i++) {
        ed->dev_attr_cfg[i] = DEV_ATTR_ROW(sysfs_str[i], ed);
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


int
uwb_tp_chrdev_create(int idx, struct _uwb_transport_instance *uwb_transport)
{
    int ret = 0;
    struct device *device;
    dev_t devt;
    struct uwbtp_encode_data *ed;
    if (idx >= ARRAY_SIZE(uwbtp_encode_inst)) {
        return -EINVAL;
    }
    ed = &uwbtp_encode_inst[idx];
    ed->uwb_transport = uwb_transport;
    if (!ed->mtu) {
        ed->mtu = uwb_transport_mtu(0, idx);
    }

    snprintf(ed->device_name, sizeof(ed->device_name)-1, "%s%d", UWB_TP_CHRDEV_NAME, idx);
    ret = alloc_chrdev_region(&devt, 0, 1, ed->device_name);
    ed->major = MAJOR(devt);
    if (ret<0) {
        slog("Failed to allocate char dev region\n");
        return ret;
    }
    /* Default 8 retries */
    ed->retries = 8;

    cdev_init(&ed->cdev, &uwb_tp_chrdev_fops);
    ed->cdev.owner = THIS_MODULE;

    /* Add the device */
    ret = cdev_add(&ed->cdev, MKDEV(ed->major, 0), 1);
    if (ret < 0) {
        slog("Cannot register a device,  err: %d", ret);
        goto err_cdev;
    }

    ed->class = class_create(THIS_MODULE, ed->device_name);
    if (IS_ERR(ed->class)) {
        ret = PTR_ERR(ed->class);
        slog("Unable to create uwb_tp_chrdev class, err: %d\n", ret);
        goto err_class;
    }

    ed->class->devnode = uwb_tp_chrdev_devnode;
    device = device_create(ed->class, NULL, MKDEV(ed->major, 0), NULL, ed->device_name);
    if (IS_ERR(device)) {
        ret = PTR_ERR(device);
        slog("Unable to create uwb_tp_chrdev device, err: %d\n", ret);
        goto err_device;
    }

    mutex_init(&ed->read_lock);
    mutex_init(&ed->write_lock);
    mutex_init(&ed->output_lock);
    INIT_KFIFO(ed->read_fifo);
    init_waitqueue_head(&ed->wait);

    ed->extension = (struct _uwb_transport_extension) {
        .tsp_code = 0xDEAD,
        .receive_cb = uwb_transport_rxcb,
        .transmit_cb = uwb_transport_txcb
    };
    uwb_transport_append_extension(ed->uwb_transport, &ed->extension);

    /* Setup sysfs */
    tp_sysfs_init(ed);

    ed->have_init = 1;
    return 0;

err_device:
    class_destroy(ed->class);
err_class:
    cdev_del(&ed->cdev);
err_cdev:
    unregister_chrdev_region(MKDEV(ed->major, 0), 1);

    return ret;
}

void
uwb_tp_chrdev_destroy(int idx)
{
    struct uwbtp_encode_data *ed;
    if (idx >= ARRAY_SIZE(uwbtp_encode_inst)) {
        return;
    }
    ed = &uwbtp_encode_inst[idx];

    if (!ed->have_init) {
        return;
    }

    uwb_transport_remove_extension(ed->uwb_transport, ed->extension.tsp_code);

    mutex_destroy(&ed->read_lock);
    mutex_destroy(&ed->write_lock);
    mutex_destroy(&ed->output_lock);
    device_destroy(ed->class, MKDEV(ed->major, 0));
    class_destroy(ed->class);
    unregister_chrdev_region(MKDEV(ed->major, 0), 1);

    if (ed->kobj) {
        sysfs_remove_group(ed->kobj, &ed->attribute_group);
        kfree(ed->attrs);
        kfree(ed->dev_attr_cfg);
        kobject_put(ed->kobj);
    }
}

#endif  /* __KERNEL__ */
