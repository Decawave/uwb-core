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

#ifndef __LSM6DSL_PRIV_H__
#define __LSM6DSL_PRIV_H__

#ifdef __cplusplus
extern "C" {
#endif

// Inspired by Kris Winer (https://github.com/kriswiner/LSM6DSM_LIS2MDL_LPS22HB)
// Adapted for NRF52 by Niklas Casaril <niklas@loligoelectronics.com>

/* LSM6DSM registers
  http://www.st.com/content/ccc/resource/technical/document/datasheet/76/27/cf/88/c5/03/42/6b/DM00218116.pdf/files/DM00218116.pdf/jcr:content/translations/en.DM00218116.pdf
*/

enum lsm6dsl_registers {
    LSM6DSL_FUNC_CFG_ACCESS             = 0x01,
    LSM6DSL_SENSOR_SYNC_TIME_FRAME      = 0x04,
    LSM6DSL_SENSOR_SYNC_RES_RATIO       = 0x05,
    LSM6DSL_FIFO_CTRL1                  = 0x06,
    LSM6DSL_FIFO_CTRL2                  = 0x07,
    LSM6DSL_FIFO_CTRL3                  = 0x08,
    LSM6DSL_FIFO_CTRL4                  = 0x09,
    LSM6DSL_FIFO_CTRL5                  = 0x0A,
    LSM6DSL_DRDY_PULSE_CFG              = 0x0B,
    LSM6DSL_INT1_CTRL                   = 0x0D,
    LSM6DSL_INT2_CTRL                   = 0x0E,
    LSM6DSL_WHO_AM_I                    = 0x0F, // should be 0x6A
    LSM6DSL_CTRL1_XL                    = 0x10,
    LSM6DSL_CTRL2_G                     = 0x11,
    LSM6DSL_CTRL3_C                     = 0x12,
    LSM6DSL_CTRL4_C                     = 0x13,
    LSM6DSL_CTRL5_C                     = 0x14,
    LSM6DSL_CTRL6_C                     = 0x15,
    LSM6DSL_CTRL7_G                     = 0x16,
    LSM6DSL_CTRL8_XL                    = 0x17,
    LSM6DSL_CTRL9_XL                    = 0x18,
    LSM6DSL_CTRL10_C                    = 0x19,
    LSM6DSL_MASTER_CONFIG               = 0x1A,
    LSM6DSL_WAKE_UP_SRC                 = 0x1B,
    LSM6DSL_TAP_SRC                     = 0x1C,
    LSM6DSL_D6D_SRC                     = 0x1D,
    LSM6DSL_STATUS_REG                  = 0x1E,
    LSM6DSL_OUT_TEMP_L                  = 0x20,
    LSM6DSL_OUT_TEMP_H                  = 0x21,
    LSM6DSL_OUTX_L_G                    = 0x22,
    LSM6DSL_OUTX_H_G                    = 0x23,
    LSM6DSL_OUTY_L_G                    = 0x24,
    LSM6DSL_OUTY_H_G                    = 0x25,
    LSM6DSL_OUTZ_L_G                    = 0x26,
    LSM6DSL_OUTZ_H_G                    = 0x27,
    LSM6DSL_OUTX_L_XL                   = 0x28,
    LSM6DSL_OUTX_H_XL                   = 0x29,
    LSM6DSL_OUTY_L_XL                   = 0x2A,
    LSM6DSL_OUTY_H_XL                   = 0x2B,
    LSM6DSL_OUTZ_L_XL                   = 0x2C,
    LSM6DSL_OUTZ_H_XL                   = 0x2D,
    LSM6DSL_SENSORHUB1_REG              = 0x2E,
    LSM6DSL_SENSORHUB2_REG              = 0x2F,
    LSM6DSL_SENSORHUB3_REG              = 0x30,
    LSM6DSL_SENSORHUB4_REG              = 0x31,
    LSM6DSL_SENSORHUB5_REG              = 0x32,
    LSM6DSL_SENSORHUB6_REG              = 0x33,
    LSM6DSL_SENSORHUB7_REG              = 0x34,
    LSM6DSL_SENSORHUB8_REG              = 0x35,
    LSM6DSL_SENSORHUB9_REG              = 0x36,
    LSM6DSL_SENSORHUB10_REG             = 0x37,
    LSM6DSL_SENSORHUB11_REG             = 0x38,
    LSM6DSL_SENSORHUB12_REG             = 0x39,
    LSM6DSL_FIFO_STATUS1                = 0x3A,
    LSM6DSL_FIFO_STATUS2                = 0x3B,
    LSM6DSL_FIFO_STATUS3                = 0x3C,
    LSM6DSL_FIFO_STATUS4                = 0x3D,
    LSM6DSL_FIFO_DATA_OUT_L             = 0x3E,
    LSM6DSL_FIFO_DATA_OUT_H             = 0x3F,
    LSM6DSL_TIMESTAMP0_REG              = 0x40,
    LSM6DSL_TIMESTAMP1_REG              = 0x41,
    LSM6DSL_TIMESTAMP2_REG              = 0x42,
    LSM6DSL_STEP_TIMESTAMP_L            = 0x49,
    LSM6DSL_STEP_TIMESTAMP_H            = 0x4A,
    LSM6DSL_STEP_COUNTER_L              = 0x4B,
    LSM6DSL_STEP_COUNTER_H              = 0x4C,
    LSM6DSL_SENSORHUB13_REG             = 0x4D,
    LSM6DSL_SENSORHUB14_REG             = 0x4E,
    LSM6DSL_SENSORHUB15_REG             = 0x4F,
    LSM6DSL_SENSORHUB16_REG             = 0x50,
    LSM6DSL_SENSORHUB17_REG             = 0x51,
    LSM6DSL_SENSORHUB18_REG             = 0x52,
    LSM6DSL_FUNC_SRC1                   = 0x53,
    LSM6DSL_FUNC_SRC2                   = 0x54,
    LSM6DSL_WRIST_TILT_IA               = 0x55,
    LSM6DSL_TAP_CFG                     = 0x58,
    LSM6DSL_TAP_THS_6D                  = 0x59,
    LSM6DSL_INT_DUR2                    = 0x5A,
    LSM6DSL_WAKE_UP_THS                 = 0x5B,
    LSM6DSL_WAKE_UP_DUR                 = 0x5C,
    LSM6DSL_FREE_FALL                   = 0x5D,
    LSM6DSL_MD1_CFG                     = 0x5E,
    LSM6DSL_MD2_CFG                     = 0x5F,
    LSM6DSL_MASTER_MODE_CODE            = 0x60,
    LSM6DSL_SENS_SYNC_SPI_ERROR_CODE    = 0x61,
    LSM6DSL_OUT_MAG_RAW_X_L             = 0x66,
    LSM6DSL_OUT_MAG_RAW_X_H             = 0x67,
    LSM6DSL_OUT_MAG_RAW_Y_L             = 0x68,
    LSM6DSL_OUT_MAG_RAW_Y_H             = 0x69,
    LSM6DSL_OUT_MAG_RAW_Z_L             = 0x6A,
    LSM6DSL_OUT_MAG_RAW_Z_H             = 0x6B,
    LSM6DSL_INT_OIS                     = 0x6F,
    LSM6DSL_CTRL1_OIS                   = 0x70,
    LSM6DSL_CTRL2_OIS                   = 0x71,
    LSM6DSL_CTRL3_OIS                   = 0x72,
    LSM6DSL_X_OFS_USR                   = 0x73,
    LSM6DSL_Y_OFS_USR                   = 0x74,
    LSM6DSL_Z_OFS_USR                   = 0x75,
};

int lsm6dsl_write8(struct lsm6dsl *dev, uint8_t reg, uint32_t value);
int lsm6dsl_read8(struct lsm6dsl *dev, uint8_t reg, uint8_t *value);
int lsm6dsl_read_bytes(struct lsm6dsl *dev, uint8_t reg, uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __LSM6DSL_PRIV_H__ */
