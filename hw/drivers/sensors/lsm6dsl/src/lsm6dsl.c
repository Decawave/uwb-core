/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * resarding copyright ownership.  The ASF licenses this file
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

#include <string.h>
#include <errno.h>
#include <assert.h>

#include "defs/error.h"
#include "os/os.h"
#include "os/os_mutex.h"
#include "sysinit/sysinit.h"

#if MYNEWT_VAL(LSM6DSL_USE_SPI)
#include "hal/hal_spi.h"
#include "hal/hal_gpio.h"
#else
#include "hal/hal_i2c.h"
#endif

#include "sensor/sensor.h"
#include "sensor/accel.h"
#include "sensor/gyro.h"
#include "lsm6dsl/lsm6dsl.h"
#include "lsm6dsl_priv.h"
#include "log/log.h"
#include <stats/stats.h>

/* Define the stats section and records */
STATS_SECT_START(lsm6dsl_stats)
    STATS_SECT_ENTRY(read_errors)
    STATS_SECT_ENTRY(write_errors)
    STATS_SECT_ENTRY(mutex_errors)
STATS_SECT_END

/* Global variable used to hold stats data */
STATS_SECT_DECL(lsm6dsl_stats) g_lsm6dsl_stats;

/* Define the stats section and records */
STATS_NAME_START(lsm6dsl_stats)
    STATS_NAME(lsm6dsl_stats, read_errors)
    STATS_NAME(lsm6dsl_stats, write_errors)
    STATS_NAME(lsm6dsl_stats, mutex_errors)
STATS_NAME_END(lsm6dsl_stats)

#define LOG_MODULE_LSM6DSL    (80)
#define LSM6DSL_INFO(...)     LOG_INFO(&_log, LOG_MODULE_LSM6DSL, __VA_ARGS__)
#define LSM6DSL_ERR(...)      LOG_ERROR(&_log, LOG_MODULE_LSM6DSL, __VA_ARGS__)
static struct log _log;

/* Exports for the sensor API */
static int lsm6dsl_sensor_read(struct sensor *, sensor_type_t,
        sensor_data_func_t, void *, uint32_t);
static int lsm6dsl_sensor_get_config(struct sensor *, sensor_type_t,
        struct sensor_cfg *);

static const struct sensor_driver g_lsm6dsl_sensor_driver = {
    lsm6dsl_sensor_read,
    lsm6dsl_sensor_get_config
};

/**
 * Writes a single byte to the specified register
 *
 * @param The sensor interface
 * @param The register address to write to
 * @param The value to write
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm6dsl_write8(struct lsm6dsl *dev, uint8_t reg, uint32_t value)
{
    int rc;
    os_error_t err = 0;
    struct sensor_itf *itf = &dev->sensor.s_itf;

    if (dev->bus_mutex)
    {
        err = os_mutex_pend(dev->bus_mutex, OS_WAIT_FOREVER);
        if (err != OS_OK)
        {
            LSM6DSL_ERR("Mutex error=%d\n", err);
            STATS_INC(g_lsm6dsl_stats, mutex_errors);
            return err;
        }
    }

#if MYNEWT_VAL(LSM6DSL_USE_SPI)
    rc=0;
    hal_gpio_write(itf->si_cs_pin, 0);

    hal_spi_tx_val(itf->si_num, reg);
    hal_spi_tx_val(itf->si_num, value);

    hal_gpio_write(itf->si_cs_pin, 1);

#else
    uint8_t payload[2] = { reg, value & 0xFF };
    struct hal_i2c_master_data data_struct = {
        .address = itf->si_addr,
        .len = 2,
        .buffer = payload
    };

    rc = hal_i2c_master_write(itf->si_num, &data_struct,
                              OS_TICKS_PER_SEC / 10, 1);

    if (rc) {
        LSM6DSL_ERR("Failed to write to 0x%02X:0x%02X with value 0x%02X\n",
                    itf->si_addr, reg, value);
        STATS_INC(g_lsm6dsl_stats, write_errors);
    }

#endif
    if (dev->bus_mutex)
    {
        err = os_mutex_release(dev->bus_mutex);
        assert(err == OS_OK);
    }

    return rc;
}

/**
 * Reads a single byte from the specified register
 *
 * @param The sensor interface
 * @param The register address to read from
 * @param Pointer to where the register value should be written
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm6dsl_read8(struct lsm6dsl *dev, uint8_t reg, uint8_t *value)
{
    int rc;
    os_error_t err = 0;
    struct sensor_itf *itf = &dev->sensor.s_itf;

    if (dev->bus_mutex)
    {
        err = os_mutex_pend(dev->bus_mutex, OS_WAIT_FOREVER);
        if (err != OS_OK)
        {
            LSM6DSL_ERR("Mutex error=%d\n", err);
            STATS_INC(g_lsm6dsl_stats, mutex_errors);
            return err;
        }
    }

#if MYNEWT_VAL(LSM6DSL_USE_SPI)
    rc=0;
    hal_gpio_write(itf->si_cs_pin, 0);

    hal_spi_tx_val(itf->si_num, reg | 0x80);
    *value = hal_spi_tx_val(itf->si_num, 0);

    hal_gpio_write(itf->si_cs_pin, 1);

#else
    struct hal_i2c_master_data data_struct = {
        .address = itf->si_addr,
        .len = 1,
        .buffer = &reg
    };


    /* Register write */
    rc = hal_i2c_master_write(itf->si_num, &data_struct,
                              OS_TICKS_PER_SEC / 10, 0);
    if (rc) {
        LSM6DSL_ERR("I2C access failed at address 0x%02X\n", itf->si_addr);
        STATS_INC(g_lsm6dsl_stats, write_errors);
        goto exit;
    }

    /* Read one byte back */
    data_struct.buffer = value;
    rc = hal_i2c_master_read(itf->si_num, &data_struct,
                             OS_TICKS_PER_SEC / 10, 1);

    if (rc) {
        LSM6DSL_ERR("Failed to read from 0x%02X:0x%02X\n", itf->si_addr, reg);
        STATS_INC(g_lsm6dsl_stats, read_errors);
    }
exit:
#endif

    if (dev->bus_mutex)
    {
        err = os_mutex_release(dev->bus_mutex);
        assert(err == OS_OK);
    }

    return rc;
}

/**
 * Reads n bytes from the specified register
 *
 * @param The sensor interface
 * @param The register address to read from
 * @param Pointer to where the register value should be written
 * @param number of bytes to read
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm6dsl_read_bytes(struct lsm6dsl *dev, uint8_t reg, uint8_t *buffer, uint32_t length)
{
    int rc;
    os_error_t err = 0;
    struct sensor_itf *itf = &dev->sensor.s_itf;

    if (dev->bus_mutex)
    {
        err = os_mutex_pend(dev->bus_mutex, OS_WAIT_FOREVER);
        if (err != OS_OK)
        {
            LSM6DSL_ERR("Mutex error=%d\n", err);
            STATS_INC(g_lsm6dsl_stats, mutex_errors);
            return err;
        }
    }

#if MYNEWT_VAL(LSM6DSL_USE_SPI)
    int i;
    rc=0;
    hal_gpio_write(itf->si_cs_pin, 0);

    hal_spi_tx_val(itf->si_num, reg | 0x80);
    for (i=0;i<length;i++) {
        buffer[i] = hal_spi_tx_val(itf->si_num, 0x00);
    }

    hal_gpio_write(itf->si_cs_pin, 1);

#else
    struct hal_i2c_master_data data_struct = {
        .address = itf->si_addr,
        .len = 1,
        .buffer = &reg
    };

    /* Register write */
    rc = hal_i2c_master_write(itf->si_num, &data_struct,
                              OS_TICKS_PER_SEC / 10, 0);
    if (rc) {
        LSM6DSL_ERR("I2C access failed at address 0x%02X\n", itf->si_addr);
        STATS_INC(g_lsm6dsl_stats, write_errors);
        goto exit;
    }

    /* Read n bytes back */
    data_struct.len = length;
    data_struct.buffer = buffer;
    rc = hal_i2c_master_read(itf->si_num, &data_struct,
                             OS_TICKS_PER_SEC / 10, 1);

    if (rc) {
        LSM6DSL_ERR("Failed to read from 0x%02X:0x%02X\n", itf->si_addr, reg);
        STATS_INC(g_lsm6dsl_stats, read_errors);
    }
exit:
#endif
    if (dev->bus_mutex)
    {
        err = os_mutex_release(dev->bus_mutex);
        assert(err == OS_OK);
    }

    return rc;
}

int
lsm6dsl_reset(struct lsm6dsl *dev)
{
    uint8_t temp;
    lsm6dsl_read8(dev, LSM6DSL_CTRL3_C, &temp);
    temp |= 0x01;
    int rc = lsm6dsl_write8(dev, LSM6DSL_CTRL3_C, temp);
    os_cputime_delay_usecs(10000);  // Wait for all registers to reset
    return rc;
}

int
lsm6dsl_sleep(struct lsm6dsl *dev)
{
    int rc;

    rc = lsm6dsl_write8(dev, LSM6DSL_CTRL1_XL, LSM6DSL_ACCEL_RATE_POWER_DOWN);
    if (rc) {
        return rc;
    }

    rc = lsm6dsl_write8(dev, LSM6DSL_CTRL2_G, LSM6DSL_GYRO_RATE_POWER_DOWN);
    return rc;
}

static int
lsm6dsl_suspend(struct os_dev *dev, os_time_t suspend_t , int force)
{
    struct lsm6dsl *lsm;
    lsm = (struct lsm6dsl *) dev;
    lsm6dsl_sleep(lsm);
    LSM6DSL_INFO("lsm suspend\n");
    return OS_OK;
}

static int
lsm6dsl_resume(struct os_dev *dev)
{
    struct lsm6dsl *lsm = (struct lsm6dsl*)dev;
    lsm6dsl_reset(lsm);

    LSM6DSL_INFO("lsm resumed\n");
    return lsm6dsl_config(lsm, &lsm->cfg);
}


int
lsm6dsl_set_gyro_rate_range(struct lsm6dsl *dev, enum lsm6dsl_gyro_rate rate , enum lsm6dsl_gyro_range range)
{
    uint8_t val = (uint8_t)range | (uint8_t)rate;
    return lsm6dsl_write8(dev, LSM6DSL_CTRL2_G, val);
}

int
lsm6dsl_get_gyro_rate_range(struct lsm6dsl *dev, enum lsm6dsl_gyro_rate *rate, enum lsm6dsl_gyro_range *range)
{
    uint8_t reg;
    int rc;

    rc = lsm6dsl_read8(dev, LSM6DSL_CTRL2_G, &reg);
    if (rc) {
        return rc;
    }

    *rate  = (enum lsm6dsl_gyro_rate)(reg  & 0xF0);
    *range = (enum lsm6dsl_gyro_range)(reg & 0x0F);

    return 0;
}

int
lsm6dsl_set_accel_rate_range(struct lsm6dsl *dev, enum lsm6dsl_accel_rate rate , enum lsm6dsl_accel_range range)
{
    uint8_t val = (uint8_t)range | (uint8_t)rate;
    return lsm6dsl_write8(dev, LSM6DSL_CTRL1_XL, val);
}

int
lsm6dsl_get_accel_rate_range(struct lsm6dsl *dev, enum lsm6dsl_accel_rate *rate, enum lsm6dsl_accel_range *range)
{
    uint8_t reg;
    int rc;

    rc = lsm6dsl_read8(dev, LSM6DSL_CTRL1_XL, &reg);
    if (rc) {
        return rc;
    }

    *rate  = (enum lsm6dsl_accel_rate)(reg  & 0xF0);
    *range = (enum lsm6dsl_accel_range)(reg & 0x0F);

    return 0;
}

int
lsm6dsl_enable_interrupt(struct lsm6dsl *dev, uint8_t enable)
{
    int rc;
    rc = lsm6dsl_write8(dev, LSM6DSL_DRDY_PULSE_CFG, 0x80);
    if (rc) {
        return rc;
    }
    return lsm6dsl_write8(dev, LSM6DSL_INT1_CTRL, (enable)? 0x03 : 0x00);
}


int
lsm6dsl_set_lpf(struct lsm6dsl *dev, uint8_t cfg)
{
    return lsm6dsl_write8(dev, LSM6DSL_CTRL8_XL, cfg);
}

int
lsm6dsl_get_lpf(struct lsm6dsl *dev, uint8_t *cfg)
{
    return lsm6dsl_read8(dev, LSM6DSL_CTRL8_XL, cfg);
}


/**
 * Expects to be called back through os_dev_create().
 *
 * @param The device object associated with this accellerometer
 * @param Argument passed to OS device init, unused
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm6dsl_init(struct os_dev *dev, void *arg)
{
    struct lsm6dsl *lsm;
    struct sensor *sensor;
    int rc;

    if (!arg || !dev) {
        return SYS_ENODEV;
    }

    lsm = (struct lsm6dsl *) dev;

    lsm->cfg.mask = SENSOR_TYPE_ALL;

    log_register("lsm6dsl", &_log, &log_console_handler, NULL, LOG_SYSLEVEL);

    sensor = &lsm->sensor;

    rc = sensor_init(sensor, dev);
    if (rc) {
        return rc;
    }

    /* Add the accelerometer/gyroscope driver */
    rc = sensor_set_driver(sensor, SENSOR_TYPE_GYROSCOPE |
         SENSOR_TYPE_ACCELEROMETER,
         (struct sensor_driver *) &g_lsm6dsl_sensor_driver);
    if (rc) {
        return rc;
    }

    rc = sensor_set_interface(sensor, arg);
    if (rc) {
        return rc;
    }

    dev->od_handlers.od_suspend = lsm6dsl_suspend;
    dev->od_handlers.od_resume = lsm6dsl_resume;

    return sensor_mgr_register(sensor);
}

int
lsm6dsl_config(struct lsm6dsl *lsm, struct lsm6dsl_cfg *cfg)
{
    int rc;
    uint8_t val;

    /* Init stats */
    rc = stats_init_and_reg(
        STATS_HDR(g_lsm6dsl_stats), STATS_SIZE_INIT_PARMS(g_lsm6dsl_stats,
        STATS_SIZE_32), STATS_NAME_INIT_PARMS(lsm6dsl_stats), "sen_lsm6dsl");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = lsm6dsl_read8(lsm, LSM6DSL_WHO_AM_I, &val);
    if (rc) {
        return rc;
    }
    if (val != LSM6DSL_WHO_AM_I_VAL) {
        return SYS_EINVAL;
    }

    rc = lsm6dsl_set_lpf(lsm, cfg->lpf_cfg);
    if (rc) {
        return rc;
    }
    lsm->cfg.lpf_cfg = cfg->lpf_cfg;


    rc = lsm6dsl_set_accel_rate_range(lsm, cfg->accel_rate, cfg->accel_range);
    if (rc) {
        return rc;
    }
    lsm6dsl_get_accel_rate_range(lsm, &(lsm->cfg.accel_rate), &(lsm->cfg.accel_range));

    rc = lsm6dsl_set_gyro_rate_range(lsm, cfg->gyro_rate, cfg->gyro_range);
    if (rc) {
        return rc;
    }
    lsm6dsl_get_gyro_rate_range(lsm, &(lsm->cfg.gyro_rate), &(lsm->cfg.gyro_range));

    rc = lsm6dsl_read8(lsm, LSM6DSL_CTRL3_C, &val);
    if (rc) {
        return rc;
    }

    /* enable block update (bit 6 = 1), auto-increment registers (bit 2 = 1) */
    rc = lsm6dsl_write8(lsm, LSM6DSL_CTRL3_C, val| 0x40 | 0x04);
    if (rc) {
        return rc;
    }

    rc = lsm6dsl_enable_interrupt(lsm, cfg->int_enable);
    if (rc) {
        return rc;
    }
    lsm->cfg.int_enable = cfg->int_enable;

    rc = sensor_set_type_mask(&(lsm->sensor), cfg->mask);
    if (rc) {
        return rc;
    }

    lsm->cfg.mask = cfg->mask;

    return 0;
}

int
lsm6dsl_read_raw(struct lsm6dsl *dev, int16_t gyro[], int16_t acc[])
{
    int rc;
    int16_t ax, ay, az, gx, gy, gz;
    uint8_t payload[14];
    rc = lsm6dsl_read_bytes(dev, LSM6DSL_OUT_TEMP_L, payload, 14);
    if (rc) {
        return rc;
    }
    gx = (int16_t)((payload[3] << 8) | payload[2]);
    gy = (int16_t)((payload[5] << 8) | payload[4]);
    gz = (int16_t)((payload[7] << 8) | payload[6]);
    ax = (((int16_t)payload[9] << 8) | payload[8]);
    ay = (((int16_t)payload[11] << 8) | payload[10]);
    az = (((int16_t)payload[13] << 8) | payload[12]);

    if (gyro)
    {
        gyro[0] = gx;
        gyro[1] = gy;
        gyro[2] = gz;
    }
    if (acc)
    {
        acc[0] = ax;
        acc[1] = ay;
        acc[2] = az;
    }
    return 0;
}


static int
lsm6dsl_sensor_read(struct sensor *sensor, sensor_type_t type,
        sensor_data_func_t data_func, void *data_arg, uint32_t timeout)
{
    (void)timeout;
    int rc;
    int16_t x, y, z;
    uint8_t payload[14];
    float lsb;
    struct lsm6dsl *lsm;
    union {
        struct sensor_accel_data sad;
        struct sensor_gyro_data sgd;
    } databuf;

    memset(payload, 0, sizeof(payload));

    /* If the read isn't looking for accel or gyro, don't do anything. */
    if (!(type & SENSOR_TYPE_ACCELEROMETER) &&
       (!(type & SENSOR_TYPE_GYROSCOPE))) {
        return SYS_EINVAL;
    }

    lsm = (struct lsm6dsl *) SENSOR_GET_DEVICE(sensor);

    if (type & (SENSOR_TYPE_ACCELEROMETER|SENSOR_TYPE_GYROSCOPE)) {
#if 1
        rc = lsm6dsl_read_bytes(lsm, LSM6DSL_OUT_TEMP_L, payload, 14);
#else
        for (int i=0;i<14;i++) {
            rc = lsm6dsl_read8(lsm, LSM6DSL_OUT_TEMP_L+i, payload+i);
        }
#endif
        if (rc) {
            return rc;
        }
    }


    /* Get a new accelerometer sample */
    if (type & SENSOR_TYPE_ACCELEROMETER) {
        x = (((int16_t)payload[9] << 8) | payload[8]);
        y = (((int16_t)payload[11] << 8) | payload[10]);
        z = (((int16_t)payload[13] << 8) | payload[12]);

        switch (lsm->cfg.accel_range) {
            case LSM6DSL_ACCEL_RANGE_2: /* +/- 2g - 16384 LSB/g */
            /* Falls through */
            default:
                lsb = 16384.0F;
            break;
            case LSM6DSL_ACCEL_RANGE_4: /* +/- 4g - 8192 LSB/g */
                lsb = 8192.0F;
            break;
            case LSM6DSL_ACCEL_RANGE_8: /* +/- 8g - 4096 LSB/g */
                lsb = 4096.0F;
            break;
            case LSM6DSL_ACCEL_RANGE_16: /* +/- 16g - 2048 LSB/g */
                lsb = 2048.0F;
            break;
        }

        databuf.sad.sad_x = (x / lsb) * STANDARD_ACCEL_GRAVITY;
        databuf.sad.sad_x_is_valid = 1;
        databuf.sad.sad_y = (y / lsb) * STANDARD_ACCEL_GRAVITY;
        databuf.sad.sad_y_is_valid = 1;
        databuf.sad.sad_z = (z / lsb) * STANDARD_ACCEL_GRAVITY;
        databuf.sad.sad_z_is_valid = 1;

        rc = data_func(sensor, data_arg, &databuf.sad,
                SENSOR_TYPE_ACCELEROMETER);
        if (rc) {
            return rc;
        }
    }

    /* Get a new gyroscope sample */
    if (type & SENSOR_TYPE_GYROSCOPE) {
        x = (int16_t)((payload[3] << 8) | payload[2]);
        y = (int16_t)((payload[5] << 8) | payload[4]);
        z = (int16_t)((payload[7] << 8) | payload[6]);

        switch (lsm->cfg.gyro_range) {
            case LSM6DSL_GYRO_RANGE_245: /* +/- 245 Deg/s - 133 LSB/Deg/s */
            /* Falls through */
            default:
                lsb = 131.0F;
            break;
            case LSM6DSL_GYRO_RANGE_500: /* +/- 500 Deg/s - 65.5 LSB/Deg/s */
                lsb = 65.5F;
            break;
            case LSM6DSL_GYRO_RANGE_1000: /* +/- 1000 Deg/s - 32.8 LSB/Deg/s */
                lsb = 32.8F;
            break;
            case LSM6DSL_GYRO_RANGE_2000: /* +/- 2000 Deg/s - 16.4 LSB/Deg/s */
                lsb = 16.4F;
            break;
        }

        databuf.sgd.sgd_x = x / lsb;
        databuf.sgd.sgd_x_is_valid = 1;
        databuf.sgd.sgd_y = y / lsb;
        databuf.sgd.sgd_y_is_valid = 1;
        databuf.sgd.sgd_z = z / lsb;
        databuf.sgd.sgd_z_is_valid = 1;

        rc = data_func(sensor, data_arg, &databuf.sgd, SENSOR_TYPE_GYROSCOPE);
        if (rc) {
            return rc;
        }
    }

    return 0;
}

static int
lsm6dsl_sensor_get_config(struct sensor *sensor, sensor_type_t type,
        struct sensor_cfg *cfg)
{
    /* If the read isn't looking for accel or gyro, don't do anything. */
    if (!(type & SENSOR_TYPE_ACCELEROMETER) &&
       (!(type & SENSOR_TYPE_GYROSCOPE))) {
        return SYS_EINVAL;
    }

    cfg->sc_valtype = SENSOR_VALUE_TYPE_FLOAT_TRIPLET;

    return 0;
}
