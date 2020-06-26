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
#include <linux/cdev.h>
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
#include <syscfg/syscfg.h>


#define slog(fmt, ...) \
    pr_info("uwb_ccp_chrdev: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define CCP_CHRDEV_NAME "uwbccp"
#define FIFO_SIZE PAGE_SIZE*4

typedef STRUCT_KFIFO_REC_2(FIFO_SIZE) local_record_fifo_t;

struct ccp_encode_data {
    struct mutex read_lock;
    struct mutex write_lock;
    local_record_fifo_t fifo;
    int have_init;
	struct device *dev;
	struct cdev cdev;
	struct class *class;
    int major;
    struct fasync_struct *async_queue;
    wait_queue_head_t wait;
    char device_name[16];
};

static struct ccp_encode_data ccp_encode_inst[MYNEWT_VAL(UWB_DEVICE_MAX)] = {0};

static int
ccp_chrdev_open(struct inode *inode, struct file *file)
{
    struct ccp_encode_data *ed = NULL;

    ed = container_of(inode->i_cdev, struct ccp_encode_data, cdev);
    if (!ed) {
        slog("Failed to get ccp_encode_data private data\n");
        return -ENODEV;
    }

    file->private_data = ed;
    return 0;
}

static int
ccp_fasync(int fd, struct file *filp, int mode)
{
    struct ccp_encode_data *ed = filp->private_data;
    return fasync_helper(fd, filp, mode, &ed->async_queue);
}

static int
ccp_chrdev_release(struct inode *inode, struct file *file)
{
    ccp_fasync(-1, file, 0);
    file->private_data = NULL;
    return 0;
}

static ssize_t
ccp_chrdev_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
	int ret;
	unsigned int copied;
    struct ccp_encode_data *ed = file->private_data;

    if (kfifo_is_empty(&ed->fifo)) {
        if (file->f_flags & O_NONBLOCK)
            // if nonblock is specified, we dont block and just return 'try again'
            return -EAGAIN;
    }

    wait_event_interruptible(ed->wait, !kfifo_is_empty(&ed->fifo));
    if (signal_pending(current)) {
        /* If we were interrupted whilst waiting */
        return -ERESTARTSYS;
    }

	if (mutex_lock_interruptible(&ed->read_lock))
		return -ERESTARTSYS;

	ret = kfifo_to_user(&ed->fifo, buf, len, &copied);

	mutex_unlock(&ed->read_lock);
	return ret ? ret : copied;
}

static unsigned int
ccp_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
    struct ccp_encode_data *ed = filp->private_data;
    poll_wait(filp, &ed->wait, wait);

    /* Serialize access to buffer */
	if (mutex_lock_interruptible(&ed->read_lock))
		return -ERESTARTSYS;

    // READ
    if(!kfifo_is_empty(&ed->fifo)) {// buffer not empty. read wont block
        slog(" POLLIN EVENT\n");
        mask |= POLLIN | POLLRDNORM;  /* fd is readable */
    }

	mutex_unlock(&ed->read_lock);

	return mask;
}

static const struct file_operations ccp_chrdev_fops = {
    .owner   = THIS_MODULE,
    .open    = ccp_chrdev_open,
    .release = ccp_chrdev_release,
    .read    = ccp_chrdev_read,
    .write   = NULL,
    .poll    = ccp_poll,
};

static char*
ccp_chrdev_devnode(struct device *dev, umode_t *mode)
{
    if (!mode) {
        return NULL;
    }

    *mode = 0444;
    return NULL;
}

int
ccp_chrdev_create(int idx)
{
    int ret = 0;
    struct device *device;
    struct ccp_encode_data *ed;
	dev_t devt;
    if (idx >= ARRAY_SIZE(ccp_encode_inst)) {
        return -EINVAL;
    }
    ed = &ccp_encode_inst[idx];

    snprintf(ed->device_name, sizeof(ed->device_name)-1, "%s%d", CCP_CHRDEV_NAME, idx);

    ret = alloc_chrdev_region(&devt, 0, 1, ed->device_name);
    ed->major = MAJOR(devt);
    if (ret<0) {
        slog("Failed to allocate char dev region\n");
        return ret;
    }

    cdev_init(&ed->cdev, &ccp_chrdev_fops);
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
        slog("Unable to create ccp_chrdev class, err: %d\n", ret);
        goto err_class;
    }

    ed->class->devnode = ccp_chrdev_devnode;
    device = device_create(ed->class, NULL, MKDEV(ed->major, 0), NULL, ed->device_name);
    if (IS_ERR(device)) {
        ret = PTR_ERR(device);
        slog("Unable to create ccp_chrdev device, err: %d\n", ret);
        goto err_device;
    }

    mutex_init(&ed->read_lock);
    mutex_init(&ed->write_lock);
    INIT_KFIFO(ed->fifo);
    init_waitqueue_head(&ed->wait);

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
ccp_chrdev_destroy(int idx)
{
    struct ccp_encode_data *ed;
    if (idx >= ARRAY_SIZE(ccp_encode_inst)) {
        return;
    }
    ed = &ccp_encode_inst[idx];

    if (!ed->have_init) {
        return;
    }
    mutex_destroy(&ed->read_lock);
    mutex_destroy(&ed->write_lock);
    // cdev_del(&ed->cdev);  /* This causes a double kobject_put? */
    device_destroy(ed->class, MKDEV(ed->major, 0));
    class_destroy(ed->class);
    unregister_chrdev_region(MKDEV(ed->major, 0), 1);
}

int
ccp_chrdev_output(int idx, char *buf, size_t len)
{
    int ret;
    struct ccp_encode_data *ed;
    if (idx >= ARRAY_SIZE(ccp_encode_inst)) {
        return -EINVAL;
    }
    ed = &ccp_encode_inst[idx];
    if (!ed->have_init) {
        return 0;
    }

	if (mutex_lock_interruptible(&ed->write_lock))
		return -ERESTARTSYS;

retry:
    ret = kfifo_in(&ed->fifo, buf, len);
    if (ret == 0) {
        if (mutex_lock_interruptible(&ed->read_lock)) {
            return -ERESTARTSYS;
        }
        kfifo_skip(&ed->fifo);
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

#endif  /* __KERNEL__ */
