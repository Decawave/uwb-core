#include <stdio.h>
#include <dpl/dpl.h>
#include <os/os_dev.h>
#include <syscfg/syscfg.h>
#if MYNEWT_VAL(DW1000_DEVICE_0)
#include <dw1000/dw1000_dev.h>
#include <dw1000/dw1000_hal.h>

static int dw1000_idx = 0;
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
    {0}
};

static char* dw1000_dev_names[] = {
    "dw1000_0",
    "dw1000_1",
    "dw1000_2"
};

int uwbcore_hal_bsp_dw1000_init(uint32_t devid, struct dpl_sem *sem)
{
    int rc;
    dw1000_dev_instance_t * dw1000_inst;
    if (devid != DWT_DEVICE_ID) {
        return 0;
    }
    dw1000_inst = hal_dw1000_inst(dw1000_idx);

    dw1000_cfg[dw1000_idx].spi_sem = sem;
    rc = os_dev_create((struct os_dev *) dw1000_inst, dw1000_dev_names[dw1000_idx],
            OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_cfg[dw1000_idx]);
    assert(rc == 0);
    printf("uwbcore: %s detected\n", dw1000_dev_names[dw1000_idx]);
    dw1000_idx++;
    return 1;
}

#endif
