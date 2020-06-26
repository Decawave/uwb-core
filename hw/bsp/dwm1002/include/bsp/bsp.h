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
#define RAM_SIZE        0x40000

/* LED pins */
#define LED_1           (0x14)
#define LED_2           (0x15)
#define LED_3           (0x16)
#define LED_4           (0x17)
#define LED_BLINK_PIN   (LED_1)

/* Buttons */

#define BUTTON_1  (02)
//#define BUTTON_2 (02)
//#define BUTTON_3 (15)
//#define BUTTON_4 (16)

/* I2C address */
#define LSM6DSL_I2C_ADDR (0b1101011)
#define LIS2MDL_I2C_ADDR (0b0011110)
#define LPS22HB_I2C_ADDR (0b1011101)


/* Signal Pins */
#define LSM6DSL_CS_PIN   (0x28)   /* P1.08 */
#define LIS2MDL_CS_PIN   (0x29)   /* P1.09 */
#define LPS22HB_CS_PIN     (11)

#define LSM6DSL_SDO_PIN     (8)
#define LPS22HB_SDO_PIN    (12)

#define LSM6DSL_IRQ2_PIN   (13)
#define LPS22HB_IRQ_PIN    (14)
#define LIS2MDL_IRQ_PIN    (17)
#define LSM6DSL_IRQ1_PIN   (19)

#ifdef __cplusplus
}
#endif

#endif  /* H_BSP_H */
