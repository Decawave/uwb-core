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
#include "os/os.h"
#include "os/os_eventq.h"
#include "os/os_task.h"

typedef os_time_t dpl_time_t;
typedef os_stime_t dpl_stime_t;
typedef os_stack_t dpl_stack_t;
/*
 * This allows to cast between dpl_* and os_* structs to make DPL for Mynewt
 * just a shim layer on top of kernel/os.
 */

struct dpl_event {
    struct os_event ev;
};

struct dpl_eventq {
    struct os_eventq evq;
};

struct dpl_callout {
    struct os_callout co;
};

struct dpl_mutex {
    struct os_mutex mu;
};

struct dpl_sem {
    struct os_sem sem;
};

struct dpl_task {
    struct os_task t;
};

/* dpl_timeval and dpl_timezone are identical copies of os equivalents */
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

typedef void dpl_event_fn(struct dpl_event *ev);
typedef void * (dpl_task_func_t)(void *);

/* Floating point type replacement for in kernel */
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
#define DPL_FLOAT32_PRINTF_PRIM "%s%d.%03d"
#define DPL_FLOAT32_PRINTF_VALS(__X) (__X)<0?"-":"", (int)(fabsf(__X)), (int)(fabsf((__X)-(int)(__X))*1000)
#define DPL_FLOAT64_PRINTF_PRIM "%s%d.%06d"
#define DPL_FLOAT64_PRINTF_VALS(__X) (__X)<0?"-":"", (int)(fabs(__X)), (int)(fabs((__X)-(int)(__X))*1000000)

#endif // _DPL_TYPES_H
