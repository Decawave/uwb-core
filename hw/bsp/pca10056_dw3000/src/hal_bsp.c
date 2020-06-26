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
#include "os/mynewt.h"
#include "nrfx.h"
#include "flash_map/flash_map.h"
#include "hal/hal_bsp.h"
#include "hal/hal_gpio.h"
#include "hal/hal_flash.h"
#include "hal/hal_system.h"
#include "mcu/nrf52_hal.h"
#include "mcu/nrf52_periph.h"
#include "bsp/bsp.h"

#if MYNEWT_VAL(DW3000_DEVICE_0)
#include "dw3000-c0/dw3000_dev.h"
#include "dw3000-c0/dw3000_hal.h"
#include "dw3000-c0/dw3000_phy.h"
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
struct dpl_sem g_spi0_sem;

#if MYNEWT_VAL(DW3000_DEVICE_0)
/*
 * dw1000 device structure defined in dw1000_hal.c
 */
static struct _dw3000_dev_instance_t * dw3000_0 = 0;
static const struct dw3000_dev_cfg dw3000_0_cfg = {
    .spi_sem = &g_spi0_sem,
    .spi_baudrate = MYNEWT_VAL(DW3000_DEVICE_BAUDRATE),
    .spi_num = 0,
    .rst_pin  = MYNEWT_VAL(DW3000_DEVICE_0_RST),
    .irq_pin  = MYNEWT_VAL(DW3000_DEVICE_0_IRQ),
    .ss_pin = MYNEWT_VAL(DW3000_DEVICE_0_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_0_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_0_TX_ANT_DLY),
    .ext_clock_delay = 0
};
#endif

#endif

#if MYNEWT_VAL(SPI_3_MASTER)
struct dpl_sem g_spi3_sem;

#if MYNEWT_VAL(DW3000_DEVICE_0)
/*
 * dw1000 device structure defined in dw1000_hal.c
 */
static struct _dw3000_dev_instance_t * dw3000_0 = 0;
static const struct dw3000_dev_cfg dw3000_0_cfg = {
    .spi_sem = &g_spi3_sem,
    .spi_baudrate = MYNEWT_VAL(DW3000_DEVICE_BAUDRATE),
    .spi_num = 3,
    .rst_pin  = MYNEWT_VAL(DW3000_DEVICE_0_RST),
    .irq_pin  = MYNEWT_VAL(DW3000_DEVICE_0_IRQ),
    .ss_pin = MYNEWT_VAL(DW3000_DEVICE_0_SS),
    .rx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_0_RX_ANT_DLY),
    .tx_antenna_delay = MYNEWT_VAL(DW3000_DEVICE_0_TX_ANT_DLY),
    .ext_clock_delay = 0
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
    /*
     * Internal flash mapped to id 0.
     */
    if (id == 0) {
        return &nrf52k_flash_dev;
    }
#if MYNEWT_VAL(QSPI_ENABLE)
    if (id == 1) {
        return &nrf52k_qspi_dev;
    }
#endif
    return NULL;
}

const struct hal_bsp_mem_dump *
hal_bsp_core_dump(int *area_cnt)
{
    *area_cnt = sizeof(dump_cfg) / sizeof(dump_cfg[0]);
    return dump_cfg;
}

int
hal_bsp_power_state(int state)
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
uint32_t
hal_bsp_get_nvic_priority(int irq_num, uint32_t pri)
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

void
hal_bsp_init(void)
{
    /* Make sure system clocks have started */
    hal_system_clock_start();

    /* Create all available nRF52840 peripherals */
    nrf52_periph_create();

    /* TODO: Generalise these */
    hal_gpio_init_in(0x20 + 11, HAL_GPIO_PULL_NONE);   /* Wake */
    hal_gpio_init_in(0x20 + 10, HAL_GPIO_PULL_NONE);   /* Irq */
    hal_gpio_init_in(0x20 +  6, HAL_GPIO_PULL_NONE);   /* TCXO - NOT PRESENT? */

#if MYNEWT_VAL(SPI_0_MASTER)
    int rc;
    rc = dpl_sem_init(&g_spi0_sem, 0x1);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(SPI_3_MASTER)
    int rc;
    rc = dpl_sem_init(&g_spi3_sem, 0x1);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(DW3000_DEVICE_0)
    dw3000_0 = hal_dw3000_inst(0);
    rc = os_dev_create((struct os_dev *) dw3000_0, "dw3000_0",
      OS_DEV_INIT_PRIMARY, 0, dw3000_dev_init, (void *)&dw3000_0_cfg);
    assert(rc == 0);
#endif
}
