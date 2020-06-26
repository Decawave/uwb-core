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

#ifndef _DPL_EVENT_H_
#define _DPL_EVENT_H_

#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <dpl/dpl_types.h>
#include <dpl/dpl_time.h>
#include <linux/workqueue.h>

struct dpl_event;
struct dpl_eventq;
typedef void dpl_event_fn(struct dpl_event *ev);

/*
 * Event queue
 */

struct dpl_event {
    struct work_struct  work;
	atomic_t            ev_queued;
    dpl_event_fn        *ev_cb;
    void                *ev_arg;
};

struct dpl_eventq {
    struct workqueue_struct *wq;
};

void dpl_eventq_init(struct dpl_eventq *evq);
int dpl_eventq_inited(struct dpl_eventq *evq);
void dpl_eventq_deinit(struct dpl_eventq *evq);

struct dpl_event * dpl_eventq_get(struct dpl_eventq *evq);
#define dpl_eventq_get_no_wait(__A) dpl_eventq_get(__A)
void dpl_eventq_put(struct dpl_eventq *evq, struct dpl_event *ev);
void dpl_eventq_remove(struct dpl_eventq *evq, struct dpl_event *ev);
void dpl_eventq_run(struct dpl_eventq *evq);
bool dpl_eventq_is_empty(struct dpl_eventq *evq);
struct dpl_eventq * dpl_eventq_dflt_get(void);

void dpl_event_init(struct dpl_event *ev, dpl_event_fn * fn, void *arg);
bool dpl_event_is_queued(struct dpl_event *ev);
void *dpl_event_get_arg(struct dpl_event *ev);
void dpl_event_set_arg(struct dpl_event *ev, void *arg);
void dpl_event_run(struct dpl_event *ev);

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_EVENT_H_ */
