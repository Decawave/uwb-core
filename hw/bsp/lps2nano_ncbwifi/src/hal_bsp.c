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
#include "syscfg/syscfg.h"
#include "sysflash/sysflash.h"
#include "flash_map/flash_map.h"
#include "hal/hal_bsp.h"
#include "hal/hal_system.h"
#include "hal/hal_flash.h"
#include "hal/hal_gpio.h"
#include "mcu/nrf52_hal.h"
#include "mcu/nrf52_periph.h"


#if MYNEWT_VAL(NCBWIFI_ESP_PASSTHROUGH)
#include "nrfx_gpiote.h"
#include "nrfx_ppi.h"
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
#include "dw1000/dw1000_dev.h"
#include "dw1000/dw1000_hal.h"
#endif

#if MYNEWT_VAL(ADC_0)
#include <adc_nrf52/adc_nrf52.h>
#include <nrfx_saadc.h>
#endif

#if MYNEWT_VAL(UART_0)
#include "hal/hal_uart.h"
#endif

#include "os/os_dev.h"
#include "bsp.h"

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
    .si_addr = 0b1101010
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
#if MYNEWT_VAL(LPS22HB_CONTINOUS_50Hz)
    cfg.output_rate = LPS22HB_OUTPUT_RATE_50;
    cfg.lpf_cfg = LPS22HB_LPF_CONFIG_ODR_9;
#else
    cfg.output_rate = LPS22HB_OUTPUT_RATE_ONESHOT;
    cfg.lpf_cfg = LPS22HB_LPF_CONFIG_DISABLED;
#endif
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

int
config_lipo_charger(void)
{
#if MYNEWT_VAL(DW1000_DEVICE_0)
    /* Disable LED functions if active */
    dw1000_gpio_config_leds(dw1000_0, 0);
    /* Set Lipo charger to 1A, (charge + used = 1A) */
    hal_config_lipo_charger(1000);
#endif
    return 0;
}

void
hal_config_lipo_charger(int ma)
{
#if MYNEWT_VAL(DW1000_DEVICE_0)
    dw1000_gpio_config_leds(dw1000_0, 0);
    int A=0,B=0;
    if (ma < 500) {
        A=B=0;
    } else if (ma<600) {
        A=1;
    } else if (ma>999) {
        A=B=1;
    }
    dw1000_gpio_init_out(dw1000_0, 0, A);
    dw1000_gpio_init_out(dw1000_0, 1, B);
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

#if MYNEWT_VAL(ADC_0)
    hal_gpio_init_out(MEAS_BATT_EN_PIN, 0);
#endif

#if MYNEWT_VAL(I2C_1)
    {
        dpl_error_t err = os_mutex_init(&g_i2c1_mutex);
        assert(err == DPL_OK);
    }
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
    {
        dpl_error_t err = dpl_sem_init(&g_spi0_sem, 0x1);
        assert(err == DPL_OK);
    }
#endif

#if MYNEWT_VAL(DW1000_DEVICE_0)
    dw1000_0 = hal_dw1000_inst(0);
    rc = os_dev_create((struct os_dev *) dw1000_0, "dw1000_0",
      OS_DEV_INIT_PRIMARY, 0, dw1000_dev_init, (void *)&dw1000_0_cfg);
    assert(rc == 0);

#if MYNEWT_VAL(NCBWIFI_EXT_38_4_MHZ)
    /* This assumes a wire from board's GPIO1 to the clock mux */
    hal_gpio_init_out(GPIO_PAD_P2_1, 1);
#endif

#endif

    /* Enable esp at boot (0=active, 1=inactive) */
    hal_gpio_init_out(ESP_PWR_PIN, MYNEWT_VAL(ESP12F_ENABLED) == 0);

    sensor_dev_create();

//#if MYNEWT_VAL(NCBWIFI_ESP_PASSTHROUGH)
//    hal_bsp_esp_bypass(true);
//#endif
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
    if (dev == 0) {
        /* Not possible to use adc (pin occupied)  */
        return -1;
    }
    assert(dev);
    hal_gpio_write(MEAS_BATT_EN_PIN, 1);

    rc = adc_chan_config(dev, 0, (void*)&channel_config);
    assert(rc==0);
    adc_read_channel(dev, 0, &result);
    hal_gpio_write(MEAS_BATT_EN_PIN, 0);

    os_dev_close((struct os_dev*)dev);
    return (result*3600*24)/10240;
#else
    return -1;
#endif
}


/**/
int16_t
hal_bsp_read_usb_voltage()
{
#if MYNEWT_VAL(ADC_0)
    int rc = 0;
    int result;
    nrfx_saadc_config_t os_bsp_adc0_nrf_config =
        NRFX_SAADC_DEFAULT_CONFIG;

    nrf_saadc_channel_config_t channel_config =
        NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(USB_V_PIN);

    struct adc_dev *dev = (struct adc_dev *)os_dev_open(
        "adc0", OS_TIMEOUT_NEVER, &os_bsp_adc0_nrf_config);
    if (dev == 0) {
        /* Not possible to use adc (pin occupied)  */
        return -1;
    }
    assert(dev);

    rc = adc_chan_config(dev, 0, (void*)&channel_config);
    assert(rc==0);
    adc_read_channel(dev, 0, &result);

    os_dev_close((struct os_dev*)dev);
    return (result*3600*57)/10240;
#else
    return -1;
#endif
}

void
hal_bsp_esp_enter_bootloader()
{
    hal_gpio_init_out(ESP_RST_PIN, 0);
    hal_gpio_init_out(ESP_PGM_PIN, 0);
    os_cputime_delay_usecs(50000);
    hal_gpio_write(ESP_RST_PIN, 1);
    os_cputime_delay_usecs(100000);
    hal_gpio_init_in(ESP_PGM_PIN, HAL_GPIO_PULL_UP);
}


void
hal_bsp_esp_bypass(int enable)
{
#if MYNEWT_VAL(NCBWIFI_ESP_PASSTHROUGH)
    int bypass_i[] = {ESP_TXD0_PIN, USB_UART_0_PIN_RX}; //MYNEWT_VAL(UART_0_PIN_RTS)
    int bypass_o[] = {USB_UART_0_PIN_TX, ESP_RXD0_PIN, ESP_RST_PIN};
#if MYNEWT_VAL(UART_0)
    hal_uart_close(0);
#endif
    hal_gpio_init_out(ESP_RST_PIN, 1);
    hal_gpio_init_in(ESP_PGM_PIN, HAL_GPIO_PULL_UP);
    hal_gpio_init_out(ESP_PWR_PIN, 0);
#if MYNEWT_VAL(NCBWIFI_ESP_ENTER_BL)
    hal_bsp_esp_enter_bootloader();
#endif

    if (!nrfx_gpiote_is_init()) {
        nrfx_gpiote_init();
    }

    for (int i=0;i<sizeof(bypass_i)/sizeof(bypass_i[0]);i++) {
        uint32_t inpin = bypass_i[i];
        uint32_t outpin = bypass_o[i];
        hal_gpio_init_in(inpin, HAL_GPIO_PULL_NONE);
        int initial_value = hal_gpio_read(inpin);

        nrfx_gpiote_in_config_t in_p_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
        nrfx_err_t err = nrfx_gpiote_in_init(inpin, &in_p_config, 0);
        assert(err==NRFX_SUCCESS);

        uint32_t input_ev_addr = nrfx_gpiote_in_event_addr_get(inpin);
        nrfx_gpiote_out_config_t p_config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(initial_value);
        err = nrfx_gpiote_out_init(outpin, &p_config);
        assert(err==NRFX_SUCCESS);
        nrfx_gpiote_out_task_enable(outpin);
        uint32_t output_task_addr = nrfx_gpiote_out_task_addr_get(outpin);

        nrf_ppi_channel_t ppi_channel;
        err = nrfx_ppi_channel_alloc(&ppi_channel);
        assert(err==NRFX_SUCCESS);
        err = nrfx_ppi_channel_assign(ppi_channel, input_ev_addr, output_task_addr);
        assert(err==NRFX_SUCCESS);
        err = nrfx_ppi_channel_enable(ppi_channel);
        assert(err==NRFX_SUCCESS);
        nrfx_gpiote_in_event_enable(inpin, true);
    }
#endif
}
