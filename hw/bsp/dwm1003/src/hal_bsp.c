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
#include <sysinit/sysinit.h>
#include <nrf52.h>
#include "os/mynewt.h"
#include "flash_map/flash_map.h"
#include "hal/hal_bsp.h"
#include "hal/hal_system.h"
#include "hal/hal_flash.h"
#include "hal/hal_gpio.h"
#include "mcu/nrf52_hal.h"
#include "mcu/nrf52_periph.h"
#include "bsp/bsp.h"
#include "defs/sections.h"

#if MYNEWT_VAL(DW1000_DEVICE_0)
#include "dw1000/dw1000_dev.h"
#include "dw1000/dw1000_hal.h"
#endif

#if MYNEWT_VAL(UARTBB_0)
#include "uart_bitbang/uart_bitbang.h"
#endif

#if MYNEWT_VAL(UARTBB_0)
static const struct uart_bitbang_conf os_bsp_uartbb0_cfg = {
    .ubc_txpin = MYNEWT_VAL(UARTBB_0_PIN_TX),
    .ubc_rxpin = MYNEWT_VAL(UARTBB_0_PIN_RX),
    .ubc_cputimer_freq = MYNEWT_VAL(OS_CPUTIME_FREQ),
};
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
struct dpl_sem g_spi0_sem;

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

#endif


#if MYNEWT_VAL(I2C_1)
struct os_mutex g_i2c1_mutex;

#if MYNEWT_VAL(LSM6DSL_ONB)
#include <lsm6dsl/lsm6dsl.h>
static struct lsm6dsl lsm6dsl = {
    .bus_mutex = &g_i2c1_mutex,
};

static struct sensor_itf i2c_1_itf_lsm = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = 0b1101011
};

#endif


#if MYNEWT_VAL(LIS2MDL_ONB)
#include <lis2mdl/lis2mdl.h>
static struct lis2mdl lis2mdl = {
    .bus_mutex = &g_i2c1_mutex,
};

static struct sensor_itf i2c_1_itf_lis = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = 0b0011110
};

#endif

#if MYNEWT_VAL(LPS22HB_ONB)
#include <lps22hb/lps22hb.h>
static struct lps22hb lps22hb = {
    .bus_mutex = &g_i2c1_mutex,
};

static struct sensor_itf i2c_1_itf_lhb = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = 0b1011100
};

#endif

#endif /* end MYNEWT_VAL(I2C_1) */


/*
 * What memory to include in coredump.
 */
static const struct hal_bsp_mem_dump dump_cfg[] = {
    [0] = {
        .hbmd_start = &_ram_start,
        .hbmd_size = RAM_SIZE
    }
};

const struct hal_flash *
hal_bsp_flash_dev(uint8_t id)
{
    /*
     * Internal flash mapped to id 0.
     */
    if (id == 0) {
        return &nrf52k_flash_dev;
    }
#if MYNEWT_VAL(ENC_FLASH_DEV)
    if (id == 1) {
        return &enc_flash_dev0.end_dev.efd_hal;
    }
#endif
    return NULL;
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
 * LSM6DSL Sensor default configuration
 *
 * @return 0 on success, non-zero on failure
 */
int
config_lsm6dsl_sensor(void)
{
#if MYNEWT_VAL(LSM6DSL_ONB)
    int rc;
    struct os_dev *dev;
    struct lsm6dsl_cfg cfg;

    dev = (struct os_dev *) os_dev_open("lsm6dsl_0", OS_TIMEOUT_NEVER, NULL);
    assert(dev != NULL);

    memset(&cfg, 0, sizeof(cfg));

    cfg.mask = SENSOR_TYPE_ACCELEROMETER | SENSOR_TYPE_GYROSCOPE;
    cfg.accel_rate  = LSM6DSL_ACCEL_RATE_1660;
    cfg.accel_range = LSM6DSL_ACCEL_RANGE_16;
    cfg.gyro_rate   = LSM6DSL_GYRO_RATE_1660;
    cfg.gyro_range  = LSM6DSL_GYRO_RANGE_2000;
    // enable accel LP2 (bit 7 = 1), set LP2 tp ODR/9 (bit 6 = 1),
    // enable input_composite (bit 3) for low noise
    cfg.lpf_cfg      = 0x80 | 0x40 | 0x08;
    cfg.int_enable  = 0;

    rc = lsm6dsl_config((struct lsm6dsl *)dev, &cfg);
    SYSINIT_PANIC_ASSERT(rc == 0);

    os_dev_close(dev);
#endif
    return 0;
}

/**
 * LIS2MDL Sensor default configuration
 *
 * @return 0 on success, non-zero on failure
 */
int
config_lis2mdl_sensor(void)
{
#if MYNEWT_VAL(LIS2MDL_ONB)
    int rc;
    struct os_dev *dev;
    struct lis2mdl_cfg cfg;

    dev = (struct os_dev *) os_dev_open("lis2mdl_0", OS_TIMEOUT_NEVER, NULL);
    assert(dev != NULL);

    memset(&cfg, 0, sizeof(cfg));

    cfg.mask = SENSOR_TYPE_MAGNETIC_FIELD;
    cfg.output_rate = LIS2MDL_OUTPUT_RATE_100;
    cfg.int_enable = 0;
    cfg.lpf_enable = 1;

    rc = lis2mdl_config((struct lis2mdl *)dev, &cfg);
    SYSINIT_PANIC_ASSERT(rc == 0);

    os_dev_close(dev);
#endif
    return 0;
}


/**
 * LPS22HB Sensor default configuration
 *
 * @return 0 on success, non-zero on failure
 */
int
config_lps22hb_sensor(void)
{
#if MYNEWT_VAL(LPS22HB_ONB)
    int rc;
    struct os_dev *dev;
    struct lps22hb_cfg cfg;

    dev = (struct os_dev *) os_dev_open("lps22hb_0", OS_TIMEOUT_NEVER, NULL);
    assert(dev != NULL);

    memset(&cfg, 0, sizeof(cfg));

    cfg.mask = SENSOR_TYPE_PRESSURE|SENSOR_TYPE_TEMPERATURE;
    cfg.output_rate = LPS22HB_OUTPUT_RATE_ONESHOT;
    cfg.lpf_cfg = LPS22HB_LPF_CONFIG_DISABLED;
    cfg.int_enable = 0;

    rc = lps22hb_config((struct lps22hb *)dev, &cfg);
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

#if MYNEWT_VAL(LSM6DSL_ONB)
    rc = os_dev_create((struct os_dev *) &lsm6dsl, "lsm6dsl_0",
      OS_DEV_INIT_PRIMARY, 0, lsm6dsl_init, (void *)&i2c_1_itf_lsm);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(LIS2MDL_ONB)
    rc = os_dev_create((struct os_dev *) &lis2mdl, "lis2mdl_0",
      OS_DEV_INIT_PRIMARY, 0, lis2mdl_init, (void *)&i2c_1_itf_lis);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(LPS22HB_ONB)
    rc = os_dev_create((struct os_dev *) &lps22hb, "lps22hb_0",
      OS_DEV_INIT_PRIMARY, 0, lps22hb_init, (void *)&i2c_1_itf_lhb);
    assert(rc == 0);
#endif

}


void hal_bsp_init(void)
{
    int rc;
    (void)rc;
    /* Make sure system clocks have started */
    hal_system_clock_start();

    /* Create all available nRF52832 peripherals */
    nrf52_periph_create();

#if MYNEWT_VAL(I2C_1)
    rc = dpl_mutex_init(&g_i2c1_mutex);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
    rc = dpl_sem_init(&g_spi0_sem, 0x1);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
    dw1000_0 = hal_dw1000_inst(0);
    rc = os_dev_create((struct os_dev *) dw1000_0, "dw1000_0",
      OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_0_cfg);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(UARTBB_0)
    rc = os_dev_create(&os_bsp_uartbb0.ud_dev, "uartbb0",
                       OS_DEV_INIT_PRIMARY, 0, uart_bitbang_init,
                       (void *)&os_bsp_uartbb0_cfg);
    assert(rc == 0);
#endif

    sensor_dev_create();
}
