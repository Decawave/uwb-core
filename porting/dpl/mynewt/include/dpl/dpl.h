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

#ifndef _DPL_H_
#define _DPL_H_

#include <stdint.h>
#include <string.h>
#include "os/os.h"
#include "os/util.h"
#include "os/os_eventq.h"
#include "os/os_task.h"

#include "dpl/dpl_os.h"
#include "dpl/dpl_types.h"
#include "dpl/dpl_error.h"
#include "dpl/dpl_mem.h"
#include "dpl/dpl_mbuf.h"
#include "dpl/dpl_mempool.h"
#include "dpl/dpl_cputime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DPL_TIME_FOREVER    (OS_TIMEOUT_NEVER)
#define DPL_TIMEOUT_NEVER   (OS_TIMEOUT_NEVER)
#define DPL_WAIT_FOREVER    (OS_TIMEOUT_NEVER)
#define DPL_STACK_ALIGNMENT (OS_STACK_ALIGNMENT)
#define DPL_CFG_ALIGNMENT   (OS_CFG_ALIGNMENT)
#define DPL_TICKS_PER_SEC   (OS_TICKS_PER_SEC)

static inline bool
dpl_os_started(void)
{
    return os_started();
}

static inline void *
dpl_get_current_task_id(void)
{
    return os_sched_get_current_task();
}

static inline struct dpl_eventq *
dpl_eventq_dflt_get(void)
{
    return (struct dpl_eventq *) os_eventq_dflt_get();
}

static inline void
dpl_eventq_init(struct dpl_eventq *evq)
{
    os_eventq_init((struct os_eventq *) evq);
}

static inline int
dpl_eventq_inited(struct dpl_eventq *evq)
{
    return os_eventq_inited((struct os_eventq *) evq);
}

#define dpl_eventq_deinit(__X) {}

static inline struct dpl_event *
dpl_eventq_get(struct dpl_eventq *evq)
{
    return (struct dpl_event *) os_eventq_get((struct os_eventq *) evq);
}

static inline void
dpl_eventq_put(struct dpl_eventq *evq, struct dpl_event *ev)
{
    os_eventq_put((struct os_eventq *) evq, (struct os_event *) ev);
}

static inline void
dpl_eventq_remove(struct dpl_eventq *evq, struct dpl_event *ev)
{
    os_eventq_remove((struct os_eventq *) evq, (struct os_event *) ev);
}

static inline struct dpl_event *
dpl_eventq_get_no_wait(struct dpl_eventq *evq)
{
    return (struct dpl_event *) os_eventq_get_no_wait((struct os_eventq *) evq);
}

static inline void
dpl_eventq_run(struct dpl_eventq *evq)
{
    os_eventq_run((struct os_eventq * )evq);
}

static inline bool
dpl_eventq_is_empty(struct dpl_eventq *evq)
{
    return STAILQ_EMPTY(&evq->evq.evq_list);
}

static inline void
dpl_event_init(struct dpl_event *ev, dpl_event_fn *fn, void *arg)
{
    memset(ev, 0, sizeof(*ev));
    ev->ev.ev_queued = 0;
    ev->ev.ev_cb = (os_event_fn *)fn;
    ev->ev.ev_arg = arg;
}

static inline bool
dpl_event_is_queued(struct dpl_event *ev)
{
    return ev->ev.ev_queued;
}

static inline void *
dpl_event_get_arg(struct dpl_event *ev)
{
    return ev->ev.ev_arg;
}

static inline void
dpl_event_run(struct dpl_event *ev)
{
    return ((struct os_event*)ev)->ev_cb((struct os_event*)ev);
}

static inline void
dpl_event_set_arg(struct dpl_event *ev, void *arg)
{
    ev->ev.ev_arg = arg;
}

static inline dpl_error_t
dpl_mutex_init(struct dpl_mutex *mu)
{
    return (dpl_error_t)os_mutex_init(&mu->mu);
}

static inline dpl_error_t
dpl_mutex_pend(struct dpl_mutex *mu, dpl_time_t timeout)
{
    return (dpl_error_t)os_mutex_pend(&mu->mu, timeout);
}

static inline dpl_error_t
dpl_mutex_release(struct dpl_mutex *mu)
{
    return (dpl_error_t)os_mutex_release(&mu->mu);
}

static inline dpl_error_t
dpl_sem_init(struct dpl_sem *sem, uint16_t tokens)
{
    return (dpl_error_t)os_sem_init(&sem->sem, tokens);
}

static inline dpl_error_t
dpl_sem_pend(struct dpl_sem *sem, dpl_time_t timeout)
{
    return (dpl_error_t)os_sem_pend(&sem->sem, timeout);
}

static inline dpl_error_t
dpl_sem_release(struct dpl_sem *sem)
{
    return (dpl_error_t)os_sem_release(&sem->sem);
}

static inline uint16_t
dpl_sem_get_count(struct dpl_sem *sem)
{
    return os_sem_get_count(&sem->sem);
}

static inline void
dpl_callout_init(struct dpl_callout *co, struct dpl_eventq *evq,
                     dpl_event_fn *ev_cb, void *ev_arg)
{
    os_callout_init(&co->co, &evq->evq, (os_event_fn *)ev_cb, ev_arg);
}

static inline dpl_error_t
dpl_callout_reset(struct dpl_callout *co, dpl_time_t ticks)
{
    return (dpl_error_t)os_callout_reset(&co->co, ticks);
}

static inline void
dpl_callout_stop(struct dpl_callout *co)
{
    os_callout_stop(&co->co);
}

static inline bool
dpl_callout_is_active(struct dpl_callout *co)
{
    return os_callout_queued(&co->co);
}

static inline dpl_time_t
dpl_callout_get_ticks(struct dpl_callout *co)
{
    return co->co.c_ticks;
}


static inline dpl_time_t
dpl_callout_remaining_ticks(struct dpl_callout *co,
                                dpl_time_t time)
{
    return os_callout_remaining_ticks(&co->co, time);
}

static inline void
dpl_callout_set_arg(struct dpl_callout *co,
                        void *arg)
{
    co->co.c_ev.ev_arg = arg;
}

static inline struct dpl_event*
dpl_callout_get_event(struct dpl_callout *co)
{
    return (struct dpl_event*)&co->co.c_ev;
}

static inline uint32_t
dpl_time_get(void)
{
    return os_time_get();
}

static inline void
dpl_get_uptime(struct dpl_timeval *tvp)
{
    os_get_uptime((struct os_timeval*)tvp);
}

static inline int
dpl_gettimeofday(struct dpl_timeval *tv, struct dpl_timezone *tz)
{
    return os_gettimeofday((struct os_timeval*)tv, (struct os_timezone*)tz);
}

static inline dpl_error_t
dpl_time_ms_to_ticks(uint32_t ms, dpl_time_t *out_ticks)
{
    return (dpl_error_t)os_time_ms_to_ticks(ms, out_ticks);
}

static inline dpl_error_t
dpl_time_ticks_to_ms(dpl_time_t ticks, uint32_t *out_ms)
{
    return os_time_ticks_to_ms(ticks, out_ms);
}

static inline dpl_time_t
dpl_time_ms_to_ticks32(uint32_t ms)
{
    return os_time_ms_to_ticks32(ms);
}

static inline uint32_t
dpl_time_ticks_to_ms32(dpl_time_t ticks)
{
    return os_time_ticks_to_ms32(ticks);
}

static inline void
dpl_time_delay(dpl_time_t ticks)
{
    return os_time_delay(ticks);
}

static inline uint32_t
dpl_hw_enter_critical(void)
{
    return os_arch_save_sr();
}

static inline void
dpl_hw_exit_critical(uint32_t ctx)
{
    os_arch_restore_sr(ctx);
}

static inline int dpl_task_init(struct dpl_task *t, const char *name, dpl_task_func_t func,
		 void *arg, uint8_t prio, dpl_time_t sanity_itvl,
		 os_stack_t * stack_bottom, uint16_t stack_size)
{

    return os_task_init((struct os_task *) t, name, (os_task_func_t) func,
        arg, prio, (os_time_t) sanity_itvl,
        (os_stack_t *) stack_bottom, stack_size) ;
}

static inline int dpl_task_remove(struct dpl_task * t)
{
    return os_task_remove((struct os_task * ) t);
}

static inline uint8_t dpl_task_count(void)
{
    return os_task_count();
}

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_H_ */
