#include <linux/fs.h>
#include <linux/device.h>
#include <linux/percpu.h>
#include <linux/uio_driver.h>
#include <linux/uwb-hal.h>
#include <linux/cpuidle.h>
#include <linux/pm_qos.h>

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("UWB HAL Driver");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("spi:uwbhal");

#define UWBHAL_NAME "uwbhal"
#define UWBHAL_MAX_INSTANCES (2)
#define slog(fmt, ...) \
    pr_info("uwbhal: %s(): %d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static struct uwbhal_data* g_uwbhal_instances[UWBHAL_MAX_INSTANCES] = {0};
static int g_uwbhal_instances_n = 0;

struct uwbhal_data* uwbhal_get_instance(int idx)
{
    if (idx >= UWBHAL_MAX_INSTANCES) {
        return ERR_PTR(-EINVAL);
    }
    return g_uwbhal_instances[idx];
}
EXPORT_SYMBOL(uwbhal_get_instance);

int uwbhal_spi_transfer(struct uwbhal_data *uwbhal,
                        u8 *tx_buf, u8 *rx_buf, int len)
{
    int ret = 0;
    struct spi_transfer msg = {
        .tx_buf	= tx_buf,
        .rx_buf	= rx_buf,
        .len	= len,
    };

    mutex_lock(&uwbhal->spilock);
    ret = spi_sync_transfer(uwbhal->spi, &msg, 1);
    if (ret) {
        slog("SPI transfer failed: %d\n", ret);
    }
    mutex_unlock(&uwbhal->spilock);

    return ret;
}
EXPORT_SYMBOL(uwbhal_spi_transfer);


irqreturn_t uwbhal_irq_handler(int irq, struct uio_info *data)
{
    struct uwbhal_data *uwbhal = (struct uwbhal_data *)data->priv;

    if (uwbhal->irq_state == UWBHAL_IRQ_STATE_DIS) {
        goto early_exit;
    }

    if (uwbhal->irq_direct_cb) {
        uwbhal->irq_direct_cb(uwbhal->irq_direct_cb_arg);
    }
early_exit:
    return IRQ_HANDLED;
}

int uwbhal_irq_cb_set(struct uwbhal_data *uwbhal, uwbhal_irq_cb_fn cb, void *arg)
{
    if (!uwbhal) return -EINVAL;
    uwbhal->irq_direct_cb = cb;
    uwbhal->irq_direct_cb_arg = arg;
    return 0;
}
EXPORT_SYMBOL(uwbhal_irq_cb_set);

/* Device Control : VDD control */
int uwbhal_power_ctrl(struct uwbhal_data *uwbhal, int onoff)
{
    int rc = 0;
    struct regulator *regulator_vdd_1p8;

    pr_info("%s - onoff : %d, state : %d\n",
            __func__, onoff, uwbhal->state);

    if (onoff == UWBHAL_STATE_POWER_ON) {
        if (uwbhal->state != 0) {
            pr_info("%s - duplicate regulator : %d\n",
                    __func__, onoff);

            uwbhal->state++;
            return 0;
        }
        uwbhal->state++;
    } else {
        if (uwbhal->state == 0) {
            pr_info("%s - already off the regulator : %d\n",
                    __func__, onoff);
            return 0;
        } else if (uwbhal->state != 1) {
            pr_info("%s - duplicate regulator : %d\n",
                    __func__, onoff);
            uwbhal->state--;
            return 0;
        }
        uwbhal->state--;
    }

    regulator_vdd_1p8 =
        regulator_get(&uwbhal->spi->dev, "uwbhal_1p8");

    if (IS_ERR(regulator_vdd_1p8) || regulator_vdd_1p8 == NULL) {
        pr_err("%s - vdd_1p8 regulator_get fail\n", __func__);
        rc = PTR_ERR(regulator_vdd_1p8);
        regulator_vdd_1p8 = NULL;
        goto get_vdd_1p8_failed;
    }

    pr_info("%s - onoff = %d\n", __func__, onoff);
    if (onoff == UWBHAL_STATE_POWER_ON) {
        rc = regulator_enable(regulator_vdd_1p8);
        if (rc) {
            pr_err("%s - enable vdd_1p8 failed, rc=%d\n",
                   __func__, rc);
            goto enable_vdd_1p8_failed;
        }
        usleep_range(1000, 1100);
    } else {
        rc = regulator_disable(regulator_vdd_1p8);
        if (rc) {
            pr_err("%s - disable vdd_1p8 failed, rc=%d\n",
                   __func__, rc);
            goto done;
        }
    }

    goto done;

enable_vdd_1p8_failed:
done:
    regulator_put(regulator_vdd_1p8);

get_vdd_1p8_failed:
    return rc;
}
EXPORT_SYMBOL(uwbhal_power_ctrl);

static int uwbhal_parse_dt(struct uwbhal_data *uwbhal, struct device *dev)
{
    int cs1, rc;
    struct device_node *node = dev->of_node;
    enum of_gpio_flags flags;
    u32 reg;

    /* Read our index / reg from DT */
    rc = of_property_read_u32_index(node, "reg", 0, &reg);
    uwbhal->index = reg;

    uwbhal->irq_gpio = of_get_named_gpio_flags(node, "uwbhal,irq-gpio", 0, &flags);
    slog("[%d] IRQgpio num=%d, flags=%x\n", uwbhal->index, uwbhal->irq_gpio, flags);
    if (!gpio_is_valid(uwbhal->irq_gpio)) {
        slog("Failed to get gpio for irq\n");
        return -EINVAL;
    }
    /* This GPIO is used in critical paths. */
    WARN_ON(gpio_cansleep(uwbhal->irq_gpio));

    uwbhal->reset_gpio = of_get_named_gpio_flags(node, "uwbhal,reset-gpio", 0, &flags);
    uwbhal->reset_flags = flags;
    slog("[%d] Reset num=%d, flags=%lx\n", uwbhal->index, uwbhal->reset_gpio, uwbhal->reset_flags);
    if (!gpio_is_valid(uwbhal->reset_gpio)) {
        slog("Failed to get reset gpio\n");
        return -EINVAL;
    }
    if (uwbhal->reset_flags == 0) {
        slog("WARNING, DT indicates that the RSTn pin needs to be driven high on this board.\n");
    }

    /* If we're alone but there's another cs we have to disable to not
     * have congestion. */
    cs1 = of_get_named_gpio_flags(node, "uwbhal,cs1", 0, &flags);
    if (gpio_is_valid(cs1)) {
        slog("CS1 num=%d, flags=%x. CS1 driven high to avoid interferance\n",
             cs1, flags);
        rc = gpio_direction_output(cs1, 1);
    }

    return 0;
}

static int uwbhal_reset_setup(struct uwbhal_data *uwbhal)
{
    int ret = 0;
    /* Reset should be open drain, or switched to input
     * whenever not driven low. It should not be driven high. */
    ret = devm_gpio_request_one(uwbhal->dev, uwbhal->reset_gpio,
                                uwbhal->reset_flags,
                                "uwbhal_reset");
    if (ret) {
        slog("Failed to request 'uwbhal_reset'\n");
        return ret;
    }
    return 0;
}

static int uwbhal_irq_setup(struct uwbhal_data *uwbhal)
{
    int ret = 0;

    ret = devm_gpio_request_one(uwbhal->dev, uwbhal->irq_gpio,
                                GPIOF_IN, "uwbhal_int");
    if (ret) {
        slog("Failed to request 'uwbhal_int'\n");
        return ret;
    }

    uwbhal->irq = gpio_to_irq(uwbhal->irq_gpio);
    if (uwbhal->irq < 0) {
        slog("Invalid IRQ numberi: %d\n", uwbhal->irq);
        return uwbhal->irq;
    }

    return ret;
}

static void uwbhal_irq_set_state(struct uwbhal_data *uwbhal, int irq_enable)
{
    if (!uwbhal) {
        slog("null pointer received");
        return;
    }
    slog("irq_enable: %d, irq_state: %d\n",
        irq_enable, uwbhal->irq_state);

    if (irq_enable) {
        if (uwbhal->irq_state)
            return;
        enable_irq(uwbhal->irq);
        uwbhal->irq_state = 1;

        cpuidle_pause_and_lock();
        //pm_qos_add_request(&uwbhal->qos, PM_QOS_CPU_DMA_LATENCY, 0);
        //cpuidle_disable_device(__this_cpu_read(cpuidle_devices));
        cpuidle_resume_and_unlock();
    } else {
        if (uwbhal->irq_state == 0)
            return;
        //pm_qos_remove_request(&uwbhal->qos);
        disable_irq(uwbhal->irq);
        uwbhal->irq_state = 0;
        //cpuidle_resume();
    }
    slog("pm_qos result: %d\n", pm_qos_request(PM_QOS_CPU_DMA_LATENCY));
}

static int uwbhal_open(struct inode *inode, struct file *file)
{
    struct uwbhal_data *uwbhal = NULL;

    uwbhal = container_of(inode->i_cdev, struct uwbhal_data, cdev);
    if (!uwbhal) {
        slog("Failed to get UWBHAL private data\n");
        return -ENODEV;
    }

    file->private_data = uwbhal;
    return 0;
}

static int uwbhal_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static ssize_t uwbhal_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    struct uwbhal_data *uwbhal = file->private_data;
    int rd_bytes;

    if (len > UWBHAL_BUF_LEN) {
        slog("Invalid length, should be < %d\n", UWBHAL_BUF_LEN);
        return -EINVAL;
    }

    rd_bytes = copy_to_user(buf, uwbhal->rx_buf, len);
    if (rd_bytes) {
        slog("Improper copy to user: %d\n", rd_bytes);
        return -EFAULT;
    }

    return len;
}

static ssize_t uwbhal_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    struct uwbhal_data *uwbhal = file->private_data;
    int wr_bytes = 0;
    int rd_bytes = 0;

    if (len >= UWBHAL_BUF_LEN) {
        slog("Invalid length, should be < %d\n", UWBHAL_BUF_LEN);
        return -EINVAL;
    }

    memset(uwbhal->rx_buf, 0, len);
    wr_bytes = copy_from_user(uwbhal->tx_buf, buf, len);
    if (wr_bytes) {
        slog("Improper copy from user: %d\n", wr_bytes);
        return -EFAULT;
    }

    wr_bytes = len - wr_bytes;
    rd_bytes = uwbhal_spi_transfer(uwbhal, uwbhal->tx_buf, uwbhal->rx_buf, len);
    if (rd_bytes)
        return rd_bytes;

    return len;
}

static const struct file_operations uwbhal_fops = {
    .owner   = THIS_MODULE,
    .open    = uwbhal_open,
    .release = uwbhal_release,
    .read    = uwbhal_read,
    .write   = uwbhal_write,
};

static char *uwbhal_devnode(struct device *dev, umode_t *mode)
{
    slog("");
    if (!mode)
        return NULL;

    *mode = 0666;

    return NULL;
}

static int uwbhal_chrdev_create(struct uwbhal_data *uwbhal)
{
    int ret = 0;
    struct device *device;

    ret = alloc_chrdev_region(&uwbhal->devt, 0, 1, uwbhal->name);
    if (ret) {
        slog("Failed to allocate char dev region\n");
        return ret;
    }

    cdev_init(&uwbhal->cdev, &uwbhal_fops);
    uwbhal->cdev.owner = THIS_MODULE;

    /* Add the device */
    ret = cdev_add(&uwbhal->cdev, uwbhal->devt, 1);
    if (ret < 0) {
        slog("Cannot register a device,  err: %d", ret);
        goto err_cdev;
    }

    uwbhal->class = class_create(THIS_MODULE, uwbhal->name);
    if (IS_ERR(uwbhal->class)) {
        ret = PTR_ERR(uwbhal->class);
        slog("Unable to create uwbhal class, err: %d\n", ret);
        goto err_class;
    }

    uwbhal->class->devnode = uwbhal_devnode;
    device = device_create(uwbhal->class, uwbhal->dev, uwbhal->devt, NULL, uwbhal->name);
    if (IS_ERR(device)) {
        ret = PTR_ERR(device);
        slog("Unable to create uwbhal device, err: %d\n", ret);
        goto err_device;
    }

    return 0;

 err_device:
    class_destroy(uwbhal->class);
 err_class:
    cdev_del(&uwbhal->cdev);
 err_cdev:
    unregister_chrdev_region(uwbhal->devt, 1);

    return ret;
}

static void uwbhal_chrdev_destroy(struct uwbhal_data *uwbhal)
{
    slog("");
    device_destroy(uwbhal->class, uwbhal->devt);
    class_destroy(uwbhal->class);
    cdev_del(&uwbhal->cdev);
    unregister_chrdev_region(uwbhal->devt, 1);
}

int uwbhal_enable(struct uwbhal_data *uwbhal)
{
    int ret = 0;
    if (!uwbhal) {
        slog("null pointer received");
        return -EINVAL;
    }

    /* Return early if we're already been enabled */
    if (atomic_read(&uwbhal->is_enable) == 1) {
        return ret;
    }

    ret = uwbhal_power_ctrl(uwbhal, UWBHAL_STATE_POWER_ON);
    if (ret)
        return ret;

    uwbhal_irq_set_state(uwbhal, UWBHAL_IRQ_STATE_EN);
    atomic_set(&uwbhal->is_enable, 1);

    slog("uwbhal is enabled\n");
    return ret;
}
EXPORT_SYMBOL(uwbhal_enable);

void uwbhal_disable(struct uwbhal_data *uwbhal)
{
    /* Return early if we're already been enabled */
    if (atomic_read(&uwbhal->is_enable) == 0) {
        return;
    }

    uwbhal_irq_set_state(uwbhal, UWBHAL_IRQ_STATE_DIS);
    atomic_set(&uwbhal->is_enable, 0);

    uwbhal_power_ctrl(uwbhal, UWBHAL_STATE_POWER_OFF);

    slog("uwbhal is disabled\n");
}
EXPORT_SYMBOL(uwbhal_disable);

static void uwbhal_init_data(struct uwbhal_data *uwbhal)
{
    uwbhal->spi = NULL;
    uwbhal->dev = NULL;
    uwbhal->irq_state = 0;
    uwbhal->state = 0;
}

static ssize_t uwbhal_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);

    return snprintf(buf, PAGE_SIZE, "%d\n",
        (atomic_read(&uwbhal->is_enable) == 1 ? 1 : 0));
}

/* TODO */
static ssize_t uwbhal_enable_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);
    int new_value;

    if (sysfs_streq(buf, "1"))
        new_value = 1;
    else if (sysfs_streq(buf, "0"))
        new_value = 0;
    else {
        slog("%s - invalid value %d\n", __func__, *buf);
        return -EINVAL;
    }

    slog("Enable : %d - %s\n", new_value, __func__);

    mutex_lock(&uwbhal->activelock);
    if (new_value)
        uwbhal_enable(uwbhal);
    else if (!new_value)
        uwbhal_disable(uwbhal);
    mutex_unlock(&uwbhal->activelock);
    return count;
}

int uwbhal_gpio_read(struct uwbhal_data *uwbhal, int gpio)
{
    if (!uwbhal) {
        slog("null pointer received");
        return -EINVAL;
    }
    switch (gpio) {
    case (UWBHAL_GPIO_RST):
        if (gpio_cansleep(uwbhal->reset_gpio))
            return gpio_get_value_cansleep(uwbhal->reset_gpio);
        else
            return gpio_get_value(uwbhal->reset_gpio);
    case (UWBHAL_GPIO_EN):   return atomic_read(&uwbhal->is_enable);
    case (UWBHAL_GPIO_IRQ):  return gpio_get_value(uwbhal->irq_gpio);
    case (UWBHAL_GPIO_WAKE): return -1;
    default: return -1;
    }
    return -1;
}
EXPORT_SYMBOL(uwbhal_gpio_read);

int uwbhal_gpio_out(struct uwbhal_data *uwbhal, int gpio, int val)
{
    int _gpio = -1, rc;
    if (!uwbhal) {
        slog("null pointer received");
        return -EINVAL;
    }
    switch (gpio) {
    case (UWBHAL_GPIO_RST):  _gpio = uwbhal->reset_gpio;break;
    case (UWBHAL_GPIO_EN):   return -EINVAL;
    case (UWBHAL_GPIO_IRQ):  return -EINVAL;
    case (UWBHAL_GPIO_WAKE): return -EINVAL;
    default: break;
    }
    if (_gpio < 0) {
        return -EINVAL;
    }

    rc = gpio_direction_output(_gpio, val);
    if (rc < 0) {
        slog("setting direction failed\n");
        return rc;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(uwbhal_gpio_out);

int uwbhal_gpio_in(struct uwbhal_data *uwbhal, int gpio)
{
    int _gpio = -1, rc;
    if (!uwbhal) {
        slog("null pointer received");
        return -EINVAL;
    }
    switch (gpio) {
    case (UWBHAL_GPIO_RST):
        _gpio = uwbhal->reset_gpio;
        if (uwbhal->reset_flags == 0) {
            /* There's a buffer on the reset-pin, keep driving it */
            gpio_direction_output(_gpio, 1);
            return 0;
        }
        break;
    case (UWBHAL_GPIO_EN):   return -EINVAL;
    case (UWBHAL_GPIO_IRQ):  return -EINVAL;
    case (UWBHAL_GPIO_WAKE): return -EINVAL;
    default: break;
    }
    if (_gpio < 0) {
        return -EINVAL;
    }

    rc = gpio_direction_input(_gpio);
    if (rc < 0) {
        slog("setting direction failed\n");
        return rc;
    }

    return 0;
}
EXPORT_SYMBOL(uwbhal_gpio_in);

void uwbhal_gpio_write(struct uwbhal_data *uwbhal, int gpio, int value)
{
    if (!uwbhal) {
        slog("null pointer received");
        return;
    }
    mutex_lock(&uwbhal->activelock);
    switch (gpio) {
    case (UWBHAL_GPIO_RST): {
        if (gpio_cansleep(uwbhal->reset_gpio))
            gpio_set_value_cansleep(uwbhal->reset_gpio, value);
        else
            gpio_set_value(uwbhal->reset_gpio, value);
        break;
    }
    case (UWBHAL_GPIO_EN): {
        if (value) {
            uwbhal_enable(uwbhal);
        } else {
            uwbhal_disable(uwbhal);
        }
        break;
    }
    case (UWBHAL_GPIO_IRQ): {
        gpio_set_value(uwbhal->irq_gpio, value);
        break;
    }
    case (UWBHAL_GPIO_WAKE): break;
    default: break;
    }
    mutex_unlock(&uwbhal->activelock);
    return;
}
EXPORT_SYMBOL(uwbhal_gpio_write);

static ssize_t uwbhal_reset_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);
    int value;

    if (gpio_cansleep(uwbhal->reset_gpio))
        value = gpio_get_value_cansleep(uwbhal->reset_gpio);
    else
        value = gpio_get_value(uwbhal->reset_gpio);

    return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t uwbhal_reset_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc;
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);
    int new_value;

    /* Reset should not be driven high for DW*000. It should
     * really be open drain, or swtiched to input wheneven not
     * driven low. As all hw doesn't have open-drain we set it to input
     * instead of driving it high. */

    if (sysfs_streq(buf, "1"))
        new_value = 1;
    else if (sysfs_streq(buf, "0"))
        new_value = 0;
    else {
        slog("%s - invalid value %d\n", __func__, *buf);
        return -EINVAL;
    }

    if (new_value) {
        mutex_lock(&uwbhal->activelock);
        if (gpio_cansleep(uwbhal->reset_gpio))
            gpio_set_value_cansleep(uwbhal->reset_gpio, 1);
        else
            gpio_set_value(uwbhal->reset_gpio, 1);
        rc = gpio_direction_input(uwbhal->reset_gpio);
        mutex_unlock(&uwbhal->activelock);
        if (rc < 0) {
            slog("setting direction failed\n");
            return rc;
        }
    } else {
        mutex_lock(&uwbhal->activelock);
        rc = gpio_direction_output(uwbhal->reset_gpio, 0);
        mutex_unlock(&uwbhal->activelock);
        if (rc < 0) {
            slog("setting direction failed\n");
            return rc;
        }
    }
    return count;
}

static ssize_t uwbhal_irq_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);

    return snprintf(buf, PAGE_SIZE, "%d\n",
        gpio_get_value(uwbhal->irq_gpio));
}

static ssize_t uwbhal_spi_mode_get(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);

    return sprintf(buf, "0x%x\n", uwbhal->spi->mode);

}

static ssize_t uwbhal_spi_mode_set(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);
    uint32_t mode = 0;
    int ret = 0;
    struct spi_device *spi;

    sscanf(buf, "%x", &mode);
    mode &= ((1<<16) - 1);
    spi = spi_dev_get(uwbhal->spi);
    uwbhal->spi->mode = mode;
    ret = spi_setup(spi);
    spi_dev_put(spi);
    if (ret < 0)
        return ret;
    return count;

}
static ssize_t uwbhal_spi_freq_get(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);

    return sprintf(buf, "%u\n", uwbhal->spi->max_speed_hz);

}

int uwbhal_spi_freq(struct uwbhal_data *uwbhal, int freq)
{
    uwbhal->spi->max_speed_hz = freq;
    return spi_setup(uwbhal->spi);
}
EXPORT_SYMBOL(uwbhal_spi_freq);

static ssize_t uwbhal_spi_freq_set(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);
    uint32_t freq = 0;
    int ret = 0;
    sscanf(buf, "%u", &freq);
    ret = uwbhal_spi_freq(uwbhal, freq);
    if (ret < 0)
        return ret;

    return count;
}
static ssize_t uwbhal_spi_bpw_get(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);

    return sprintf(buf, "%u\n", uwbhal->spi->bits_per_word);

}
static ssize_t uwbhal_spi_bpw_set(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct uwbhal_data *uwbhal = dev_get_drvdata(dev);
    unsigned int bits_per_word = 0;
    int ret = 0;

    slog("In SPI_BPW:Before sscanf BPW: %s&\n",buf);
    sscanf(buf, "%u", &bits_per_word);
    slog("In SPI_BPW:AFter sscanf BPW: %s&\n",buf);
    uwbhal->spi->bits_per_word = bits_per_word;
    slog("In SPI_BPW:Before setup BPW: %s&\n",buf);
    ret = spi_setup(uwbhal->spi);
    slog("In SPI_BPW:AFter setup BPW: %s&\n",buf);
    if (ret < 0)
        return ret;
    return count;
}

static struct device_attribute dev_attr_uwbhal_enable =
    __ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
    uwbhal_enable_show, uwbhal_enable_store);

static struct device_attribute dev_attr_uwbhal_reset =
    __ATTR(reset, S_IRUGO|S_IWUSR|S_IWGRP,
    uwbhal_reset_show, uwbhal_reset_store);

static struct device_attribute dev_attr_uwbhal_irq =
    __ATTR(irq, S_IRUGO,
    uwbhal_irq_show, 0);

static struct device_attribute dev_attr_uwbhal_spi_mode =
    __ATTR(spi_mode, S_IRUGO|S_IWUSR|S_IWGRP,
    uwbhal_spi_mode_get, uwbhal_spi_mode_set);

static struct device_attribute dev_attr_uwbhal_spi_freq =
    __ATTR(spi_freq, S_IRUGO|S_IWUSR|S_IWGRP,
    uwbhal_spi_freq_get, uwbhal_spi_freq_set);

static struct device_attribute dev_attr_uwbhal_spi_bpw =
    __ATTR(spi_bpw, S_IRUGO|S_IWUSR|S_IWGRP,
    uwbhal_spi_bpw_get, uwbhal_spi_bpw_set);

static struct attribute *uwbhal_sysfs_attrs[] = {
    &dev_attr_uwbhal_enable.attr,
    &dev_attr_uwbhal_reset.attr,
    &dev_attr_uwbhal_irq.attr,
    &dev_attr_uwbhal_spi_mode.attr,
    &dev_attr_uwbhal_spi_freq.attr,
    &dev_attr_uwbhal_spi_bpw.attr,
    NULL
};

static struct attribute_group uwbhal_attribute_group = {
    .attrs = uwbhal_sysfs_attrs,
};


static int uwbhal_spi_probe(struct spi_device *spi)
{
    struct uwbhal_data *uwbhal = NULL;
    struct device *dev = &spi->dev;
    int ret = 0;

    slog("\n");

    /* Setup SPI bus */
    spi->bits_per_word = 8;
    ret = spi_setup(spi);
    if (ret)
        return ret;

    if (!dev->of_node)
        return -ENODEV;

    slog("setup mode: %d, %u bits/w, %u Hz max\n",
        (int) (spi->mode & (SPI_CPOL | SPI_CPHA)),
        spi->bits_per_word, spi->max_speed_hz);

    /* Allocate memory for UWBHAL data */
    uwbhal = devm_kzalloc(dev, sizeof(*uwbhal), GFP_KERNEL);
    if (uwbhal == NULL) {
        slog("Failed to allocate memory\n");
        return -ENOMEM;
    }
    uwbhal->tx_buf = devm_kzalloc(dev, UWBHAL_BUF_LEN, GFP_DMA | GFP_KERNEL);
    if (uwbhal->tx_buf == NULL) {
        slog("Failed to allocate tx-dma memory\n");
        return -ENOMEM;
    }
    uwbhal->rx_buf = devm_kzalloc(dev, UWBHAL_BUF_LEN, GFP_DMA | GFP_KERNEL);
    if (uwbhal->rx_buf == NULL) {
        slog("Failed to allocate rx-dma memory\n");
        return -ENOMEM;
    }

    /* Init uwbhal_data object */
    uwbhal_init_data(uwbhal);
    uwbhal->spi = spi;
    uwbhal->dev = dev;
    spi_set_drvdata(spi, uwbhal);

    mutex_init(&uwbhal->spilock);
    mutex_init(&uwbhal->activelock);

    ret = uwbhal_parse_dt(uwbhal, &spi->dev);
    if (ret < 0)
        goto err;

    snprintf(uwbhal->name, sizeof(uwbhal->name),
             "%s%d", UWBHAL_NAME, uwbhal->index);

    g_uwbhal_instances_n++;
    g_uwbhal_instances[uwbhal->index] = uwbhal;

    ret = uwbhal_irq_setup(uwbhal);
    if (ret)
        goto err;

    ret = uwbhal_reset_setup(uwbhal);
    if (ret)
        goto err;

    ret = uwbhal_power_ctrl(uwbhal, UWBHAL_STATE_POWER_ON);
    if (ret)
        goto err;

    usleep_range(1000, 1100);

    ret = uwbhal_power_ctrl(uwbhal, UWBHAL_STATE_POWER_OFF);
    if (ret)
        goto err;

    /* Sysfs */
    ret = sysfs_create_group(&uwbhal->dev->kobj, &uwbhal_attribute_group);
    if (ret) {
        slog("Failed to create sysfs group\n");
        goto err_sysfs;
    }
    ret = uwbhal_chrdev_create(uwbhal);
    if (ret)
        goto err_sysfs;

    atomic_set(&uwbhal->is_enable, 0);

    /* UIO interface still used by userspace app, may be
     * removed in future if the userspace access is no longer needed. */
    uwbhal->uio = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (!uwbhal->uio)
        goto err_sysfs;

    uwbhal->uio->name = uwbhal->name;
    uwbhal->uio->priv = uwbhal;
    uwbhal->uio->version = "0.1.0";
    uwbhal->uio->irq = uwbhal->irq;
    uwbhal->uio->irq_flags = IRQF_SHARED|IRQF_TRIGGER_RISING;
    uwbhal->uio->priv = (void*)uwbhal;
    uwbhal->uio->handler = uwbhal_irq_handler;

    ret = uio_register_device(dev, uwbhal->uio);
    if (ret)
        goto err_uio;
    disable_irq(uwbhal->irq);
    uwbhal->initialized = true;
    slog("Probe successful\n");
    return 0;

err_uio:
    uio_unregister_device(uwbhal->uio);
    kfree(uwbhal->uio);
err_sysfs:
err:
    mutex_destroy(&uwbhal->spilock);
    mutex_destroy(&uwbhal->activelock);
    slog("Probe failed!!!\n");
    return ret;
}

static int uwbhal_spi_remove(struct spi_device *spi)
{
    struct uwbhal_data *uwbhal = spi_get_drvdata(spi);
    slog();
    disable_irq(uwbhal->irq);
    uio_unregister_device(uwbhal->uio);
    kfree(uwbhal->uio);

    sysfs_remove_group(&uwbhal->dev->kobj,
                       &uwbhal_attribute_group);
    mutex_destroy(&uwbhal->spilock);
    mutex_destroy(&uwbhal->activelock);
    uwbhal_chrdev_destroy(uwbhal);
    /* Do not kfree uwbhal, tx_buf, and rx_buf, they have been
     * allocated with the managed
     * function devm_kzalloc and should be automatically freed. */
    return 0;
}

static struct of_device_id uwbhal_of_device_ids[] = {
    { .compatible = "decawave,uwbhal", },
    { },
};

static const struct spi_device_id uwbhal_spi_ids_table[] = {
    {"uwbhal", 0},
    { }
};

static struct spi_driver uwb_spi_driver = {
    .driver = {
        .name = UWBHAL_NAME,
        .owner = THIS_MODULE,
        .of_match_table = uwbhal_of_device_ids,
    },
    .probe		= uwbhal_spi_probe,
    .remove		= uwbhal_spi_remove,
    .id_table	= uwbhal_spi_ids_table,
};
module_spi_driver(uwb_spi_driver);
