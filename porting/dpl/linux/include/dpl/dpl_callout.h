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

#ifndef _DPL_CALLOUT_H_
#define _DPL_CALLOUT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "dpl/dpl_types.h"
#include "dpl/dpl_eventq.h"
#include "dpl/dpl_error.h"
/*
 * Callouts
 */

void dpl_callout_init(struct dpl_callout *co, struct dpl_eventq *evq, dpl_event_fn *ev_cb, void *ev_arg);
dpl_error_t dpl_callout_reset(struct dpl_callout *co, dpl_time_t ticks);
void dpl_callout_stop(struct dpl_callout *co);
bool dpl_callout_is_active(struct dpl_callout *co);
dpl_time_t dpl_callout_get_ticks(struct dpl_callout *co);
dpl_time_t dpl_callout_remaining_ticks(struct dpl_callout *co, dpl_time_t time);
void dpl_callout_set_arg(struct dpl_callout *co, void *arg);

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_CALLOUT_H_ */
