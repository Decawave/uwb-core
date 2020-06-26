#include <linux/delay.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>
#include <linux/debugfs.h>

#include <os/os_dev.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <hal/hal_timer.h>

#include <uwb/uwb.h>
#include <uwb_rng/uwb_rng.h>
#include <uwb_ccp/uwb_ccp.h>
#include <uwbcfg/uwbcfg.h>
#include <linux/uwb-hal.h>
#include "uwbcore_priv.h"

#define slog(fmt, ...) \
    pr_info("uwbcore:%s():%d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

MODULE_AUTHOR("UWBCore Maintainer <uwbcore@gmail.com>");
MODULE_DESCRIPTION("UWB Core Driver");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_ALIAS("spi:uwbcore");

#define UWBCORE_MAXNUM_DEVS (3)

static struct dpl_sem g_spi0_sem;
static struct os_dev* g_os_devs[UWBCORE_MAXNUM_DEVS] = {0};
static struct kobject *uwbcore_kobj;
static struct dentry *uwbcore_dentry;

static int
os_dev_init(struct os_dev *dev, char *name, uint8_t stage,
        uint8_t priority, os_dev_init_func_t od_init, void *arg)
{
    dev->od_name = name;
    dev->od_stage = stage;
    dev->od_priority = priority;
    /* assume these are set after the fact. */
    dev->od_flags = 0;
    dev->od_open_ref = 0;
    dev->od_init = od_init;
    dev->od_init_arg = arg;
    memset(&dev->od_handlers, 0, sizeof(dev->od_handlers));

    return (0);
}

int os_dev_create(struct os_dev *dev, char *name, uint8_t stage,
                  uint8_t priority, os_dev_init_func_t od_init, void *arg)
{
    int rc, i;

    rc = os_dev_init(dev, name, stage, priority, od_init, arg);
    if (rc != 0) {
        goto err;
    }
    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        if (g_os_devs[i] == 0) {
            g_os_devs[i] = dev;
            break;
        }
    }

    if (i==UWBCORE_MAXNUM_DEVS) {
        rc = ENOMEM;
        goto err;
    }

    rc = dev->od_init(dev, dev->od_init_arg);
    if (rc == 0) {
        dev->od_flags |= OS_DEV_F_STATUS_READY;
    }
err:
    return rc;
}

struct os_dev* os_dev_lookup(char* name)
{
    int i;
    struct os_dev *dev = NULL;

    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        dev = g_os_devs[i];
        if (!dev) continue;
        if (!strcmp(dev->od_name, name)) {
            break;
        }
    }
    return (dev);
}

static int
_enable_and_reset(int en_pin, int rst_pin)
{
    int rc = 0;
    hal_gpio_write(en_pin, 1);
    usleep_range(10000,12000);
    rc |= hal_gpio_init_out(rst_pin, 0);
    usleep_range(10,100);
    hal_gpio_write(rst_pin, 1);
    rc |= hal_gpio_init_in(rst_pin, 0);
    return rc;
}

static void
enable_and_reset(int idx)
{
    switch (idx) {
        case (0): {
#if MYNEWT_VAL(DW1000_DEVICE_0)
            _enable_and_reset(MYNEWT_VAL(DW1000_DEVICE_0_EN), MYNEWT_VAL(DW1000_DEVICE_0_RST));
#endif
#if MYNEWT_VAL(DW3000_DEVICE_0)
            _enable_and_reset(MYNEWT_VAL(DW3000_DEVICE_0_EN), MYNEWT_VAL(DW3000_DEVICE_0_RST));
#endif
            break;
        }
        case (1): {
#if MYNEWT_VAL(DW1000_DEVICE_1)
            _enable_and_reset(MYNEWT_VAL(DW1000_DEVICE_1_EN), MYNEWT_VAL(DW1000_DEVICE_1_RST));
#endif
#if MYNEWT_VAL(DW3000_DEVICE_1)
            _enable_and_reset(MYNEWT_VAL(DW3000_DEVICE_1_EN), MYNEWT_VAL(DW3000_DEVICE_1_RST));
#endif
            break;
        }
        case (2): {
#if MYNEWT_VAL(DW1000_DEVICE_2)
            _enable_and_reset(MYNEWT_VAL(DW1000_DEVICE_2_EN), MYNEWT_VAL(DW1000_DEVICE_2_RST));
#endif
#if MYNEWT_VAL(DW3000_DEVICE_2)
            _enable_and_reset(MYNEWT_VAL(DW3000_DEVICE_2_EN), MYNEWT_VAL(DW3000_DEVICE_2_RST));
#endif
            break;
        }
    }
}

static int
hal_bsp_init(void)
{
    int rc, num_devices, i;
    uint32_t devid = 0;
    uint8_t tx[5] = {0};
    uint8_t rx[5] = {0};
    struct hal_spi_settings spi_c = {
        .data_order = HAL_SPI_MSB_FIRST,
        .data_mode = HAL_SPI_MODE0,
        .baudrate = 1000,
        .word_size = HAL_SPI_WORD_SIZE_8BIT
    };

    rc = dpl_sem_init(&g_spi0_sem, 0x1);
    assert(rc == 0);

    /* Enable and reset all available instances */
    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        if (IS_ERR_OR_NULL(uwbhal_get_instance(i))) {
            continue;
        }
        enable_and_reset(i);
    }

    num_devices = 0;
    /* Probe for devices on the instances */
    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        if (IS_ERR_OR_NULL(uwbhal_get_instance(i))) {
            continue;
        }

        rc = hal_spi_init(i, (void *)0, HAL_SPI_TYPE_MASTER);
        assert(rc == 0);
        rc = hal_spi_config(i, &spi_c);

        usleep_range(10000,20000);
        rc = hal_spi_txrx(i, tx, rx, 5);
        if (!rc) {
            devid = rx[1] | (uint32_t)rx[2]<<8 | (uint32_t)rx[3]<<16 | (uint32_t)rx[4]<<24;
        }
        slog("probe device[%d]: %d, devid: %X\n", i, rc, devid);

#if MYNEWT_VAL(DW1000_DEVICE_0) || MYNEWT_VAL(DW1000_DEVICE_1)
        num_devices += uwbcore_hal_bsp_dw1000_init(i, devid, &g_spi0_sem);
#endif
#if MYNEWT_VAL(DW3000_DEVICE_0) || MYNEWT_VAL(DW3000_DEVICE_1)
        num_devices += uwbcore_hal_bsp_dw3000_init(i, devid, &g_spi0_sem);
#endif
    }

#if MYNEWT_VAL(OS_CPUTIME_TIMER_NUM >= 0)
    rc = hal_timer_init(0, NULL);
    assert(rc == 0);
    rc = dpl_cputime_init(MYNEWT_VAL(OS_CPUTIME_FREQ));
    assert(rc == 0);
#endif
    if (num_devices==0) {
        slog("ERROR: No devices detected\n");
        return -1;
    }
    slog("done\n");
    return 0;
}

#if 1
void __attribute__((weak)) flash_map_init(void) { /* Dummy */ }
void __attribute__((weak)) log_init(void) { /* Dummy */ }
void __attribute__((weak)) mfg_init(void) { /* Dummy */ }
void __attribute__((weak)) modlog_init(void) { /* Dummy */ }
void __attribute__((weak)) uwb_pan_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) nmgr_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) nmgr_uwb_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) twr_ss_nrng_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) nrng_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) survey_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) uwb_ccp_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) tdma_pkg_init(void) { /* Dummy */ }

void __attribute__((weak)) nffs_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) tu_init(void) { /* Dummy */ }

void __attribute__((weak)) os_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) shell_init(void) { /* Dummy */ }
void __attribute__((weak)) console_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) config_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) config_pkg_init_stage2(void) { /* Dummy */ }
struct shell_cmd;
int shell_cmd_register(struct shell_cmd *cmd) {return 0;}
#endif

void sysinit_app(void);
extern int (* const sysdown_cbs[])(int reason);

static void hal_bsp_deinit(void)
{
    int i=0;

    /* TODO: don't have control over stats in apache-mynewt
     * so manually calling down function here */
    int stats_module_down(int reason);
    stats_module_down(0);

    /* Find end and start from there */
    while (sysdown_cbs[i]) i++;
    i--;
    while (sysdown_cbs[i]) {
        slog("sysdown(%d)\n", i);
        if (sysdown_cbs[i]) {
            sysdown_cbs[i](0);
        }
        if (i==0) break;
        i--;
    }

#if MYNEWT_VAL(DW1000_DEVICE_0)
    hal_gpio_write(MYNEWT_VAL(DW1000_DEVICE_0_EN), 0);
#endif
#if MYNEWT_VAL(DW3000_DEVICE_0)
    hal_gpio_write(MYNEWT_VAL(DW3000_DEVICE_0_EN), 0);
#endif

#if MYNEWT_VAL(OS_CPUTIME_TIMER_NUM >= 0)
    hal_timer_deinit(0);
#endif
    slog("done\n");
}

struct kobject* uwbcore_get_kobj(void)
{
    return uwbcore_kobj;
}

struct dentry* uwbcore_get_dfs(void)
{
    return uwbcore_dentry;
}

static int
uwb_config_updated(void)
{
    int i;
    struct uwb_dev *inst;

    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        inst = uwb_dev_idx_lookup(i);
        if (IS_ERR_OR_NULL(inst)) {
            continue;
        }

        uwb_mac_config(inst, NULL);
        uwb_txrf_config(inst, &inst->config.txrf);
        slog("config updated for instance %d\n", i);
    }
    return 0;
}

static struct uwbcfg_cbs uwb_cb = {
    .uc_update = uwb_config_updated
};



static int __init uwbcore_init(void)
{
    int rc, i;
    int hal_timeout;
    struct uwb_dev *udev;
    slog("");
    uwbcore_kobj = kobject_create_and_add("uwbcore", kernel_kobj);
    if (!uwbcore_kobj) {
        slog("Failed to create uwbcore_kobj\n");
        return -ENOMEM;
    }

    uwbcore_dentry = debugfs_create_dir("uwbcore", NULL);
    if (!uwbcore_dentry) {
        slog("Failed to create debugfs entry\n");
    }

    /* Wait until uwb-hal is initialized */
    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        if (IS_ERR(uwbhal_get_instance(i))) {
            continue;
        }
        /* Workaround for uwb-hal not being intialized when
         * uwbcore loads too quickly. TODO: find another way to know
         * the number of instances? */
        hal_timeout = 5;
        while (!uwbhal_get_instance(i) && --hal_timeout) {
            slog("Waiting for uwb-hal null");
            usleep_range(100000, 150000);
        }
        if (!uwbhal_get_instance(i)) {
            continue;
        }
        while (!uwbhal_get_instance(i)->initialized && --hal_timeout) {
            slog("Waiting for uwb-hal init");
            usleep_range(100000, 150000);
        }
    }

    rc = hal_bsp_init();
    if (rc) {
        goto err;
    }

    sysinit_app();

    /* Stats pkg.yml is out of our control so run it's
     * second stage manually. This sets up the stats-sysfs */
    {
        void stats_module_init_stage2(void);
        stats_module_init_stage2();
    }

    /* Register for cfg updates */
    uwbcfg_register(&uwb_cb);
    uwb_config_updated();

    for (i=0;i<UWBCORE_MAXNUM_DEVS;i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) continue;

        uwb_set_autoack(udev, true);
        uwb_set_autoack_delay(udev, 12);

        slog("[%d]device_id=%X,panid=%X,addr=%X,part_id=%X,lot_id=%X\n",
             i, udev->device_id, udev->pan_id, udev->uid,
             (uint32_t)(udev->euid&0xffffffff),
             (uint32_t)(udev->euid>>32));
    }
err:
    return 0;
}

static void __exit uwbcore_exit(void)
{
    slog("");

#if MYNEWT_VAL(DW1000_DEVICE_0)
    hal_gpio_write(MYNEWT_VAL(DW1000_DEVICE_0_EN), 0);
#endif
#if MYNEWT_VAL(DW3000_DEVICE_0)
    hal_gpio_write(MYNEWT_VAL(DW3000_DEVICE_0_EN), 0);
#endif
    /* TODO: Free structures and stop workqueues */
#ifdef UWBCORE_TEST_APP
    dpl_callout_stop(&tx_callout);
#endif
    hal_bsp_deinit();
    kobject_put(uwbcore_kobj);
    debugfs_remove_recursive(uwbcore_dentry);
}

module_init(uwbcore_init);
module_exit(uwbcore_exit);
