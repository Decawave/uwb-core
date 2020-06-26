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

#ifndef __SENSOR_LPS22HB_H__
#define __SENSOR_LPS22HB_H__

#include "os/os.h"
#include "os/os_dev.h"
#include "sensor/sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

enum lps22hb_output_rate {
    LPS22HB_OUTPUT_RATE_ONESHOT = 0x00 << 4,
    LPS22HB_OUTPUT_RATE_1       = 0x01 << 4,
    LPS22HB_OUTPUT_RATE_10      = 0x02 << 4,
    LPS22HB_OUTPUT_RATE_25      = 0x03 << 4,
    LPS22HB_OUTPUT_RATE_50      = 0x04 << 4,
    LPS22HB_OUTPUT_RATE_75      = 0x05 << 4,
};

enum lps22hb_lpf_config {
    LPS22HB_LPF_CONFIG_DISABLED = 0x00 << 2,
    LPS22HB_LPF_CONFIG_ODR_9    = 0x10 << 2,
    LPS22HB_LPF_CONFIG_ODR_20   = 0x11 << 2,
};

#define LPS22HB_WHO_AM_I_VAL 0b10110001

struct lps22hb_cfg {
    enum lps22hb_output_rate output_rate;
    enum lps22hb_lpf_config lpf_cfg;
    uint8_t int_enable;
    sensor_type_t mask;
};

struct lps22hb {
    struct os_dev dev;
    struct sensor sensor;
    struct os_mutex *bus_mutex;
    struct lps22hb_cfg cfg;
    os_time_t last_read_time;
};

int lps22hb_reset(struct lps22hb *dev);
int lps22hb_sleep(struct lps22hb *dev);
int lps22hb_set_lpf(struct lps22hb *dev, enum lps22hb_lpf_config cfg);
int lps22hb_get_lpf(struct lps22hb *dev, enum lps22hb_lpf_config *cfg);
int lps22hb_set_output_rate(struct lps22hb *dev, enum lps22hb_output_rate rate);
int lps22hb_get_output_rate(struct lps22hb *dev, enum lps22hb_output_rate *rate);

int lps22hb_enable_interrupt(struct lps22hb *dev, uint8_t enable);

int lps22hb_init(struct os_dev *, void *);
int lps22hb_config(struct lps22hb *, struct lps22hb_cfg *);

int lps22hb_read_raw(struct lps22hb *dev, uint32_t *pressure);


#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_LPS22HB_H__ */
