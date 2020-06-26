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


#include <math.h>
#include <assert.h>
#include <stdio.h>

#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#ifdef __KERNEL__
#include <linux/slab.h>
#define slog(fmt, ...) \
    pr_info("uwbcore:%s():%d: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#include <stdlib.h>
#endif
struct uwb_dev*
uwb_dev_idx_lookup(int idx)
{
    const char base1k[] = "dw1000_%d";
    const char base3k[] = "dw3000_%d";
    char buf[sizeof(base3k) + 2];
    struct os_dev *odev;
    snprintf(buf, sizeof buf, base1k, idx);
    odev = os_dev_lookup(buf);
    if (!odev) {
        snprintf(buf, sizeof buf, base3k, idx);
        odev = os_dev_lookup(buf);
    }

    return (struct uwb_dev*)odev;
}
EXPORT_SYMBOL(uwb_dev_idx_lookup);

/**
 * API to register extension  callbacks for different services.
 *
 * @param dev  Pointer to struct uwb_dev
 * @param callbacks  callback instance.
 * @return void
 */
struct uwb_mac_interface *
uwb_mac_append_interface(struct uwb_dev* dev, struct uwb_mac_interface * cbs)
{
    assert(dev);
    assert(cbs);
    cbs->status.initialized = true;

    if(!(SLIST_EMPTY(&dev->interface_cbs))) {
        struct uwb_mac_interface * prev_cbs = NULL;
        struct uwb_mac_interface * cur_cbs = NULL;
        SLIST_FOREACH(cur_cbs, &dev->interface_cbs, next){
            prev_cbs = cur_cbs;
        }
        SLIST_INSERT_AFTER(prev_cbs, cbs, next);
    } else {
        SLIST_INSERT_HEAD(&dev->interface_cbs, cbs, next);
    }

    return cbs;
}
EXPORT_SYMBOL(uwb_mac_append_interface);

/**
 * API to remove specified callbacks.
 *
 * @param dev  Pointer to struct uwb_dev
 * @param id    ID of the service.
 * @return void
 */
void
uwb_mac_remove_interface(struct uwb_dev* dev, uwb_extension_id_t id)
{
    struct uwb_mac_interface * cbs = NULL;
    assert(dev);
    SLIST_FOREACH(cbs, &dev->interface_cbs, next){
        if(cbs->id == id){
            SLIST_REMOVE(&dev->interface_cbs, cbs, uwb_mac_interface, next);
            break;
        }
    }
}
EXPORT_SYMBOL(uwb_mac_remove_interface);

/**
 * API to return specified callbacks.
 *
 * @param dev  Pointer to struct uwb_dev
 * @param id    ID of the service.
 * @return struct uwb_mac_interface * cbs
 */
struct uwb_mac_interface *
uwb_mac_get_interface(struct uwb_dev* dev, uwb_extension_id_t id)
{
    struct uwb_mac_interface * cbs = NULL;
    assert(dev);
    SLIST_FOREACH(cbs, &dev->interface_cbs, next){
        if(cbs->id == id){
            break;
        }
    }
    return cbs;
}
EXPORT_SYMBOL(uwb_mac_get_interface);

/**
 *  Finds the first instance pointer to the callback structure
 *  setup with a specific id.
 *
 * @param dev      Pointer to struct uwb_dev
 * @param id       Corresponding id to find (UWBEXT_CCP,...)
 * @return void pointer to instance, null otherwise
 */
void*
uwb_mac_find_cb_inst_ptr(struct uwb_dev *dev, uint16_t id)
{
    struct uwb_mac_interface * cbs = uwb_mac_get_interface(dev, id);
    if (cbs) {
        return cbs->inst_ptr;
    }
    return 0;
}
EXPORT_SYMBOL(uwb_mac_find_cb_inst_ptr);

#ifndef __KERNEL__
/**
 * API to execute each of the interrupt in queue.
 *
 * @param arg  Pointer to the queue of interrupts.
 * @return void
 */
static void *
uwb_interrupt_task(void *arg)
{
    struct uwb_dev * inst = (struct uwb_dev *)arg;
    while (1) {
        dpl_eventq_run(&inst->eventq);
    }
    return NULL;
}
#endif

/**
 * The UWB processing of interrupts in a task context instead of the interrupt context such that other interrupts
 * and high priority tasks are not blocked waiting for the interrupt handler to complete processing.
 * This uwb softstack needs to coexists with other stacks and sensors interfaces.
 *
 * @param inst      Pointer to struct uwb_dev.
 * @param irq_ev_cb Pointer to function processing interrupt
 *
 * @return void
 */
void
uwb_task_init(struct uwb_dev * inst, void (*irq_ev_cb)(struct dpl_event*))
{
    int rc;
    /* Check if the task is already initiated */
    if (!dpl_eventq_inited(&inst->eventq))
    {
        /* Use a dedicate event queue for timer and interrupt events */
        dpl_eventq_init(&inst->eventq);
        /*
         * Create the task to process timer and interrupt events from the
         * my_timer_interrupt_eventq event queue.
         */
        dpl_event_init(&inst->interrupt_ev, irq_ev_cb, (void *)inst);

        /* IRQ / ISR Semaphore init */
        rc = dpl_sem_init(&inst->irq_sem, 0x1);
        assert(rc == DPL_OK);

#ifndef __KERNEL__
        dpl_task_init(&inst->task_str, "uwb_irq",
                      uwb_interrupt_task,
                      (void *) inst,
                      inst->task_prio, DPL_WAIT_FOREVER,
                      inst->task_stack,
                      MYNEWT_VAL(UWB_DEV_TASK_STACK_SZ));
#endif
    }
}

void
uwb_task_deinit(struct uwb_dev * inst)
{
    if (dpl_eventq_inited(&inst->eventq))
    {
        dpl_task_remove(&inst->task_str);
        dpl_eventq_deinit(&inst->eventq);
    }
}

void
uwb_dev_init(struct uwb_dev * inst)
{
    if (!inst->txbuf) {
#ifdef __KERNEL__
        inst->txbuf = kmalloc(inst->txbuf_size, GFP_DMA|GFP_KERNEL);
        if (!inst->txbuf) {
            printk("ERROR, can't allocate txbuf\n");
            assert(inst->txbuf);
        }
#else
        inst->txbuf = malloc(inst->txbuf_size);
        assert(inst->txbuf);
#endif
    }
    if (!inst->rxbuf) {
#ifdef __KERNEL__
        inst->rxbuf = kmalloc(inst->rxbuf_size, GFP_DMA|GFP_KERNEL);
        if (!inst->rxbuf) {
            printk("ERROR, can't allocate rxbuf\n");
            assert(inst->rxbuf);
        }
#else
        inst->rxbuf = malloc(inst->rxbuf_size);
        assert(inst->rxbuf);
#endif
    }
}

void
uwb_dev_deinit(struct uwb_dev * inst)
{
#ifdef __KERNEL__
    if (inst->txbuf) {
        kfree(inst->txbuf);
        inst->txbuf = 0;
    }
    if (inst->rxbuf) {
        kfree(inst->rxbuf);
        inst->rxbuf = 0;
    }
#else
    if (inst->txbuf) {
        free(inst->txbuf);
        inst->txbuf = 0;
    }
    if (inst->rxbuf) {
        free(inst->rxbuf);
        inst->rxbuf = 0;
    }
#endif
}

/*!
 * @fn uwb_calc_aoa(float pdoa, float wavelength, float antenna_separation)
 *
 * @brief Calculate the angle of arrival
 *
 * @param pdoa       - phase difference of arrival in radians
 * @param wavelength - wavelength of the radio channel used in meters
 * @param antenna_separation - distance between centers of antennas in meters
 *
 * output parameters
 *
 * returns angle of arrival - float, in radians
 */
dpl_float32_t
uwb_calc_aoa(dpl_float32_t pdoa, int channel, dpl_float32_t antenna_separation)
{
    dpl_float32_t pd_dist, wavelength;
    dpl_float32_t frequency = DPL_FLOAT32_INIT(0);
    switch(channel) {
    case (1): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_1);break;
    case (2): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_2);break;
    case (3): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_3);break;
    case (4): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_4);break;
    case (5): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_5);break;
    case (7): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_7);break;
    case (9): frequency = DPL_FLOAT32_INIT(UWB_CH_FREQ_CHAN_9);break;
    default: assert(0); break;
    }
    wavelength = DPL_FLOAT32_DIV(DPL_FLOAT32_INIT(SPEED_OF_LIGHT), frequency);
    pd_dist = DPL_FLOAT32_DIV(pdoa, DPL_FLOAT32_INIT(2.0f*M_PI));
    pd_dist = DPL_FLOAT32_MUL(pd_dist, wavelength);
    return DPL_FLOAT32_FROM_F64(
        DPL_FLOAT64_ASIN(DPL_FLOAT64_FROM_F32(
                         DPL_FLOAT32_DIV(pd_dist, antenna_separation)
                         )
            )
        );
}


/* When building for kernel we can't inline the api translation.
 */
#ifdef __KERNEL__
#define UWB_API_IMPL_PREFIX
#include "uwb/uwb_api_impl.h"

s32 uwb_float32_to_s32(dpl_float32_t arg)
{
    return DPL_FLOAT32_INT(arg);
}
EXPORT_SYMBOL(uwb_float32_to_s32);

s32 uwb_float32_to_s32x1000(dpl_float32_t arg)
{
    return DPL_FLOAT32_INT(DPL_FLOAT32_MUL(DPL_FLOAT32_INIT(1000.0), arg));
}
EXPORT_SYMBOL(uwb_float32_to_s32x1000);

#endif

/**
 * Set Absolute rx end post tx, in dtu
 *
 * @param dev      pointer to struct uwb_dev.
 * @param tx_ts    The future timestamp of the tx will start, in dwt timeunits (uwb usecs << 16).
 * @param tx_post_rmarker_len Length of package after rmarker.
 * @param rx_end   The future time at which the RX will end, in dwt timeunits.
 *
 * @return struct uwb_dev_status
 *
 * @brief Automatically adjusts the rx_timeout after each received frame to match dx_time_end
 * in a rx window after a tx-frame. Note that the rx-timeout needs to be calculated as the
 * length in time after the completion of the transmitted frame, not after the rmarker (set timestamp).
 */
struct uwb_dev_status
uwb_set_post_tx_rx_window(struct uwb_dev * dev,
                          uint64_t tx_ts, uint32_t tx_post_rmarker_len,
                          uint64_t rx_end)
{
    /* Calculate initial rx-timeout */
    uint64_t tx_end = (tx_ts + tx_post_rmarker_len) & UWB_DTU_40BMASK;
    uint32_t timeout = ((rx_end - tx_end) & UWB_DTU_40BMASK) >> 16;
    uwb_set_abs_timeout(dev, rx_end);
    uwb_set_rx_timeout(dev, timeout);
    return dev->status;
}
