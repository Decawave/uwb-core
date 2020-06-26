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

#ifndef _DPL_TIME_H_
#define _DPL_TIME_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "dpl/dpl_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Time functions
 */
typedef uint32_t dpl_time_t;
#define CLK_TCK 1000000
#define DPL_TICKS_PER_SEC CLK_TCK

dpl_time_t dpl_time_get(void);
dpl_error_t dpl_time_ms_to_ticks(uint32_t ms, dpl_time_t *out_ticks);
dpl_error_t dpl_time_ticks_to_ms(dpl_time_t ticks, uint32_t *out_ms);
dpl_time_t dpl_time_ms_to_ticks32(uint32_t ms);
uint32_t dpl_time_ticks_to_ms32(dpl_time_t ticks);
void dpl_time_delay(dpl_time_t ticks);

/* Not yet implemented */
//void dpl_get_uptime(struct dpl_timeval *tvp);
//int dpl_gettimeofday(struct dpl_timeval *tv, struct dpl_timezone *tz);

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_TIME_H_ */
