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
#include <signal.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include "dpl/dpl.h"
#include "hal/hal_timer.h"

/*
 * For native cpu implementation.
 */
static struct dpl_task native_timer_task_struct;
static struct dpl_eventq native_timer_evq;


struct native_timer {
    timer_t  timer;
    struct sigaction sa;
    uint32_t ticks_per_ostick;
    uint32_t cnt;
    uint32_t last_ostime;
    int num;
    TAILQ_HEAD(hal_timer_qhead, hal_timer) timers;
} native_timers[1];


/**
 * This is the function called when the timer fires.
 *
 * @param arg
 */

static void
native_timer_cb(struct native_timer *nt)
{
    int32_t delta = 0;
    struct itimerspec its;
    uint32_t cnt;
    struct hal_timer *ht;
    dpl_sr_t sr;

    /* Disable interrupt while running */
    DPL_ENTER_CRITICAL(sr);
    cnt = hal_timer_read(nt->num);
    while ((ht = TAILQ_FIRST(&nt->timers)) != NULL) {
        if (((int32_t)(cnt - ht->expiry)) >= delta) {
            TAILQ_REMOVE(&nt->timers, ht, link);
            ht->link.tqe_prev = NULL;
            ht->cb_func(ht->cb_arg);
        } else {
            break;
        }
    }

    ht = TAILQ_FIRST(&nt->timers);
    if (ht) {
        uint32_t ticks = (ht->expiry - hal_timer_read(nt->num)) / nt->ticks_per_ostick;
        if (!ticks) ticks = 1;
        /* Workaround to reduce latency - if we let the timer expire to seldom
         * our latency goes up. I.e. wait max 1ms */
        if (ticks > 1000) ticks = 1000;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec = (ticks / 1000000);
        its.it_value.tv_nsec = (ticks % 1000000) * 1000; // expiration
        its.it_value.tv_nsec %= 1000000000;
        timer_settime(nt->timer, 0, &its, NULL);
    }
    DPL_EXIT_CRITICAL(sr);
}

static void
native_timer_sysev_cb(union sigval sv)
{
    struct native_timer *nt = (struct native_timer *)sv.sival_ptr;
    assert(nt);
    native_timer_cb(nt);
}

static void
handler(int sig, siginfo_t *si, void *uc)
{
    timer_t *tidp;
    struct timespec ts;
    int overruns;
    int diff;

    tidp = si->si_value.sival_ptr;
    struct native_timer *nt = (struct native_timer *)si->si_value.sival_ptr;
    native_timer_cb(nt);
}

int
hal_timer_init(int num, void *cfg)
{
    return 0;
}

int
hal_timer_config(int num, uint32_t clock_freq)
{
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

    nt->last_ostime = dpl_time_get();

#if !defined(ANDROID_APK_BUILD)
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
    }

    /* Try to alter priority */
    struct sched_param sched_param;
    const int sched_policy = SCHED_FIFO;
    int max_prio = sched_get_priority_max(SCHED_FIFO);
    int min_prio = sched_get_priority_min(SCHED_FIFO);
    sched_param.sched_priority = max_prio - 10;
    if (sched_param.sched_priority < min_prio) {
        sched_param.sched_priority = min_prio;
    }
    int res = sched_setscheduler(0, sched_policy, &sched_param);
    if (res < 0) {
        perror("Could not set scheduler");
    }
#endif
    /* Setup signal */
    nt->sa.sa_flags = SA_SIGINFO;
    nt->sa.sa_sigaction = handler;
    sigemptyset(&nt->sa.sa_mask);
    assert(sigaction(SIGRTMIN, &nt->sa, NULL) != -1);

    struct sigevent event;
    event.sigev_notify = SIGEV_SIGNAL;
    event.sigev_signo = SIGRTMIN;
    event.sigev_value.sival_ptr = nt; // put native_timer in signal args

    timer_create(CLOCK_REALTIME, &event, &nt->timer);

    return 0;
}

int
hal_timer_deinit(int num)
{
    struct native_timer *nt;

    if (num != 0) {
        return -1;
    }
    nt = &native_timers[num];

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

    if (num >= 0) {
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
    dpl_sr_t sr;
    uint32_t ostime;
    uint32_t delta_osticks;

    if (num != 0) {
        return -1;
    }

    nt = &native_timers[num];
    DPL_ENTER_CRITICAL(sr);
    ostime = dpl_time_get();
    delta_osticks = (uint32_t)(ostime - nt->last_ostime);
    if (delta_osticks) {
        nt->last_ostime = ostime;
        nt->cnt += nt->ticks_per_ostick * delta_osticks;

    }
    DPL_EXIT_CRITICAL(sr);

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
    struct native_timer *nt;

    if (num != 0) {
        return -1;
    }
    nt = &native_timers[num];
    timer->cb_func = cb_func;
    timer->cb_arg = arg;
    timer->bsp_timer = nt;
    timer->link.tqe_prev = NULL;
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
    struct itimerspec its;
    struct native_timer *nt;
    struct hal_timer *ht;
    uint32_t curtime;
    uint32_t osticks;
    dpl_sr_t sr;

    nt = (struct native_timer *)timer->bsp_timer;

    timer->expiry = tick;

    DPL_ENTER_CRITICAL(sr);

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

    curtime = hal_timer_read(nt->num);
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if ((int32_t)(tick - curtime) <= 0) {
        /*
         * Event in the past (should be the case if it was just inserted).
         */
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 1000;
        timer_settime(nt->timer, 0, &its, NULL);
    } else {
        if (timer == TAILQ_FIRST(&nt->timers)) {
            osticks = (tick - curtime) / nt->ticks_per_ostick;
            its.it_value.tv_sec = (osticks / 1000000);
            its.it_value.tv_nsec = (osticks % 1000000) * 1000; // expiration
            its.it_value.tv_nsec %= 1000000000;
            timer_settime(nt->timer, 0, &its, NULL);
        }
    }
    DPL_EXIT_CRITICAL(sr);

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
    struct itimerspec its;
    struct native_timer *nt;
    struct hal_timer *ht;
    int reset_ocmp;
    dpl_sr_t sr;

    DPL_ENTER_CRITICAL(sr);

    nt = (struct native_timer *)timer->bsp_timer;
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
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;
            if (ht) {
                uint32_t ticks = (ht->expiry - hal_timer_read(nt->num)) /
                    nt->ticks_per_ostick;
                if (!ticks) ticks = 1;
                its.it_value.tv_sec = (ticks / 1000000);
                its.it_value.tv_nsec = (ticks % 1000000) * 1000; // expiration
                its.it_value.tv_nsec %= 1000000000;
            } else {
                /* Disable timer */
                its.it_value.tv_sec = 0;
                its.it_value.tv_nsec = 0;
            }
            timer_settime(nt->timer, 0, &its, NULL);
        }
    }
    DPL_EXIT_CRITICAL(sr);

    return 0;
}
