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

#if MYNEWT_VAL(LIS2MDL_USE_SPI)
#include "hal/hal_spi.h"
#include "hal/hal_gpio.h"
#else
#include "hal/hal_i2c.h"
#endif

#include "sensor/sensor.h"
#include "sensor/mag.h"
#include "lis2mdl/lis2mdl.h"
#include "lis2mdl_priv.h"
#include "log/log.h"
#include <stats/stats.h>

STATS_SECT_START(lis2mdl_stats)
    STATS_SECT_ENTRY(read_errors)
    STATS_SECT_ENTRY(write_errors)
    STATS_SECT_ENTRY(mutex_errors)
STATS_SECT_END

/* Global variable used to hold stats data */
STATS_SECT_DECL(lis2mdl_stats) g_lis2mdl_stats;

/* Define the stats section and records */
STATS_NAME_START(lis2mdl_stats)
    STATS_NAME(lis2mdl_stats, read_errors)
    STATS_NAME(lis2mdl_stats, write_errors)
    STATS_NAME(lis2mdl_stats, mutex_errors)
STATS_NAME_END(lis2mdl_stats)


#define LOG_MODULE_LIS2MDL    (81)
#define LIS2MDL_INFO(...)     LOG_INFO(&_log, LOG_MODULE_LIS2MDL, __VA_ARGS__)
#define LIS2MDL_ERR(...)      LOG_ERROR(&_log, LOG_MODULE_LIS2MDL, __VA_ARGS__)
static struct log _log;

/* Exports for the sensor API */
static int lis2mdl_sensor_read(struct sensor *, sensor_type_t,
        sensor_data_func_t, void *, uint32_t);
static int lis2mdl_sensor_get_config(struct sensor *, sensor_type_t,
        struct sensor_cfg *);

static const struct sensor_driver g_lis2mdl_sensor_driver = {
    lis2mdl_sensor_read,
    lis2mdl_sensor_get_config
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
lis2mdl_write8(struct lis2mdl *dev, uint8_t reg, uint32_t value)
{
    int rc;
    os_error_t err = 0;
    struct sensor_itf *itf = &dev->sensor.s_itf;

    if (dev->bus_mutex)
    {
        err = os_mutex_pend(dev->bus_mutex, OS_WAIT_FOREVER);
        if (err != OS_OK)
        {
            LIS2MDL_ERR("Mutex error=%d\n", err);
            STATS_INC(g_lis2mdl_stats, mutex_errors);
            return err;
        }
    }

#if MYNEWT_VAL(LIS2MDL_USE_SPI)
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
        LIS2MDL_ERR("Failed to write to 0x%02X:0x%02X with value 0x%02X\n",
                       itf->si_addr, reg, value);
        STATS_INC(g_lis2mdl_stats, write_errors);
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
lis2mdl_read8(struct lis2mdl *dev, uint8_t reg, uint8_t *value)
{
    int rc;
    os_error_t err = 0;
    struct sensor_itf *itf = &dev->sensor.s_itf;

    if (dev->bus_mutex)
    {
        err = os_mutex_pend(dev->bus_mutex, OS_WAIT_FOREVER);
        if (err != OS_OK)
        {
            LIS2MDL_ERR("Mutex error=%d\n", err);
            STATS_INC(g_lis2mdl_stats, mutex_errors);
            return err;
        }
    }

#if MYNEWT_VAL(LIS2MDL_USE_SPI)

    rc=0;
    hal_gpio_write(itf->si_cs_pin, 0);

    hal_spi_tx_val(itf->si_num, reg | 0x80);

    /* Reconfig spi for reading from 3wire */
    dev->spi_read_cb(1);
    *value = hal_spi_tx_val(itf->si_num, 0xff);
    dev->spi_read_cb(0);

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
        LIS2MDL_ERR("I2C access failed at address 0x%02X\n", itf->si_addr);
        STATS_INC(g_lis2mdl_stats, write_errors);
        goto exit;
    }

    /* Read one byte back */
    data_struct.buffer = value;
    rc = hal_i2c_master_read(itf->si_num, &data_struct,
                             OS_TICKS_PER_SEC / 10, 1);

    if (rc) {
         LIS2MDL_ERR("Failed to read from 0x%02X:0x%02X\n", itf->si_addr, reg);
         STATS_INC(g_lis2mdl_stats, read_errors);
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
lis2mdl_read_bytes(struct lis2mdl *dev, uint8_t reg, uint8_t *buffer, uint32_t length)
{
    int rc;
    os_error_t err = 0;
    struct sensor_itf *itf = &dev->sensor.s_itf;

    if (dev->bus_mutex)
    {
        err = os_mutex_pend(dev->bus_mutex, OS_WAIT_FOREVER);
        if (err != OS_OK)
        {
            LIS2MDL_ERR("Mutex error=%d\n", err);
            STATS_INC(g_lis2mdl_stats, mutex_errors);
            return err;
        }
    }

#if MYNEWT_VAL(LIS2MDL_USE_SPI)
    int i;
    rc=0;
    hal_gpio_write(itf->si_cs_pin, 0);

    hal_spi_tx_val(itf->si_num, reg | 0x80);
    dev->spi_read_cb(1);
    for (i=0;i<length;i++) {
        buffer[i] = hal_spi_tx_val(itf->si_num, 0xff);
    }
    dev->spi_read_cb(0);

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
        LIS2MDL_ERR("I2C access failed at address 0x%02X\n", itf->si_addr);
        STATS_INC(g_lis2mdl_stats, write_errors);
        goto exit;
    }

    /* Read n bytes back */
    data_struct.len = length;
    data_struct.buffer = buffer;
    rc = hal_i2c_master_read(itf->si_num, &data_struct,
                             OS_TICKS_PER_SEC / 10, 1);

    if (rc) {
        LIS2MDL_ERR("Failed to read from 0x%02X:0x%02X\n", itf->si_addr, reg);
        STATS_INC(g_lis2mdl_stats, read_errors);
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
lis2mdl_reset(struct lis2mdl *dev)
{
    int rc;
    uint8_t reg;
    lis2mdl_read8(dev, LIS2MDL_CFG_REG_A, &reg);
    // Reset
    rc = lis2mdl_write8(dev, LIS2MDL_CFG_REG_A, reg | 0x20);
    if (rc) {
        return rc;
    }
    os_cputime_delay_usecs(1000);
    // Boot
    rc = lis2mdl_write8(dev, LIS2MDL_CFG_REG_A, reg | 0x40);
    return rc;
}

int
lis2mdl_sleep(struct lis2mdl *dev)
{
    uint8_t reg;
    lis2mdl_read8(dev, LIS2MDL_CFG_REG_A, &reg);
    // Reset
    return lis2mdl_write8(dev, LIS2MDL_CFG_REG_A, reg | 0x20);
}

int
lis2mdl_set_output_rate(struct lis2mdl *dev, enum lis2mdl_output_rate rate)
{
    int rc;
    uint8_t reg;
    // 0x80 - temperature compensation, continuous mode (bits 0:1 == 00)
    rc = lis2mdl_read8(dev, LIS2MDL_CFG_REG_A, &reg);
    if (rc) {
        return rc;
    }

    reg = (reg & (~0xC)) | ((uint8_t)rate << 2);
    return lis2mdl_write8(dev, LIS2MDL_CFG_REG_A, reg);
}

int
lis2mdl_get_output_rate(struct lis2mdl *dev, enum lis2mdl_output_rate *rate)
{
    int rc;
    uint8_t reg;

    rc = lis2mdl_read8(dev, LIS2MDL_CFG_REG_A, &reg);
    if (rc) {
        return rc;
    }

    *rate  = (enum lis2mdl_output_rate)((reg>>2)  & 0x03);

    return 0;
}


int
lis2mdl_enable_interrupt(struct lis2mdl *dev, uint8_t enable)
{
    int rc;
    uint8_t reg;
    // 0x80 - temperature compensation, continuous mode (bits 0:1 == 00)
    rc = lis2mdl_read8(dev, LIS2MDL_CFG_REG_C, &reg);
    if (rc) {
        return rc;
    }

    if (enable)
        reg |= 0x01;
    else
        reg &= ~(0x01);

    return lis2mdl_write8(dev, LIS2MDL_CFG_REG_A, reg);
}


int
lis2mdl_set_lpf(struct lis2mdl *dev, uint8_t enable)
{
    int rc;
    uint8_t reg;

    rc = lis2mdl_read8(dev, LIS2MDL_CFG_REG_B, &reg);
    if (rc) {
        return rc;
    }

    if (enable)
        reg |= 0x01;
    else
        reg &= ~(0x01);

    return lis2mdl_write8(dev, LIS2MDL_CFG_REG_B, reg);
}

int
lis2mdl_get_lpf(struct lis2mdl *dev, uint8_t *cfg)
{
    return lis2mdl_read8(dev, LIS2MDL_CFG_REG_B, cfg);
}

static int lis2mdl_suspend(struct os_dev *dev, os_time_t suspend_t , int force)
{
    struct lis2mdl *lis;
    lis = (struct lis2mdl *) dev;
    lis2mdl_sleep(lis);
    LIS2MDL_INFO("Lis suspend\n");
    return OS_OK;
}

static int lis2mdl_resume(struct os_dev *dev)
{
    struct lis2mdl *lis = (struct lis2mdl*)dev;
    lis2mdl_reset(lis);

    LIS2MDL_INFO("Lis resume\n");
    return lis2mdl_config(lis, &lis->cfg);
}

/**
 * Expects to be called back through os_dev_create().
 *
 * @param The device object associated with this accellerometer
 * @param Argument passed to OS device init
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lis2mdl_init(struct os_dev *dev, void *arg)
{
    struct lis2mdl *lis;
    struct sensor *sensor;
    int rc;

    if (!arg || !dev) {
        return SYS_ENODEV;
    }

    lis = (struct lis2mdl *) dev;

    lis->cfg.mask = SENSOR_TYPE_ALL;

    log_register("lis2mdl", &_log, &log_console_handler, NULL, LOG_SYSLEVEL);

    sensor = &lis->sensor;

    rc = sensor_init(sensor, dev);
    if (rc) {
        return rc;
    }

    /* Add the magnetometer driver */
    rc = sensor_set_driver(sensor, SENSOR_TYPE_MAGNETIC_FIELD,
         (struct sensor_driver *) &g_lis2mdl_sensor_driver);
    if (rc) {
        return rc;
    }

    rc = sensor_set_interface(sensor, arg);
    if (rc) {
        return rc;
    }

    dev->od_handlers.od_suspend = lis2mdl_suspend;
    dev->od_handlers.od_resume = lis2mdl_resume;

    return sensor_mgr_register(sensor);
}

int
lis2mdl_config(struct lis2mdl *lis, struct lis2mdl_cfg *cfg)
{
    int rc;
    uint8_t val;

    /* Init stats */
    rc = stats_init_and_reg(
        STATS_HDR(g_lis2mdl_stats), STATS_SIZE_INIT_PARMS(g_lis2mdl_stats,
        STATS_SIZE_32), STATS_NAME_INIT_PARMS(lis2mdl_stats), "sen_lis2mdl");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = lis2mdl_read8(lis, LIS2MDL_WHO_AM_I, &val);
    if (rc) {
        return rc;
    }
    if (val != LIS2MDL_WHO_AM_I_VAL) {
        return SYS_EINVAL;
    }

    rc = lis2mdl_set_output_rate(lis, cfg->output_rate);
    if (rc) {
        return rc;
    }
    lis->cfg.output_rate = lis->cfg.output_rate;

    rc = lis2mdl_set_lpf(lis, cfg->lpf_enable);
    if (rc) {
        return rc;
    }
    lis->cfg.lpf_enable = lis->cfg.lpf_enable;

	// enable block data read (bit 4 == 1)
    rc = lis2mdl_write8(lis, LIS2MDL_CFG_REG_C, 0x10);
    if (rc) {
        return rc;
    }

    rc = lis2mdl_enable_interrupt(lis, cfg->int_enable);
    if (rc) {
        return rc;
    }
    lis->cfg.int_enable = lis->cfg.int_enable;

    rc = sensor_set_type_mask(&(lis->sensor), cfg->mask);
    if (rc) {
        return rc;
    }

    lis->cfg.mask = cfg->mask;

    return 0;
}

int
lis2mdl_read_raw(struct lis2mdl *dev, int16_t val[])
{
    int rc;
    int16_t x, y, z;
    uint8_t payload[8];
    rc = lis2mdl_read_bytes(dev, LIS2MDL_OUTX_L_REG, payload, 8);
    if (rc) {
        return rc;
    }
    x = (((int16_t)payload[1] << 8) | payload[0]);
    y = (((int16_t)payload[3] << 8) | payload[2]);
    z = (((int16_t)payload[5] << 8) | payload[4]);

    if (val)
    {
        val[0] = x;
        val[1] = y;
        val[2] = z;
    }
    return 0;
}

static int
lis2mdl_sensor_read(struct sensor *sensor, sensor_type_t type,
        sensor_data_func_t data_func, void *data_arg, uint32_t timeout)
{
    (void)timeout;
    int rc;
    int16_t x, y, z;
    uint8_t payload[8];
    union {
        struct sensor_mag_data smd;
    } databuf;

    /* If the read isn't looking for accel or gyro, don't do anything. */
    if (!(type & SENSOR_TYPE_MAGNETIC_FIELD)) {
        return SYS_EINVAL;
    }

    struct lis2mdl *dev = (struct lis2mdl *)SENSOR_GET_DEVICE(sensor);

    /* Get a new accelerometer sample */
    if (type & SENSOR_TYPE_MAGNETIC_FIELD) {
        rc = lis2mdl_read_bytes(dev, LIS2MDL_OUTX_L_REG, payload, 8);
        if (rc) {
            return rc;
        }
        x = (((int16_t)payload[1] << 8) | payload[0]);
        y = (((int16_t)payload[3] << 8) | payload[2]);
        z = (((int16_t)payload[5] << 8) | payload[4]);

        /* Data is already in mG (same as 10*uT) */
        databuf.smd.smd_x = x;
        databuf.smd.smd_x_is_valid = 1;
        databuf.smd.smd_y = y;
        databuf.smd.smd_y_is_valid = 1;
        databuf.smd.smd_z = z;
        databuf.smd.smd_z_is_valid = 1;

        rc = data_func(sensor, data_arg, &databuf.smd,
             SENSOR_TYPE_MAGNETIC_FIELD);
        if (rc) {
            return rc;
        }
    }

    return 0;
}

static int
lis2mdl_sensor_get_config(struct sensor *sensor, sensor_type_t type,
        struct sensor_cfg *cfg)
{
    /* If the read isn't looking for accel or gyro, don't do anything. */
    if (!(type & SENSOR_TYPE_MAGNETIC_FIELD)) {
        return SYS_EINVAL;
    }

    cfg->sc_valtype = SENSOR_VALUE_TYPE_FLOAT_TRIPLET;

    return 0;
}
