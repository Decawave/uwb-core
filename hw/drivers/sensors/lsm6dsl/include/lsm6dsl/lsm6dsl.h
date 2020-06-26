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
 */

#ifndef __SENSOR_LSM6DSL_H__
#define __SENSOR_LSM6DSL_H__

#include "os/os.h"
#include "os/os_dev.h"
#include "sensor/sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

enum lsm6dsl_accel_range {
    LSM6DSL_ACCEL_RANGE_2            = 0x00 << 2, /* +/- 2g  */
    LSM6DSL_ACCEL_RANGE_4            = 0x02 << 2, /* +/- 4g  */
    LSM6DSL_ACCEL_RANGE_8            = 0x03 << 2, /* +/- 8g  */
    LSM6DSL_ACCEL_RANGE_16           = 0x01 << 2, /* +/- 16g */
};

enum lsm6dsl_gyro_range {
    LSM6DSL_GYRO_RANGE_245           = 0x00 << 2, /* +/- 250 Deg/s  */
    LSM6DSL_GYRO_RANGE_500           = 0x01 << 2, /* +/- 500 Deg/s  */
    LSM6DSL_GYRO_RANGE_1000          = 0x02 << 2, /* +/- 1000 Deg/s */
    LSM6DSL_GYRO_RANGE_2000          = 0x03 << 2, /* +/- 2000 Deg/s */
};

enum lsm6dsl_accel_rate {
    LSM6DSL_ACCEL_RATE_POWER_DOWN    = 0x00 << 4,
    LSM6DSL_ACCEL_RATE_12_5          = 0x01 << 4,
    LSM6DSL_ACCEL_RATE_26            = 0x02 << 4,
    LSM6DSL_ACCEL_RATE_52            = 0x03 << 4,
    LSM6DSL_ACCEL_RATE_104           = 0x04 << 4,
    LSM6DSL_ACCEL_RATE_208           = 0x05 << 4,
    LSM6DSL_ACCEL_RATE_416           = 0x06 << 4,
    LSM6DSL_ACCEL_RATE_833           = 0x07 << 4,
    LSM6DSL_ACCEL_RATE_1660          = 0x08 << 4,
    LSM6DSL_ACCEL_RATE_3330          = 0x09 << 4,
    LSM6DSL_ACCEL_RATE_6660          = 0x0A << 4,
};

enum lsm6dsl_gyro_rate {
    LSM6DSL_GYRO_RATE_POWER_DOWN    = 0x00 << 4,
    LSM6DSL_GYRO_RATE_12_5          = 0x01 << 4,
    LSM6DSL_GYRO_RATE_26            = 0x02 << 4,
    LSM6DSL_GYRO_RATE_52            = 0x03 << 4,
    LSM6DSL_GYRO_RATE_104           = 0x04 << 4,
    LSM6DSL_GYRO_RATE_208           = 0x05 << 4,
    LSM6DSL_GYRO_RATE_416           = 0x06 << 4,
    LSM6DSL_GYRO_RATE_833           = 0x07 << 4,
    LSM6DSL_GYRO_RATE_1660          = 0x08 << 4,
    LSM6DSL_GYRO_RATE_3330          = 0x09 << 4,
    LSM6DSL_GYRO_RATE_6660          = 0x0A << 4,
};

#define LSM6DSL_WHO_AM_I_VAL 0x6A

struct lsm6dsl_cfg {
    enum lsm6dsl_accel_range accel_range;
    enum lsm6dsl_accel_rate accel_rate;
    enum lsm6dsl_gyro_range gyro_range;
    enum lsm6dsl_gyro_rate gyro_rate;
    uint8_t int_enable;
    uint8_t lpf_cfg;     /* See LSM6DSL_CTRL8_XL control reg in datasheet */
    sensor_type_t mask;
};

struct os_mutex;

struct lsm6dsl {
    struct os_dev dev;
    struct sensor sensor;
    struct os_mutex *bus_mutex;
    struct lsm6dsl_cfg cfg;
    os_time_t last_read_time;
};

int lsm6dsl_reset(struct lsm6dsl *dev);
int lsm6dsl_sleep(struct lsm6dsl *dev);
int lsm6dsl_set_lpf(struct lsm6dsl *dev, uint8_t cfg);
int lsm6dsl_get_lpf(struct lsm6dsl *dev, uint8_t *cfg);
int lsm6dsl_set_gyro_rate_range(struct lsm6dsl *dev,
    enum lsm6dsl_gyro_rate rate , enum lsm6dsl_gyro_range range);
int lsm6dsl_get_gyro_rate_range(struct lsm6dsl *dev,
    enum lsm6dsl_gyro_rate *rate, enum lsm6dsl_gyro_range *range);
int lsm6dsl_set_accel_rate_range(struct lsm6dsl *dev,
    enum lsm6dsl_accel_rate rate , enum lsm6dsl_accel_range range);
int lsm6dsl_get_accel_rate_range(struct lsm6dsl *dev,
    enum lsm6dsl_accel_rate *rate, enum lsm6dsl_accel_range *range);

int lsm6dsl_enable_interrupt(struct lsm6dsl *dev, uint8_t enable);

int lsm6dsl_init(struct os_dev *, void *);
int lsm6dsl_config(struct lsm6dsl *, struct lsm6dsl_cfg *);

int lsm6dsl_read_raw(struct lsm6dsl *dev, int16_t gyro[], int16_t acc[]);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_LSM6DSL_H__ */
