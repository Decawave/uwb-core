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

#ifndef H_BSP_H
#define H_BSP_H

#include <inttypes.h>
#include <syscfg/syscfg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define special stackos sections */
#define sec_data_core   __attribute__((section(".data.core")))
#define sec_bss_core    __attribute__((section(".bss.core")))
#define sec_bss_nz_core __attribute__((section(".bss.core.nz")))

/* More convenient section placement macros. */
#define bssnz_t         sec_bss_nz_core

extern uint8_t _ram_start;
#define RAM_SIZE        0x10000

/* LED pins */
#define LED_1           (15)
#define LED_2           (-1)
#define LED_3           (-1)
#define LED_BLINK_PIN   (LED_1)

/* Buttons */

#define BUTTON_1 (-1)
//#define BUTTON_2 (02)
//#define BUTTON_3 (15)
//#define BUTTON_4 (16)

#define ESP_RXD0_PIN 14
#define ESP_TXD0_PIN 22
#define ESP_PWR_PIN  23
#define ESP_PGM_PIN  27
#define ESP_RST_PIN  13

#define USB_UART_0_PIN_TX 5
#define USB_UART_0_PIN_RX 11

#define USB_V_PIN           NRF_SAADC_INPUT_AIN6
#define BATT_V_PIN          NRF_SAADC_INPUT_AIN7
#define MEAS_BATT_EN_PIN    30

int16_t hal_bsp_read_battery_voltage();
int16_t hal_bsp_read_usb_voltage();
void hal_bsp_esp_bypass(int enable);
void hal_config_lipo_charger(int ma);

#ifdef __cplusplus
}
#endif

#endif  /* H_BSP_H */
