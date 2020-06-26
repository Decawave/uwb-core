/**
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
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <dpl/dpl.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>

#include "hal/hal_timer.h"

/*
 * For native cpu implementation.
 */

struct native_timer {
    struct hrtimer hr_timer;
    spinlock_t lock;
    spinlock_t read_lock;
    int has_config;
    struct sigaction sa;
    uint32_t ticks_per_ostick;
    uint32_t cnt;
    uint32_t last_ostime;
    int num;
    TAILQ_HEAD(hal_timer_qhead, hal_timer) timers;
} native_timers[1] = {0};


static void
timer_update_ocmp(struct native_timer *nt, struct hal_timer *ht)
{
    struct itimerspec its;
    uint32_t ticks;
    uint64_t nsec;
    unsigned long flags;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    ticks = (ht->expiry - hal_timer_read(nt->num)) / nt->ticks_per_ostick;
    if ((int32_t)ticks <= 0) {
        ticks = 1;
    }
    nsec = (ticks % 1000000) * 1000;
    nsec %= 1000000000;
    spin_lock_irqsave(&nt->read_lock, flags);
    hrtimer_start(&nt->hr_timer, ktime_set((ticks / 1000000), nsec), HRTIMER_MODE_REL);
    spin_unlock_irqrestore(&nt->read_lock, flags);
}

/**
 * This is the function called when the timer fires.
 *
 * @param arg
 */
static enum hrtimer_restart
native_timer_cb(struct native_timer *nt)
{
    enum hrtimer_restart ret;
    ktime_t kt;
    ktime_t now;
    int32_t delta = 0;
    uint32_t cnt, ticks;
    uint64_t nsec;
    struct hal_timer *ht;
    unsigned long flags;
    unsigned long read_flags;

    /* Disable interrupt while running */
    spin_lock_irqsave(&nt->lock, flags);
    cnt = hal_timer_read(nt->num);
    while ((ht = TAILQ_FIRST(&nt->timers)) != NULL) {
        if (((int32_t)(cnt - ht->expiry)) >= delta) {
            TAILQ_REMOVE(&nt->timers, ht, link);
            ht->link.tqe_prev = NULL;
            if (ht->cb_func) {
                ht->cb_func(ht->cb_arg);
            } else {
                pr_warn("NULL CB");
            }
        } else {
            break;
        }
    }

    ht = TAILQ_FIRST(&nt->timers);
    if (ht) {
        spin_lock_irqsave(&nt->read_lock, read_flags);
        now = hrtimer_cb_get_time(&nt->hr_timer);
        spin_unlock_irqrestore(&nt->read_lock, read_flags);

        ticks = (ht->expiry - hal_timer_read(nt->num)) / nt->ticks_per_ostick;
        if ((int32_t)ticks <= 0) {
            ticks = 1;
        }
        nsec = (ticks % 1000000) * 1000; // expiration
        /* nsec %= 1000000000; */
        div64_u64_rem(nsec, 1000000000UL, &nsec);
        kt = ktime_set((ticks / 1000000), nsec);

        spin_lock_irqsave(&nt->read_lock, read_flags);
        hrtimer_forward(&nt->hr_timer, now, kt);
        spin_unlock_irqrestore(&nt->read_lock, read_flags);
        ret = HRTIMER_RESTART;  //restart timer
    } else {
        ret = HRTIMER_NORESTART;
    }
    spin_unlock_irqrestore(&nt->lock, flags);
    return ret;
}

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
    struct native_timer *nt = (struct native_timer *)container_of(timer, struct native_timer, hr_timer);
    assert(nt);

    return native_timer_cb(nt);
}

int
hal_timer_init(int num, void *cfg)
{
    return 0;
}

int
hal_timer_config(int num, uint32_t clock_freq)
{
    int64_t ns;
    struct native_timer *nt;

    if (num != 0) {
        return -1;
    }
    nt = &native_timers[num];
    /* Set the clock frequency */
    nt->ticks_per_ostick = clock_freq / DPL_TICKS_PER_SEC;
    if (!nt->ticks_per_ostick) {
        nt->ticks_per_ostick = 1;
    }
    nt->num = num;
    nt->cnt = 0;
    spin_lock_init(&nt->lock);
    spin_lock_init(&nt->read_lock);

    hrtimer_init(&nt->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    nt->hr_timer.function = hrtimer_handler;

    ns = ktime_to_ns(hrtimer_cb_get_time(&nt->hr_timer));
    nt->last_ostime = 0xFFFFFFFF & (div_s64(ns, 1000));

    nt->has_config = 1;
    return 0;
}

int
hal_timer_deinit(int num)
{
    unsigned long flags;
    struct native_timer *nt;

    if (num != 0) {
        return -1;
    }
    nt = &native_timers[num];

    spin_lock_irqsave(&nt->read_lock, flags);
    hrtimer_cancel(&nt->hr_timer);
    spin_unlock_irqrestore(&nt->read_lock, flags);
    return 0;
}

/**
 * hal timer get resolution
 *
 * Get the resolution of the timer. This is the timer period, in nanoseconds
 *
 * @param timer_num
 *
 * @return uint32_t
 */
uint32_t
hal_timer_get_resolution(int num)
{
    struct native_timer *nt;

    if (num < 0 || num > ARRAY_SIZE(native_timers)) {
        return 0;
    }
    nt = &native_timers[num];
    return 1000000000 / (nt->ticks_per_ostick * DPL_TICKS_PER_SEC);
}

/**
 * hal_timer_read
 *
 * Returns the low 32 bits of timer counter.
 *
 * @return uint32_t The timer counter register.
 */
uint32_t
hal_timer_read(int num)
{
    struct native_timer *nt;
    uint32_t ostime;
    uint32_t delta_osticks;
    unsigned long flags;

    if (num != 0) {
        return -1;
    }
    nt = &native_timers[num];
    if (!nt->has_config) {
        pr_warn("%s run before config", __func__);
        return 0;
    }
    spin_lock_irqsave(&nt->read_lock, flags);
    ostime = div_s64(ktime_to_ns(hrtimer_cb_get_time(&nt->hr_timer)), 1000);
    delta_osticks = (uint32_t)(ostime - nt->last_ostime);
    if (delta_osticks) {
        nt->last_ostime = ostime;
        nt->cnt += nt->ticks_per_ostick * delta_osticks;
    }
    spin_unlock_irqrestore(&nt->read_lock, flags);

    return (uint32_t)nt->cnt;
}

/**
 * hal timer delay
 *
 * Blocking delay for n ticks
 *
 * @param timer_num
 * @param ticks
 *
 * @return int 0 on success; error code otherwise.
 */
int
hal_timer_delay(int num, uint32_t ticks)
{
    uint32_t until;

    if (num != 0) {
        return -1;
    }

    until = hal_timer_read(0) + ticks;
    while ((int32_t)(hal_timer_read(0) - until) <= 0) {
        ;
    }
    return 0;
}

/**
 *
 * Initialize the HAL timer structure with the callback and the callback
 * argument. Also initializes the HW specific timer pointer.
 *
 * @param cb_func
 *
 * @return int
 */
int
hal_timer_set_cb(int num, struct hal_timer *timer, hal_timer_cb cb_func,
                 void *arg)
{
    unsigned long flags;
    struct native_timer *nt;

    if (num != 0) {
        return -1;
    }
    nt = &native_timers[num];

    spin_lock_irqsave(&nt->lock, flags);

    timer->cb_func = cb_func;
    timer->cb_arg = arg;
    timer->bsp_timer = nt;
    timer->link.tqe_prev = NULL;

    spin_unlock_irqrestore(&nt->lock, flags);
    return 0;
}

/**
 * hal_timer_start()
 *
 * Start a timer. Timer fires 'ticks' ticks from now.
 *
 * @param timer
 * @param ticks
 *
 * @return int
 */
int
hal_timer_start(struct hal_timer *timer, uint32_t ticks)
{
    struct native_timer *nt;
    uint32_t tick;

    nt = (struct native_timer *)timer->bsp_timer;

    tick = ticks + hal_timer_read(nt->num);
    return hal_timer_start_at(timer, tick);
}

/**
 * hal_timer_start_at()
 *
 * Start a timer. Timer fires at tick 'tick'.
 *
 * @param timer
 * @param tick
 *
 * @return int
 */
int
hal_timer_start_at(struct hal_timer *timer, uint32_t tick)
{
    struct native_timer *nt;
    struct hal_timer *ht;
    unsigned long flags;

    nt = (struct native_timer *)timer->bsp_timer;

    timer->expiry = tick;

    spin_lock_irqsave(&nt->lock,flags);

    if (TAILQ_EMPTY(&nt->timers)) {
        TAILQ_INSERT_HEAD(&nt->timers, timer, link);
    } else {
        TAILQ_FOREACH(ht, &nt->timers, link) {
            if ((int32_t)(timer->expiry - ht->expiry) < 0) {
                TAILQ_INSERT_BEFORE(ht, timer, link);
                break;
            }
        }
        if (!ht) {
            TAILQ_INSERT_TAIL(&nt->timers, timer, link);
        }
    }

    /* Only update hrtimer if this is the first timer */
    if (timer == TAILQ_FIRST(&nt->timers)) {
        timer_update_ocmp(nt, timer);
    }
    spin_unlock_irqrestore(&nt->lock,flags);

    return 0;
}

/**
 * hal_timer_stop()
 *
 * Cancels the timer.
 *
 * @param timer
 *
 * @return int
 */
int
hal_timer_stop(struct hal_timer *timer)
{
    struct native_timer *nt;
    struct hal_timer *ht;
    int reset_ocmp;
    unsigned long flags;
    unsigned long read_flags;

    nt = (struct native_timer *)timer->bsp_timer;
    if (!nt) {
        return 0;
    }
    spin_lock_irqsave(&nt->lock,flags);
    if (timer->link.tqe_prev != NULL) {
        reset_ocmp = 0;
        if (timer == TAILQ_FIRST(&nt->timers)) {
            /* If first on queue, we will need to reset OCMP */
            ht = TAILQ_NEXT(timer, link);
            reset_ocmp = 1;
        }
        TAILQ_REMOVE(&nt->timers, timer, link);
        timer->link.tqe_prev = NULL;
        if (reset_ocmp) {
            if (ht) {
                timer_update_ocmp(nt, ht);
            } else {
                /* Disable timer */
                spin_lock_irqsave(&nt->read_lock, read_flags);
                hrtimer_cancel(&nt->hr_timer);
                spin_unlock_irqrestore(&nt->read_lock, read_flags);
            }
        }
    }
    spin_unlock_irqrestore(&nt->lock,flags);

    return 0;
}
