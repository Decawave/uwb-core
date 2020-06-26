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

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "os/mynewt.h"
#include <nrf52.h>
#include "flash_map/flash_map.h"
#include "hal/hal_bsp.h"
#include "hal/hal_system.h"
#include "hal/hal_flash.h"
#include "hal/hal_spi.h"
#include "hal/hal_watchdog.h"
#include "hal/hal_i2c.h"
#include "hal/hal_gpio.h"
#include "mcu/nrf52_hal.h"
#include "mcu/nrf52_periph.h"

#if MYNEWT_VAL(DW1000_DEVICE_0)
#include "dw1000/dw1000_dev.h"
#include "dw1000/dw1000_hal.h"
#endif

#include "os/os_dev.h"
#include "bsp.h"
#if MYNEWT_VAL(ADC_0)
#include <adc_nrf52/adc_nrf52.h>
#include <nrfx_saadc.h>
#endif

#if MYNEWT_VAL(MPU6500_ONB)
#include <mpu6500/mpu6500.h>
static struct mpu6500 mpu6500;
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
struct dpl_sem g_spi0_sem;
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
/*
 * dw1000 device structure defined in dw1000_hal.c
 */
static dw1000_dev_instance_t *dw1000_0 = 0;
static const struct dw1000_dev_cfg dw1000_0_cfg = {
    .spi_sem = &g_spi0_sem,
    .spi_baudrate = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_HIGH),
    .spi_baudrate_low = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_LOW),
    .spi_num = 0,
    .rst_pin  = MYNEWT_VAL(DW1000_DEVICE_0_RST),
    .irq_pin  = MYNEWT_VAL(DW1000_DEVICE_0_IRQ),
    .ss_pin = MYNEWT_VAL(DW1000_DEVICE_0_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_0_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_0_TX_ANT_DLY),
    .ext_clock_delay = 0
};
#endif


#if MYNEWT_VAL(I2C_1)

#if MYNEWT_VAL(MPU6500_ONB)
static struct sensor_itf i2c_1_itf_mpu = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = 0x69        /* 0b1101001 */
};
#endif
#endif

/*
 * What memory to include in coredump.
 */
static const struct hal_bsp_mem_dump dump_cfg[] = {
    [0] = {
        .hbmd_start = &_ram_start,
        .hbmd_size = RAM_SIZE
    }
};



const struct hal_flash * hal_bsp_flash_dev(uint8_t id)
{
    /*
     * Internal flash mapped to id 0.
     */
    if (id != 0) {
        return NULL;
    }
    return &nrf52k_flash_dev;
}

const struct hal_bsp_mem_dump * hal_bsp_core_dump(int *area_cnt)
{
    *area_cnt = sizeof(dump_cfg) / sizeof(dump_cfg[0]);
    return dump_cfg;
}

int hal_bsp_power_state(int state)
{
    return (0);
}

/**
 * Returns the configured priority for the given interrupt. If no priority
 * configured, return the priority passed in
 *
 * @param irq_num
 * @param pri
 *
 * @return uint32_t
 */
uint32_t hal_bsp_get_nvic_priority(int irq_num, uint32_t pri)
{
    uint32_t cfg_pri;

    switch (irq_num) {
    /* Radio gets highest priority */
    case RADIO_IRQn:
        cfg_pri = 0;
        break;
    default:
        cfg_pri = pri;
    }
    return cfg_pri;
}


/**
 * MPU6500 Sensor default configuration
 *
 * @return 0 on success, non-zero on failure
 */
int
config_mpu6500_sensor(void)
{
#if MYNEWT_VAL(MPU6500_ONB)
    int rc;
    struct os_dev *dev;
    struct mpu6500_cfg cfg;

    dev = (struct os_dev *) os_dev_open("mpu6500_0", OS_TIMEOUT_NEVER, NULL);
    assert(dev != NULL);

    memset(&cfg, 0, sizeof(cfg));

    cfg.mask = SENSOR_TYPE_ACCELEROMETER | SENSOR_TYPE_GYROSCOPE;
    cfg.accel_range = MPU6500_ACCEL_RANGE_16;
    cfg.gyro_range = MPU6500_GYRO_RANGE_2000;
    cfg.sample_rate_div = MPU6500_GYRO_RATE_200;
    cfg.lpf_cfg = 0;
    cfg.int_enable = 0;
    cfg.int_cfg = 0;

    rc = mpu6500_config((struct mpu6500 *)dev, &cfg);
    SYSINIT_PANIC_ASSERT(rc == 0);

    os_dev_close(dev);
#endif
    return 0;
}

static void
sensor_dev_create(void)
{
    int rc;
    (void)rc;

#if MYNEWT_VAL(MPU6500_ONB)
    rc = os_dev_create((struct os_dev *) &mpu6500, "mpu6500_0",
      OS_DEV_INIT_PRIMARY, 0, mpu6500_init, (void *)&i2c_1_itf_mpu);
    assert(rc == 0);
#endif

}


void hal_bsp_init(void)
{
    /* Make sure system clocks have started */
    hal_system_clock_start();

    /* Create all available nRF52832 peripherals */
    nrf52_periph_create();

#if MYNEWT_VAL(ADC_0)
    hal_gpio_init_out(EXTON_PIN, 0);
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
    dpl_error_t err = dpl_sem_init(&g_spi0_sem, 0x1);
    assert(err == DPL_OK);
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
    hal_gpio_init_out(DW1000_ENABLE_N_PIN, 0);
    os_cputime_delay_usecs(10000);

    dw1000_0 = hal_dw1000_inst(0);
    int rc = os_dev_create((struct os_dev *) dw1000_0, "dw1000_0",
      OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_0_cfg);
    assert(rc == 0);
#else
    hal_gpio_init_out(DW1000_ENABLE_N_PIN, 1);
#endif

    sensor_dev_create();
}

/**/
int16_t
hal_bsp_read_battery_voltage()
{
#if MYNEWT_VAL(ADC_0)
    int rc = 0;
    int result;
    nrfx_saadc_config_t os_bsp_adc0_nrf_config =
        NRFX_SAADC_DEFAULT_CONFIG;
    nrf_saadc_channel_config_t channel_config =
        NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATT_V_PIN);

    struct adc_dev *dev = (struct adc_dev *)os_dev_open(
    "adc0", OS_TIMEOUT_NEVER, &os_bsp_adc0_nrf_config);

    hal_gpio_write(EXTON_PIN, 1);

    rc = adc_chan_config(dev, 0, (void*)&channel_config);
    assert(rc==0);
    adc_read_channel(dev, 0, &result);
    hal_gpio_write(EXTON_PIN, 0);

    os_dev_close((struct os_dev*)dev);
    return (result*3600*24)/10240;
#else
    return -1;
#endif
}
