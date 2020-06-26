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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#if defined(MYNEWT)
#include "os/mynewt.h"
#include "hal/hal_flash_int.h"
#include "flash_map/flash_map.h"
#include "defs/sections.h"
#endif
#include <bsp/bsp.h>
#include <hal/hal_gpio.h>
#include <hal/hal_spi.h>
#include "native_bsp.h"
#include <uart/uart.h>
#include <uart_hal/uart_hal.h>

#if !defined(MYNEWT)
#include <signal.h>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include "SYSFS.h"
pthread_t thread_1;
#endif

#include <uwb/uwb.h>
static struct uart_dev os_bsp_uart0;
int uwbcore_hal_bsp_dw1000_init(uint32_t devid, struct dpl_sem *sem);
int uwbcore_hal_bsp_dw3000_init(uint32_t devid, struct dpl_sem *sem);

struct dpl_sem g_spi0_sem;

const struct hal_flash *
hal_bsp_flash_dev(uint8_t id)
{
    switch (id) {
    case 0:
        return &native_flash_dev;
    case 1:
        //return &ef_dev0.etd_dev.efd_hal;
    default:
        return NULL;
    }
}

int
hal_bsp_power_state(int state)
{
    return (0);
}

#if !defined(MYNEWT)
uint32_t g_uio_pull_ticks = 0;
void * UIO_poll(void *p)
{
    int E=-1, ret, uio_fd;
    int cnt = 0;

    struct uwb_dev * inst = uwb_dev_idx_lookup(0);
    uio_fd=open("/dev/uio0", O_RDONLY);
    if(uio_fd==-1) {
        perror("open");
        assert(0);
    }
    while(1)
    {
        ret=read(uio_fd,&E,4);
        if (!dpl_event_get_arg(&inst->interrupt_ev)) {
            continue;
        }

        if ( ret < 0) {
            perror("readi:");
        } else {
            dpl_eventq_put(&inst->eventq, &inst->interrupt_ev);
        }
    }
}
#endif


void hal_bsp_init(void)
{
    int rc;
    uint32_t devid = 0;
    uint8_t tx[5] = {0};
    uint8_t rx[5] = {0};
    struct hal_spi_settings spi_c = {
        .data_order = HAL_SPI_MSB_FIRST,
        .data_mode = HAL_SPI_MODE0,
        .baudrate = 500,
        .word_size = HAL_SPI_WORD_SIZE_8BIT
    };

#if MYNEWT_VAL(OS_CPUTIME_TIMER_NUM >= 0)
    rc = hal_timer_init(0, NULL);
    assert(rc == 0);
    rc = os_cputime_init(MYNEWT_VAL(OS_CPUTIME_FREQ));
    assert(rc == 0);
#endif

    rc = os_dev_create((struct os_dev *) &os_bsp_uart0, "uart0",
            OS_DEV_INIT_PRIMARY, 0, uart_hal_init, (void *) NULL);
    assert(rc == 0);

#if MYNEWT_VAL(DW1000_DEVICE_0)
    hal_gpio_write(MYNEWT_VAL(DW1000_DEVICE_0_EN), 1);
    usleep(10000);
    hal_gpio_init_out(MYNEWT_VAL(DW1000_DEVICE_0_RST), 0);
    usleep(10);
    hal_gpio_write(MYNEWT_VAL(DW1000_DEVICE_0_RST), 1);
    hal_gpio_init_in(MYNEWT_VAL(DW1000_DEVICE_0_RST), 0);
#endif
#if MYNEWT_VAL(DW3000_DEVICE_0)
    hal_gpio_write(MYNEWT_VAL(DW3000_DEVICE_0_EN), 1);
    usleep(10000);
    hal_gpio_init_out(MYNEWT_VAL(DW3000_DEVICE_0_RST), 0);
    usleep(10);
    hal_gpio_write(MYNEWT_VAL(DW3000_DEVICE_0_RST), 1);
    hal_gpio_init_in(MYNEWT_VAL(DW3000_DEVICE_0_RST), 0);
#endif

#if MYNEWT_VAL(DW3000_DEVICE_0) || MYNEWT_VAL(DW1000_DEVICE_0)
    rc = dpl_sem_init(&g_spi0_sem, 0x1);
    assert(rc == 0);
    rc = hal_spi_init(0, (void *)0, HAL_SPI_TYPE_MASTER);
    assert(rc == 0);
    rc = hal_spi_config(0, &spi_c);
    usleep(10000);
    rc = hal_spi_txrx(0, tx, rx, 5);
    if (!rc) {
        devid = rx[1] | (uint32_t)rx[2]<<8 | (uint32_t)rx[3]<<16 | (uint32_t)rx[4]<<24;
    }
    printf("probe device: %d, devid: %X\n", rc, devid);

    int num_devices = 0;
#if MYNEWT_VAL(DW1000_DEVICE_0)
    num_devices += uwbcore_hal_bsp_dw1000_init(devid, &g_spi0_sem);
#endif

#if MYNEWT_VAL(DW3000_DEVICE_0)
    num_devices += uwbcore_hal_bsp_dw3000_init(devid, &g_spi0_sem);
#endif

#if !defined(MYNEWT)
    int err, max_prio;
    pthread_attr_t          attr;
    struct sched_param      param;

    /* Start "polling" irq thread */
    err = pthread_attr_init(&attr);
    assert(err==0);

#if !defined(ANDROID_APK_BUILD)
    /* These features should not be used when integrating the library
       in an overall framework as they will lock the memory of the entire
       framework from being swapped out or moved. */
    err = mlockall(MCL_CURRENT|MCL_FUTURE);
    assert(err==0);
    err = pthread_attr_getschedparam (&attr, &param);
    assert(err==0);
    err = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    assert(err==0);
    max_prio = sched_get_priority_max(SCHED_FIFO);
    param.sched_priority = max_prio - 10;
    err = pthread_attr_setschedparam (&attr, &param);
    assert(err==0);
#endif

    pthread_create(&thread_1, &attr, UIO_poll, NULL);
#endif
#endif
}

extern int hal_spi_deinit(int spi_num);

#if !defined(MYNEWT)
void
handle_sigint(int sig)
{
    printf("Caught signal %d - shutting down\n", sig);
    dpl_sem_pend(&g_spi0_sem, DPL_TIMEOUT_NEVER);
    hal_spi_deinit(0);
    hal_gpio_write(MYNEWT_VAL(DW3000_DEVICE_0_EN), 0);
    exit(0);
}

void __attribute__((weak)) os_pkg_init(void)
{
#if !defined(ANDROID_APK_BUILD)
    signal(SIGINT, handle_sigint);
#endif

    /* Replacement for calling hal_bsp_init at reset */
    hal_bsp_init();
}
#endif
