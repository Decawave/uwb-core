#include <dpl/dpl.h>
#include <os/os_dev.h>
#if MYNEWT_VAL(DW3000_DEVICE_0)
#include <dw3000-c0/dw3000_dev.h>
#include <dw3000-c0/dw3000_hal.h>

static struct dw3000_dev_cfg dw3000_cfg[] = {
    {
    .spi_sem = 0,
    .spi_baudrate = MYNEWT_VAL(DW3000_DEVICE_BAUDRATE),
    .spi_num = MYNEWT_VAL(DW3000_DEVICE_0_SPI_IDX),
    .rst_pin  = MYNEWT_VAL(DW3000_DEVICE_0_RST),
    .irq_pin  = MYNEWT_VAL(DW3000_DEVICE_0_IRQ),
    .ss_pin = MYNEWT_VAL(DW3000_DEVICE_0_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_0_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_0_TX_ANT_DLY),
    .ext_clock_delay = 0
    },
    {
    .spi_sem = 0,
    .spi_baudrate = MYNEWT_VAL(DW3000_DEVICE_BAUDRATE),
    .spi_num = MYNEWT_VAL(DW3000_DEVICE_1_SPI_IDX),
    .rst_pin  = MYNEWT_VAL(DW3000_DEVICE_1_RST),
    .irq_pin  = MYNEWT_VAL(DW3000_DEVICE_1_IRQ),
    .ss_pin = MYNEWT_VAL(DW3000_DEVICE_1_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_1_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_1_TX_ANT_DLY),
    .ext_clock_delay = 0
    },
    {0}
};

static char* dw3000_dev_names[] = {
    "dw3000_0",
    "dw3000_1",
    "dw3000_2"
};

int uwbcore_hal_bsp_dw3000_init(int spi_num, uint32_t devid, struct dpl_sem *sem)
{
    int rc, idx;
    dw3000_dev_instance_t * dw3000_inst;
    if ((devid&0xffffff00) != DWT_DEVICE_ID) {
        return 0;
    }
    dw3000_inst = hal_dw3000_inst(spi_num);

    for (idx=0;idx<ARRAY_SIZE(dw3000_cfg);idx++) {
        if (dw3000_cfg[idx].spi_num == spi_num) {
            break;
        }
    }
    if (idx == ARRAY_SIZE(dw3000_cfg)) {
        return 0;
    }

    dw3000_cfg[idx].spi_sem = sem;
    rc = os_dev_create((struct os_dev *) dw3000_inst, dw3000_dev_names[idx],
            OS_DEV_INIT_PRIMARY, 0, dw3000_dev_init, (void *)&dw3000_cfg[idx]);
    assert(rc == 0);
    printk("uwbcore: %s detected on %d\n", dw3000_dev_names[idx], spi_num);
    return 1;
}

#endif
