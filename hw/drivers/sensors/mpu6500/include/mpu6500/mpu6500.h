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
 *
 * Based of Julian Ingram's mpu6050 sensor driver
 */

#ifndef __SENSOR_MPU6500_H__
#define __SENSOR_MPU6500_H__

#include "os/os.h"
#include "os/os_dev.h"
#include "sensor/sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mpu6500_gyro_range {
    MPU6500_GYRO_RANGE_250           = 0x00 << 3, /* +/- 250 Deg/s  */
    MPU6500_GYRO_RANGE_500           = 0x01 << 3, /* +/- 500 Deg/s  */
    MPU6500_GYRO_RANGE_1000          = 0x02 << 3, /* +/- 1000 Deg/s */
    MPU6500_GYRO_RANGE_2000          = 0x03 << 3, /* +/- 2000 Deg/s */
};

enum mpu6500_accel_range {
    MPU6500_ACCEL_RANGE_2            = 0x00 << 3, /* +/- 2g  */
    MPU6500_ACCEL_RANGE_4            = 0x01 << 3, /* +/- 4g  */
    MPU6500_ACCEL_RANGE_8            = 0x02 << 3, /* +/- 8g  */
    MPU6500_ACCEL_RANGE_16           = 0x03 << 3, /* +/- 16g */
};

enum mpu6500_gyro_rate {
    MPU6500_GYRO_RATE_1000          = 0x00, /* 1000 / (1 + 0) Hz */
    MPU6500_GYRO_RATE_500           = 0x01, /* 1000 / (1 + 1) Hz */
    MPU6500_GYRO_RATE_333           = 0x02, /* 1000 / (1 + 2) Hz */
    MPU6500_GYRO_RATE_250           = 0x03, /* 1000 / (1 + 3) Hz */
    MPU6500_GYRO_RATE_200           = 0x04, /* 1000 / (1 + 4) Hz */
    MPU6500_GYRO_RATE_100           = 0x09, /* 1000 / (1 + 9) Hz */
};

enum mpu6500_clock_select {
    MPU6500_CLK_8MHZ_INTERNAL        = 0x00, /* Internal 8MHz oscillator */
    MPU6500_CLK_GYRO_X               = 0x01, /* PLL with X axis gyroscope reference  */
    MPU6500_CLK_GYRO_Y               = 0x02, /* PLL with Y axis gyroscope reference */
    MPU6500_CLK_GYRO_Z               = 0x03, /* PLL with Z axis gyroscope reference */
    MPU6500_CLK_EXTERNAL_LS          = 0x04, /* PLL with external 32.768kHz reference */
    MPU6500_CLK_EXTERNAL_HS          = 0x05, /* PLL with external 19.2MHz reference */
};

#define MPU6500_INT_LEVEL (0x80)
#define MPU6500_INT_OPEN (0x40)
#define MPU6500_INT_LATCH_EN (0x20)
#define MPU6500_INT_RD_CLEAR (0x10)

struct mpu6500_cfg {
    enum mpu6500_accel_range accel_range;
    enum mpu6500_gyro_range  gyro_range;
    enum mpu6500_clock_select clock_source;
    uint8_t sample_rate_div; /* Sample Rate = Gyroscope Output Rate /
            (1 + sample_rate_div) */
    uint8_t lpf_cfg; /* See data sheet */
    uint8_t int_enable;
    uint8_t int_cfg;
    sensor_type_t mask;
};

struct mpu6500 {
    struct os_dev dev;
    struct sensor sensor;
    struct mpu6500_cfg cfg;
    os_time_t last_read_time;
};

int mpu6500_reset(struct sensor_itf *itf);
int mpu6500_sleep(struct sensor_itf *itf, uint8_t enable);
int mpu6500_set_clock_source(struct sensor_itf *itf,
    enum mpu6500_clock_select source);
int mpu6500_get_clock_source(struct sensor_itf *itf,
    enum mpu6500_clock_select *source);
int mpu6500_set_lpf(struct sensor_itf *itf, uint8_t cfg);
int mpu6500_get_lpf(struct sensor_itf *itf, uint8_t *cfg);
int mpu6500_set_sample_rate(struct sensor_itf *itf, uint8_t rate_div);
int mpu6500_get_sample_rate(struct sensor_itf *itf, uint8_t *rate_div);
int mpu6500_set_gyro_range(struct sensor_itf *itf,
    enum mpu6500_gyro_range range);
int mpu6500_get_gyro_range(struct sensor_itf *itf,
    enum mpu6500_gyro_range *range);
int mpu6500_set_accel_range(struct sensor_itf *itf,
    enum mpu6500_accel_range range);
int mpu6500_get_accel_range(struct sensor_itf *itf,
    enum mpu6500_accel_range *range);
int mpu6500_enable_interrupt(struct sensor_itf *itf, uint8_t enable);
int mpu6500_config_interrupt(struct sensor_itf *itf, uint8_t cfg);

int mpu6500_init(struct os_dev *, void *);
int mpu6500_config(struct mpu6500 *, struct mpu6500_cfg *);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_MPU6500_H__ */
