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
#include <nrf52840.h>
#include "os/os_cputime.h"
#include "syscfg/syscfg.h"
#include "sysflash/sysflash.h"
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


#if MYNEWT_VAL(DW1000_DEVICE_0) || MYNEWT_VAL(DW1000_DEVICE_1)
#include "dw1000/dw1000_dev.h"
#include "dw1000/dw1000_hal.h"
#include "dw1000/dw1000_phy.h"
#endif

#include "os/os_dev.h"
#include "bsp.h"


#if MYNEWT_VAL(SPI_0_MASTER)
struct dpl_sem g_spi0_sem;
#endif
#if MYNEWT_VAL(SPI_3_MASTER)
struct dpl_sem g_spi3_sem;
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
/*
 * dw1000 device structure defined in dw1000_hal.c
 */
static struct _dw1000_dev_instance_t * dw1000_0 = 0;
static const struct dw1000_dev_cfg dw1000_0_cfg = {
#if MYNEWT_VAL(DW1000_DEVICE_SPI_IDX)==0
    .spi_sem = &g_spi0_sem,
#else
    .spi_sem = &g_spi3_sem,
#endif
    .spi_baudrate = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_HIGH),
    .spi_baudrate_low = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_LOW),
    .spi_num = MYNEWT_VAL(DW1000_DEVICE_SPI_IDX),
    .rst_pin  = MYNEWT_VAL(DW1000_DEVICE_0_RST),
    .irq_pin  = MYNEWT_VAL(DW1000_DEVICE_0_IRQ),
    .ss_pin = MYNEWT_VAL(DW1000_DEVICE_0_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_0_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_0_TX_ANT_DLY),
    .ext_clock_delay = 0
};
#endif

#if MYNEWT_VAL(DW1000_DEVICE_1)
/*
 * dw1000 device structure defined in dw1000_hal.c
 */
static struct _dw1000_dev_instance_t *dw1000_1 = 0;
static const struct dw1000_dev_cfg dw1000_1_cfg = {
#if MYNEWT_VAL(DW1000_DEVICE_SPI_IDX)==0
    .spi_sem = &g_spi0_sem,
#else
    .spi_sem = &g_spi3_sem,
#endif
    .spi_baudrate = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_HIGH),
    .spi_baudrate_low = MYNEWT_VAL(DW1000_DEVICE_BAUDRATE_LOW),
    .spi_num = MYNEWT_VAL(DW1000_DEVICE_SPI_IDX),
    .rst_pin  = MYNEWT_VAL(DW1000_DEVICE_1_RST),
    .irq_pin  = MYNEWT_VAL(DW1000_DEVICE_1_IRQ),
    .ss_pin = MYNEWT_VAL(DW1000_DEVICE_1_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_1_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW1000_DEVICE_1_TX_ANT_DLY),
    .ext_clock_delay = 0
};
#endif



#if MYNEWT_VAL(SPI_2_MASTER)
struct os_mutex g_spi2_mutex;

static struct hal_spi_settings os_bsp_spi2m_settings = {
    .data_order = HAL_SPI_MSB_FIRST,
    .data_mode = HAL_SPI_MODE3,
    .baudrate = 4000,
    .word_size = HAL_SPI_WORD_SIZE_8BIT,
};


static void
spi2_three_wire_read(int en)
{
    int rc;
    struct nrf52_hal_spi_cfg spi_read_cfg = {
        .sck_pin      =  os_bsp_spi2m_cfg.sck_pin,
        .mosi_pin     =  0xff, /* Not used */
        .miso_pin     =  os_bsp_spi2m_cfg.mosi_pin,
    };

    hal_gpio_init_in(os_bsp_spi2m_cfg.sck_pin, HAL_GPIO_PULL_UP);

    if (en) {
        /* Reconfig spi for reading from 3wire */
        hal_spi_disable(2);
        rc = hal_spi_init(2, (void *)&spi_read_cfg, HAL_SPI_TYPE_MASTER);
        assert(rc == 0);
        rc = hal_spi_config(2, &os_bsp_spi2m_settings);
        assert(rc == 0);
        rc = hal_spi_enable(2);
        assert(rc == 0);
    } else {
        /* Normal 4 wire config */
        hal_spi_disable(2);
        rc = hal_spi_init(2, (void *)&os_bsp_spi2m_cfg, HAL_SPI_TYPE_MASTER);
        assert(rc == 0);
        rc = hal_spi_config(2, &os_bsp_spi2m_settings);
        assert(rc == 0);
        rc = hal_spi_enable(2);
        assert(rc == 0);
    }

}

#endif


#if MYNEWT_VAL(I2C_1)
struct os_mutex g_i2c1_mutex;
#endif


#if MYNEWT_VAL(LSM6DSL_ONB)
#include <lsm6dsl/lsm6dsl.h>
static struct lsm6dsl lsm6dsl = {
#if MYNEWT_VAL(LSM6DSL_USE_SPI)
    .bus_mutex = &g_spi2_mutex,
#else
    .bus_mutex = &g_i2c1_mutex,
#endif
};

#if MYNEWT_VAL(LSM6DSL_USE_SPI)
static struct sensor_itf itf_lsm = {
    .si_type = SENSOR_ITF_SPI,
    .si_num  = 2,
    .si_cs_pin = LSM6DSL_CS_PIN,
};
#else
static struct sensor_itf itf_lsm = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = LSM6DSL_I2C_ADDR
};
#endif

#endif


#if MYNEWT_VAL(LIS2MDL_ONB)
#include <lis2mdl/lis2mdl.h>

static struct lis2mdl lis2mdl = {
#if MYNEWT_VAL(LIS2MDL_USE_SPI)
    .bus_mutex = &g_spi2_mutex,
    .spi_read_cb = spi2_three_wire_read,
#else
    .bus_mutex = &g_i2c1_mutex,
#endif
};

#if MYNEWT_VAL(LIS2MDL_USE_SPI)
static struct sensor_itf itf_lis = {
    .si_type = SENSOR_ITF_SPI,
    .si_num  = 2,
    .si_cs_pin = LIS2MDL_CS_PIN,
};
#else
static struct sensor_itf itf_lis = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = LIS2MDL_I2C_ADDR
};
#endif

#endif

#if MYNEWT_VAL(LPS22HB_ONB)
#include <lps22hb/lps22hb.h>
static struct lps22hb lps22hb = {
#if MYNEWT_VAL(LPS22HB_USE_SPI)
    .bus_mutex = &g_spi2_mutex,
#else
    .bus_mutex = &g_i2c1_mutex,
#endif
};

#if MYNEWT_VAL(LPS22HB_USE_SPI)
static struct sensor_itf itf_lhb = {
    .si_type = SENSOR_ITF_SPI,
    .si_num  = 2,
    .si_cs_pin = LPS22HB_CS_PIN,
};
#else
static struct sensor_itf itf_lhb = {
    .si_type = SENSOR_ITF_I2C,
    .si_num  = 1,
    .si_addr = LPS22HB_I2C_ADDR
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



const struct hal_flash *
hal_bsp_flash_dev(uint8_t id)
{
    switch (id) {
    case 0:
        /* MCU internal flash. */
        return &nrf52k_flash_dev;
    default:
        /* External flash.  Assume not present in this BSP. */
        return NULL;
    }
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
      OS_DEV_INIT_PRIMARY, 0, lsm6dsl_init, (void *)&itf_lsm);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(LIS2MDL_ONB)
    rc = os_dev_create((struct os_dev *) &lis2mdl, "lis2mdl_0",
      OS_DEV_INIT_PRIMARY, 0, lis2mdl_init, (void *)&itf_lis);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(LPS22HB_ONB)
    rc = os_dev_create((struct os_dev *) &lps22hb, "lps22hb_0",
      OS_DEV_INIT_PRIMARY, 0, lps22hb_init, (void *)&itf_lhb);
    assert(rc == 0);
#endif

}

#if MYNEWT_VAL(DW1000_DEVICE_0) || MYNEWT_VAL(DW1000_DEVICE_1)

static struct uwb_dev_status
hal_bsp_dw_clk_sync(struct uwb_dev * dev)
{
#if !MYNEWT_VAL(DW1000_DEVICE_0) || !MYNEWT_VAL(DW1000_DEVICE_1)
    return dev->status;
#endif
    int n=2;
    dw1000_dev_instance_t * inst[2];
    for(int i=0;i<n;i++) {
        inst[i] = hal_dw1000_inst(i);
    }

    /* Prepare for sync */
    hal_gpio_init_out(MYNEWT_VAL(DW1000_PDOA_SYNC), 0);
    hal_gpio_init_out(MYNEWT_VAL(DW1000_PDOA_SYNC_CLR), 1);
    hal_gpio_init_out(MYNEWT_VAL(DW1000_PDOA_SYNC_EN), 1);

    for (uint8_t i = 0; i < n; i++ ) {
        dw1000_phy_external_sync(inst[i],33, true);
    }

    hal_gpio_write(MYNEWT_VAL(DW1000_PDOA_SYNC), 1);
    /* Only needs to be active for 1 period of 38.4MHz => < 1usec */
    os_cputime_delay_usecs(1);
    hal_gpio_write(MYNEWT_VAL(DW1000_PDOA_SYNC), 0);

    for (uint8_t i = 0; i < n; i++ ) {
        /* Verify that chip was synced */
        uint32_t status = dw1000_read_reg(inst[i], SYS_STATUS_ID, 0, sizeof(uint32_t));
        inst[i]->uwb_dev.status.ext_sync = (status&SYS_STATUS_ESYNCR) != 0;

        /* Clear sync status and ext sync registers */
        dw1000_write_reg(inst[i], SYS_STATUS_ID, 0, status&SYS_STATUS_ESYNCR, sizeof(uint32_t));
        dw1000_phy_external_sync(inst[i],0, false);
    }

    hal_gpio_write(MYNEWT_VAL(DW1000_PDOA_SYNC_CLR), 0);
    hal_gpio_write(MYNEWT_VAL(DW1000_PDOA_SYNC_EN), 0);

    return dev->status;
}
#endif


void hal_bsp_init(void)
{
    int rc;

    (void)rc;

    /* Make sure system clocks have started */
    hal_system_clock_start();

    /* Create all available nRF52832 peripherals */
    nrf52_periph_create();

#if MYNEWT_VAL(I2C_1)
    hal_gpio_init_in(LSM6DSL_SDO_PIN, HAL_GPIO_PULL_UP);
    hal_gpio_init_in(LPS22HB_SDO_PIN, HAL_GPIO_PULL_UP);

    hal_gpio_init_out(LSM6DSL_CS_PIN, 1);
    hal_gpio_init_out(LIS2MDL_CS_PIN, 1);
    hal_gpio_init_out(LPS22HB_CS_PIN, 1);
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
    rc = dpl_sem_init(&g_spi0_sem, 0x1);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(SPI_3_MASTER)
    rc = dpl_sem_init(&g_spi3_sem, 0x1);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
    dw1000_0 = hal_dw1000_inst(0);
    rc = os_dev_create((struct os_dev *) dw1000_0, "dw1000_0",
      OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_0_cfg);
    assert(rc == 0);
    /* Set external sync function */
    dw1000_0->uwb_dev.uw_dyn_funcs.uf_sync_to_ext_clock = hal_bsp_dw_clk_sync;
#endif

#if MYNEWT_VAL(DW1000_DEVICE_1)
    dw1000_1 = hal_dw1000_inst(1);
    rc = os_dev_create((struct os_dev *) dw1000_1, "dw1000_1",
      OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_1_cfg);
    assert(rc == 0);
    /* Set external sync function */
    dw1000_1->uwb_dev.uw_dyn_funcs.uf_sync_to_ext_clock = hal_bsp_dw_clk_sync;
#endif

#if MYNEWT_VAL(SPI_2_MASTER)
    hal_gpio_init_out(LSM6DSL_CS_PIN, 1);
    hal_gpio_init_out(LIS2MDL_CS_PIN, 1);
    hal_gpio_init_out(LPS22HB_CS_PIN, 1);
    hal_gpio_init_in(LSM6DSL_SDO_PIN, HAL_GPIO_PULL_UP);
    hal_gpio_init_in(LPS22HB_SDO_PIN, HAL_GPIO_PULL_UP);

    hal_spi_disable(2);
    rc = hal_spi_config(2, &os_bsp_spi2m_settings);
    assert(rc == 0);
    rc = hal_spi_enable(2);
    assert(rc == 0);

    rc = dpl_mutex_init(&g_spi2_mutex);
    assert(rc == 0);
#endif
    sensor_dev_create();
}
