#include <dpl/dpl.h>
#include <os/os_dev.h>
#if MYNEWT_VAL(DW1000_DEVICE_0)
#include <linux/kernel.h>
#include <dw1000/dw1000_dev.h>
#include <dw1000/dw1000_hal.h>

static struct dw1000_dev_cfg dw1000_cfg[] = {
    {
    .spi_sem = 0,
    .spi_baudrate = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_HIGH),
    .spi_baudrate_low = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_LOW),
    .spi_num = MYNEWT_VAL(DW1000_DEVICE_0_SPI_IDX),
    .rst_pin  = MYNEWT_VAL(DW1000_DEVICE_0_RST),
    .irq_pin  = MYNEWT_VAL(DW1000_DEVICE_0_IRQ),
    .ss_pin = MYNEWT_VAL(DW1000_DEVICE_0_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_0_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_0_TX_ANT_DLY),
    .ext_clock_delay = 0
    },
    {
    .spi_sem = 0,
    .spi_baudrate = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_HIGH),
    .spi_baudrate_low = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_LOW),
    .spi_num = MYNEWT_VAL(DW1000_DEVICE_1_SPI_IDX),
    .rst_pin  = MYNEWT_VAL(DW1000_DEVICE_1_RST),
    .irq_pin  = MYNEWT_VAL(DW1000_DEVICE_1_IRQ),
    .ss_pin = MYNEWT_VAL(DW1000_DEVICE_1_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_1_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_1_TX_ANT_DLY),
    .ext_clock_delay = 0
    },
    {0}
};

static char* dw1000_dev_names[] = {
    "dw1000_0",
    "dw1000_1",
    "dw1000_2"
};

int uwbcore_hal_bsp_dw1000_init(int spi_num, uint32_t devid, struct dpl_sem *sem)
{
    int rc, idx;
    dw1000_dev_instance_t * dw1000_inst;
    if (devid != DWT_DEVICE_ID) {
        return 0;
    }
    dw1000_inst = hal_dw1000_inst(spi_num);
    if (!dw1000_inst) {
        return 0;
    }

    for (idx=0;idx<ARRAY_SIZE(dw1000_cfg);idx++) {
        if (dw1000_cfg[idx].spi_num == spi_num) {
            break;
        }
    }
    if (idx == ARRAY_SIZE(dw1000_cfg)) {
        pr_info("%s:%dno spi_num that matches", __func__, __LINE__);
        return 0;
    }
    dw1000_cfg[idx].spi_sem = sem;
    rc = os_dev_create((struct os_dev *) dw1000_inst, dw1000_dev_names[idx],
            OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_cfg[idx]);
    assert(rc == 0);
    printk("uwbcore: %s detected on %d\n", dw1000_dev_names[idx], spi_num);
    return 1;
}

#endif
