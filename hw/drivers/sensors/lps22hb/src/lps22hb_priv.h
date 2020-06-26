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

#ifndef __LPS22HB_PRIV_H__
#define __LPS22HB_PRIV_H__

#ifdef __cplusplus
extern "C" {
#endif

// Inspired by Kris Winer (https://github.com/kriswiner/LSM6DSM_LPS22HB_LPS22HB)
// Adapted for NRF52 by Niklas Casaril <niklas@loligoelectronics.com>

enum lps22hb_registers {
    LPS22HB_INTERRUPT_CFG = 0x0b,
    LPS22HB_THIS_P_L      = 0x0c,
    LPS22HB_THIS_P_H      = 0x0d,
    LPS22HB_WHO_AM_I      = 0x0f,

    LPS22HB_CTRL_REG1     = 0x10,
    LPS22HB_CTRL_REG2     = 0x11,
    LPS22HB_CTRL_REG3     = 0x12,
    LPS22HB_FIFO_CTRL     = 0x14,
    LPS22HB_REF_P_XL      = 0x15,
    LPS22HB_REF_P_L       = 0x16,
    LPS22HB_REF_P_H       = 0x17,
    LPS22HB_RPDS_L        = 0x18,
    LPS22HB_RPDS_H        = 0x19,
    LPS22HB_RES_CONF      = 0x1A,
    LPS22HB_INT_SOURCE    = 0x25,
    LPS22HB_FIFO_STATUS   = 0x26,
    LPS22HB_STATUS        = 0x27,
    LPS22HB_PRESS_OUT_XL  = 0x28,
    LPS22HB_PRESS_OUT_L   = 0x29,
    LPS22HB_PRESS_OUT_H   = 0x2A,
    LPS22HB_TEMP_OUT_L    = 0x2B,
    LPS22HB_TEMP_OUT_H    = 0x2C,
    LPS22HB_LPFP_RES      = 0x33,
};

#define LPS22HB_REG2_ONESHOT (1<<0)
#define LPS22HB_REG2_RESET (1<<2)
#define LPS22HB_REG3_DRDY (1<<2)

int lps22hb_write8(struct lps22hb *dev, uint8_t reg, uint32_t value);
int lps22hb_read8(struct lps22hb *dev, uint8_t reg, uint8_t *value);
int lps22hb_read_bytes(struct lps22hb *dev, uint8_t reg, uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __LPS22HB_PRIV_H__ */
