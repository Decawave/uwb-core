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

#ifdef __KERNEL__
#include <linux/time.h>
#include <linux/signal.h>
#include <linux/math64.h>

#include <stdbool.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>

#include <dpl/dpl_eventq.h>
#include <dpl/dpl_time.h>
#include "softfloat/softfloat.h"
#include "softfloat/softfloat_types.h"
#include <softfloat/specialize.h>


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
#define OS_TASK_PRI_HIGHEST 99
#define OS_TASK_PRI_LOWEST  1

#ifndef OS_STACK_ALIGNMENT
#define OS_STACK_ALIGNMENT              (8)
#endif

#define DPL_STACK_ALIGNMENT (OS_STACK_ALIGNMENT)

typedef int dpl_stack_t;
struct dpl_event;
struct dpl_eventq;

struct dpl_callout {
    struct delayed_work dlyd_work;
    struct dpl_event    c_ev;
    struct dpl_eventq  *c_evq;
    uint32_t    c_ticks;
    bool        c_active;
};

struct dpl_mutex {
    struct mutex lock;
};

struct dpl_sem {
    struct semaphore lock;
};

struct dpl_task {
    // Not actually used - using workqueues directly instead
    const char* name;
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

#endif

/* Floating point type replacement for in kernel */
typedef float32_t dpl_float32_t;
typedef float64_t dpl_float64_t;
#define DPL_FLOAT32_INIT(__X) SOFTFLOAT_INIT32(__X)
#define DPL_FLOAT64_INIT(__X) SOFTFLOAT_INIT64(__X)
#define DPL_FLOAT32_I32_TO_F32(__X) i32_to_f32(__X)
#define DPL_FLOAT64_I32_TO_F64(__X) i32_to_f64(__X)
#define DPL_FLOAT64_I64_TO_F64(__X) i64_to_f64(__X)
#define DPL_FLOAT64_U64_TO_F64(__X) ui64_to_f64(__X)
#define DPL_FLOAT64_F64_TO_U64(__X) f64_to_ui64(f64_roundToInt(__X, softfloat_round_min, false), softfloat_round_min, false)
#define DPL_FLOAT32_INT(__X) f32_to_i32(f32_roundToInt(__X, softfloat_round_min, false), softfloat_round_min, false)
#define DPL_FLOAT64_INT(__X) f64_to_i64(f64_roundToInt(__X, softfloat_round_min, false), softfloat_round_min, false)
#define DPL_FLOAT64_FROM_F32(__X) f32_to_f64(__X)
#define DPL_FLOAT32_FROM_F64(__X) f64_to_f32(__X)
#define DPL_FLOAT32_FRAC(__X) f32_sub(soft_abs32(__X), f32_roundToInt(soft_abs32(__X), softfloat_round_min, false))
#define DPL_FLOAT64_FRAC(__X) f64_sub(soft_abs(__X), f64_roundToInt(soft_abs(__X), softfloat_round_min, false))
#define DPL_FLOAT32_CEIL(__X) f32_roundToInt(__X, softfloat_round_max, false)
#define DPL_FLOAT64_CEIL(__X) f64_roundToInt(__X, softfloat_round_max, false)
#define DPL_FLOAT32_FMOD(__X, __Y) fmodf_soft(__X, __Y)
#define DPL_FLOAT64_FMOD(__X, __Y) fmod_soft(__X, __Y)
#define DPL_FLOAT32_NAN() soft_nan32()
#define DPL_FLOAT64_NAN() soft_nan()
#define DPL_FLOAT32_ISNAN(__X) soft_isnan32(__X)
#define DPL_FLOAT64_ISNAN(__X) soft_isnan(__X)
#define DPL_FLOAT32_FABS(__X) soft_abs32(__X)
#define DPL_FLOAT64_FABS(__X) soft_abs(__X)
#define DPL_FLOAT32_LOG10(__X) log10_soft(f32_to_f64(__X))
#define DPL_FLOAT64_LOG10(__X) log10_soft(__X)
#define DPL_FLOAT64_ASIN(__X) asin_soft(__X)
#define DPL_FLOAT64_ATAN(__X) atan_soft(__X)
#define DPL_FLOAT32_SUB(__X, __Y) f32_sub(__X, __Y)
#define DPL_FLOAT64_SUB(__X, __Y) f64_sub(__X, __Y)
#define DPL_FLOAT32_ADD(__X, __Y) f32_add(__X, __Y)
#define DPL_FLOAT64_ADD(__X, __Y) f64_add(__X, __Y)
#define DPL_FLOAT32_MUL(__X, __Y) f32_mul(__X, __Y)
#define DPL_FLOAT64_MUL(__X, __Y) f64_mul(__X, __Y)
#define DPL_FLOAT32_DIV(__X, __Y) f32_div(__X, __Y)
#define DPL_FLOAT64_DIV(__X, __Y) f64_div(__X, __Y)
#define DPL_FLOAT32_PRINTF_PRIM "%s%d.%03d"
#define DPL_FLOAT32_PRINTF_VALS(__X) f32_lt((__X), DPL_FLOAT32_INIT(0.0f)) ? "-":"", DPL_FLOAT32_INT(DPL_FLOAT32_FABS(__X)), DPL_FLOAT32_INT(DPL_FLOAT32_FABS(DPL_FLOAT32_MUL(DPL_FLOAT32_FRAC(__X), DPL_FLOAT32_INIT(1000.0))))
#define DPL_FLOAT64_PRINTF_PRIM "%s%lld.%06lld"
#define DPL_FLOAT64_PRINTF_VALS(__X) f64_lt((__X), DPL_FLOAT64_INIT(0.0f)) ? "-":"", DPL_FLOAT64_INT(DPL_FLOAT64_FABS(__X)), DPL_FLOAT64_INT(DPL_FLOAT64_FABS(DPL_FLOAT64_MUL(DPL_FLOAT64_FRAC(__X), DPL_FLOAT64_INIT(1000000.0))))

#endif // _DPL_TYPES_H
