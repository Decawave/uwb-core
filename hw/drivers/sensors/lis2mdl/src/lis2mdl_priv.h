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

#ifndef __LIS2MDL_PRIV_H__
#define __LIS2MDL_PRIV_H__

#ifdef __cplusplus
extern "C" {
#endif

// Inspired by Kris Winer (https://github.com/kriswiner/LSM6DSM_LIS2MDL_LPS22HB)
// Adapted for NRF52 by Niklas Casaril <niklas@loligoelectronics.com>

enum lis2mdl_registers {
    LIS2MDL_OFFSET_X_REG_L		    = 0x45,
    LIS2MDL_OFFSET_X_REG_H		    = 0x46,
    LIS2MDL_OFFSET_Y_REG_L		    = 0x47,
    LIS2MDL_OFFSET_Y_REG_H		    = 0x48,
    LIS2MDL_OFFSET_Z_REG_L		    = 0x49,
    LIS2MDL_OFFSET_Z_REG_H		    = 0x4A,
    LIS2MDL_WHO_AM_I			    = 0x4F,  /* Should be 0x40 */
    LIS2MDL_CFG_REG_A			    = 0x60,
    LIS2MDL_CFG_REG_B			    = 0x61,
    LIS2MDL_CFG_REG_C			    = 0x62,
    LIS2MDL_INT_CTRL_REG		    = 0x63,
    LIS2MDL_INT_SOURCE_REG		    = 0x64,
    LIS2MDL_INT_THS_L_REG		    = 0x65,
    LIS2MDL_INT_THS_H_REG		    = 0x66,
    LIS2MDL_STATUS_REG			    = 0x67,
    LIS2MDL_OUTX_L_REG			    = 0x68,
    LIS2MDL_OUTX_H_REG			    = 0x69,
    LIS2MDL_OUTY_L_REG			    = 0x6A,
    LIS2MDL_OUTY_H_REG			    = 0x6B,
    LIS2MDL_OUTZ_L_REG			    = 0x6C,
    LIS2MDL_OUTZ_H_REG			    = 0x6D,
    LIS2MDL_TEMP_OUT_L_REG		    = 0x6E,
    LIS2MDL_TEMP_OUT_H_REG		    = 0x6F,
};

int lis2mdl_write8(struct lis2mdl *dev, uint8_t reg, uint32_t value);
int lis2mdl_read8(struct lis2mdl *dev, uint8_t reg, uint8_t *value);
int lis2mdl_read_bytes(struct lis2mdl *dev, uint8_t reg, uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __LIS2MDL_PRIV_H__ */
