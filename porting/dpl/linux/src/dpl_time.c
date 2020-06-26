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

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "dpl/dpl_time.h"

/**
 * Return ticks [us] since system start as uint32_t.
 */
dpl_time_t
dpl_time_get(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now)) {
        return 0;
    }
    /* Handle 32bit overflow */
    uint64_t v = now.tv_sec * 1000000 + now.tv_nsec / 1000;
    while (v > 0xffffffffUL) v -= 0xffffffffUL;
    return (dpl_time_t)v;
}

dpl_error_t dpl_time_ms_to_ticks(uint32_t ms, dpl_time_t *out_ticks)
{
#if DPL_TICKS_PER_SEC == 1000000
    *out_ticks = ms*1000;
#else
    *out_ticks = ms*DPL_TICKS_PER_SEC/1000;
#endif
    return DPL_OK;
}

dpl_error_t dpl_time_ticks_to_ms(dpl_time_t ticks, uint32_t *out_ms)
{
#if DPL_TICKS_PER_SEC == 1000000
    *out_ms = ticks/1000;
#else
    *out_ms = ticks*1000/DPL_TICKS_PER_SEC;
#endif
    return DPL_OK;
}

dpl_time_t dlp_time_ms_to_ticks32(uint32_t ms)
{
#if DPL_TICKS_PER_SEC == 1000000
    return ms*1000;
#else
    return ms*DPL_TICKS_PER_SEC/1000;
#endif
}

uint32_t dpl_time_ticks_to_ms32(dpl_time_t ticks)
{
#if DPL_TICKS_PER_SEC == 1000000
    return ticks/1000;
#else
    return ticks*1000/DPL_TICKS_PER_SEC;
#endif
}

void
dpl_time_delay(dpl_time_t ticks)
{
    struct timespec sleep_time;
#if DPL_TICKS_PER_SEC == 1000000
    uint32_t us = ticks;
    uint32_t s = us / 1000000;

    us -= s * 1000000;
    sleep_time.tv_sec = s;
    sleep_time.tv_nsec = us * 1000;
#else
    long ms = dpl_time_ticks_to_ms32(ticks);
    uint32_t s = ms / 1000;

    ms -= s * 1000;
    sleep_time.tv_sec = s;
    sleep_time.tv_nsec = ms * 1000000;
#endif
    nanosleep(&sleep_time, NULL);
}
