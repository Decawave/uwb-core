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

#ifndef __SENSOR_LIS2MDL_H__
#define __SENSOR_LIS2MDL_H__

#include "os/os.h"
#include "os/os_dev.h"
#include "sensor/sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

enum lis2mdl_output_rate {
    LIS2MDL_OUTPUT_RATE_10     = 0x00,
    LIS2MDL_OUTPUT_RATE_20     = 0x01,
    LIS2MDL_OUTPUT_RATE_50     = 0x02,
    LIS2MDL_OUTPUT_RATE_100    = 0x03, /* 100 Hz */
};

#define LIS2MDL_WHO_AM_I_VAL 0x40

struct lis2mdl_cfg {
    enum lis2mdl_output_rate output_rate;
    uint8_t int_enable;
    uint8_t lpf_enable;
    sensor_type_t mask;
};

struct os_mutex;
struct nrf52_hal_spi_cfg;

struct lis2mdl {
    struct os_dev dev;
    struct sensor sensor;
    struct os_mutex *bus_mutex;
    struct lis2mdl_cfg cfg;
    os_time_t last_read_time;
#if MYNEWT_VAL(LIS2MDL_USE_SPI)
    void (*spi_read_cb)(int en);
#endif
};

int lis2mdl_reset(struct lis2mdl *dev);
int lis2mdl_sleep(struct lis2mdl *dev);
int lis2mdl_set_lpf(struct lis2mdl *dev, uint8_t cfg);
int lis2mdl_get_lpf(struct lis2mdl *dev, uint8_t *cfg);
int lis2mdl_set_output_rate(struct lis2mdl *dev, enum lis2mdl_output_rate rate);
int lis2mdl_get_output_rate(struct lis2mdl *dev, enum lis2mdl_output_rate *rate);

int lis2mdl_enable_interrupt(struct lis2mdl *dev, uint8_t enable);

int lis2mdl_init(struct os_dev *, void *);
int lis2mdl_config(struct lis2mdl *, struct lis2mdl_cfg *);

int lis2mdl_read_raw(struct lis2mdl *dev, int16_t val[]);


#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_LIS2MDL_H__ */
