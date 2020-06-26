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

#ifndef _DPL_TYPES_H
#define _DPL_TYPES_H

#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include "dpl/dpl_eventq.h"
#include "dpl/dpl_time.h"

#ifndef UINT32_MAX
#define UINT32_MAX  0xFFFFFFFFUL
#endif

#ifndef INT32_MAX
#define INT32_MAX   0x7FFFFFFFL
#endif

#define OS_TIME_MAX UINT32_MAX
#define OS_STIME_MAX INT32_MAX

/* Used to wait forever for events and mutexs */
#define DPL_TIMEOUT_NEVER (OS_TIME_MAX)
#define DPL_WAIT_FOREVER (DPL_TIMEOUT_NEVER)

/* The highest and lowest task priorities */
#define OS_TASK_PRI_HIGHEST (sched_get_priority_max(SCHED_RR))
#define OS_TASK_PRI_LOWEST  (sched_get_priority_min(SCHED_RR))

#ifndef OS_STACK_ALIGNMENT
#define OS_STACK_ALIGNMENT              (8)
#endif

#define DPL_STACK_ALIGNMENT (OS_STACK_ALIGNMENT)

#ifdef __APPLE__
typedef unsigned long __timer_t;
typedef __timer_t timer_t;
#endif


//typedef int os_sr_t;
typedef int dpl_stack_t;

struct dpl_event {
    uint8_t             ev_queued;
    dpl_event_fn        *ev_cb;
    void                *ev_arg;
};

struct dpl_eventq {
    void               *q;
};

struct dpl_callout {
    struct dpl_event    c_ev;
    struct dpl_eventq  *c_evq;
    uint32_t    c_ticks;
    timer_t     c_timer;
    bool        c_active;
};

struct dpl_mutex {
    pthread_mutex_t         lock;
    pthread_mutexattr_t     attr;
    struct timespec         wait;
};

struct dpl_sem {
    sem_t                   lock;
};

struct dpl_task {
    pthread_t               handle;
    pthread_attr_t          attr;
    struct sched_param      param;
    const char*             name;
};

struct dpl_timeval {
    /* Seconds */
    int64_t tv_sec;
    /* Microseconds within the second */
    int32_t tv_usec;
};

struct dpl_timezone {
    /** Minutes west of GMT */
    int16_t tz_minuteswest;
    /** Daylight savings time correction (if any) */
    int16_t tz_dsttime;
};

/* Floating point type replacement (for use in kernel) */
typedef float dpl_float32_t;
typedef double dpl_float64_t;
#define DPL_FLOAT32_INIT(__X) ((float)__X)
#define DPL_FLOAT64_INIT(__X) ((double)__X)
#define DPL_FLOAT64TO32(__X) (float)(__X)
#define DPL_FLOAT32_I32_TO_F32(__X) (float)(__X)
#define DPL_FLOAT64_I32_TO_F64(__X) ((double)(__X))
#define DPL_FLOAT64_I64_TO_F64(__X) ((double)(__X))
#define DPL_FLOAT64_U64_TO_F64(__X) ((double)(__X))
#define DPL_FLOAT64_F64_TO_U64(__X) ((uint64_t)(__X))
#define DPL_FLOAT32_INT(__X) ((int32_t)__X)
#define DPL_FLOAT64_INT(__X) ((int64_t)__X)
#define DPL_FLOAT64_FROM_F32(__X) (double)(__X)
#define DPL_FLOAT32_FROM_F64(__X) (float)(__X)
#define DPL_FLOAT32_CEIL(__X) (ceilf(__X))
#define DPL_FLOAT64_CEIL(__X) (ceil(__X))
#define DPL_FLOAT32_FABS(__X) fabsf(__X)
#define DPL_FLOAT32_FMOD(__X, __Y) fmodf(__X, __Y)
#define DPL_FLOAT64_FMOD(__X, __Y) fmod(__X, __Y)
#define DPL_FLOAT32_NAN() nanf("")
#define DPL_FLOAT64_NAN() nan("")
#define DPL_FLOAT32_ISNAN(__X) isnan(__X)
#define DPL_FLOAT64_ISNAN(__X) DPL_FLOAT32_ISNAN(__X)
#define DPL_FLOAT32_LOG10(__X) (log10f(__X))
#define DPL_FLOAT64_LOG10(__X) (log10(__X))
#define DPL_FLOAT64_ASIN(__X) asin(__X)
#define DPL_FLOAT64_ATAN(__X) atan(__X)
#define DPL_FLOAT32_SUB(__X, __Y) ((__X)-(__Y))
#define DPL_FLOAT64_SUB(__X, __Y) ((__X)-(__Y))
#define DPL_FLOAT32_ADD(__X, __Y) ((__X)+(__Y))
#define DPL_FLOAT64_ADD(__X, __Y) ((__X)+(__Y))
#define DPL_FLOAT32_MUL(__X, __Y) ((__X)*(__Y))
#define DPL_FLOAT64_MUL(__X, __Y) ((__X)*(__Y))
#define DPL_FLOAT32_DIV(__X, __Y) ((__X)/(__Y))
#define DPL_FLOAT64_DIV(__X, __Y) ((__X)/(__Y))
#define DPL_FLOAT32_PRINTF_PRIM "%.3f"
#define DPL_FLOAT32_PRINTF_VALS(__X) (__X)
#define DPL_FLOAT64_PRINTF_PRIM "%.6f"
#define DPL_FLOAT64_PRINTF_VALS(__X) (__X)


#endif // _DPL_OS_TYPES_H
