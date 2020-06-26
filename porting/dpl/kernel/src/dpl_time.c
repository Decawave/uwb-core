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

#include <stdint.h>
#include <time.h>
#include <linux/delay.h>
#include "dpl/dpl_time.h"
#include "hal/hal_timer.h"

/**
 * Return ticks [us] since system start as uint32_t.
 */
dpl_time_t
dpl_time_get(void)
{
    return hal_timer_read(0);
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

dpl_time_t dpl_time_ms_to_ticks32(uint32_t ms)
{
#if DPL_TICKS_PER_SEC == 1000000
    return ms*1000;
#else
    return ms*DPL_TICKS_PER_SEC/1000;
#endif
}
EXPORT_SYMBOL(dpl_time_ms_to_ticks32);

uint32_t dpl_time_ticks_to_ms32(dpl_time_t ticks)
{
#if DPL_TICKS_PER_SEC == 1000000
    return ticks/1000;
#else
    return ticks*1000/DPL_TICKS_PER_SEC;
#endif
}
EXPORT_SYMBOL(dpl_time_ticks_to_ms32);

void
dpl_time_delay(dpl_time_t ticks)
{
    uint64_t sleep_us;
#if DPL_TICKS_PER_SEC == 1000000
    sleep_us = ticks;
#else
    uint64_t ms = dpl_time_ticks_to_ms32(ticks);
    sleep_us = ms*1000;
#endif
    if (sleep_us < 11) {
        udelay(sleep_us);
    } else {
        usleep_range(sleep_us, sleep_us+50);
    }
}
