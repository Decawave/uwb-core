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

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "syscfg/syscfg.h"
#include "dpl/dpl_cputime.h"
#include "hal/hal_timer.h"

#if defined(OS_CPUTIME_FREQ_HIGH)
struct dpl_cputime_data g_dpl_cputime;
#endif

int
dpl_cputime_init(uint32_t clock_freq)
{
    int rc;

    /* Set the ticks per microsecond. */
#if defined(OS_CPUTIME_FREQ_HIGH)
    g_dpl_cputime.ticks_per_usec = clock_freq / 1000000U;
#endif
    rc = hal_timer_config(MYNEWT_VAL(OS_CPUTIME_TIMER_NUM), clock_freq);
    return rc;
}

/**
 * Wait until the number of ticks has elapsed. This is a blocking delay.
 *
 * @param ticks The number of ticks to wait.
 */
void
dpl_cputime_delay_ticks(uint32_t ticks)
{
    uint32_t until;

    until = dpl_cputime_get32() + ticks;
    while ((int32_t)(dpl_cputime_get32() - until) < 0) {
        /* Loop here till finished */
    }
}

void
dpl_cputime_delay_usecs(uint32_t usecs)
{
    uint32_t ticks;
    ticks = dpl_cputime_usecs_to_ticks(usecs);
    dpl_cputime_delay_ticks(ticks);
}

void
dpl_cputime_timer_init(struct hal_timer *timer, hal_timer_cb fp, void *arg)
{
    assert(timer != NULL);
    assert(fp != NULL);
    hal_timer_set_cb(MYNEWT_VAL(OS_CPUTIME_TIMER_NUM), timer, fp, arg);
}

int
dpl_cputime_timer_start(struct hal_timer *timer, uint32_t cputime)
{
    int rc;

    rc = hal_timer_start_at(timer, cputime);
    return rc;
}

int
dpl_cputime_timer_relative(struct hal_timer *timer, uint32_t usecs)
{
    int rc;
    uint32_t cputime;

    assert(timer != NULL);

    cputime = dpl_cputime_get32() + dpl_cputime_usecs_to_ticks(usecs);
    rc = hal_timer_start_at(timer, cputime);

    return rc;
}

void
dpl_cputime_timer_stop(struct hal_timer *timer)
{
    hal_timer_stop(timer);
}

uint32_t
dpl_cputime_get32(void)
{
    uint32_t cpu_time;

    cpu_time = hal_timer_read(MYNEWT_VAL(OS_CPUTIME_TIMER_NUM));
return cpu_time;
}
