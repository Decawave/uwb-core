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

#include "dpl/dpl.h"
#include "wqueue.h"

extern "C" {

typedef wqueue<dpl_event *> wqueue_t;

static struct dpl_eventq dflt_evq;

struct dpl_eventq *
dpl_eventq_dflt_get(void)
{
    if (!dflt_evq.q) {
        dflt_evq.q = new wqueue_t();
    }

    return &dflt_evq;
}

void
dpl_eventq_init(struct dpl_eventq *evq)
{
    evq->q = new wqueue_t();
}

bool
dpl_eventq_is_empty(struct dpl_eventq *evq)
{
    wqueue_t *q = static_cast<wqueue_t *>(evq->q);

    if (q->size()) {
        return 1;
    } else {
        return 0;
    }
}

int
dpl_eventq_inited(struct dpl_eventq *evq)
{
    return (evq->q != NULL);
}

void
dpl_eventq_put(struct dpl_eventq *evq, struct dpl_event *ev)
{
    wqueue_t *q = static_cast<wqueue_t *>(evq->q);

    if (ev->ev_queued) {
        return;
    }

    ev->ev_queued = 1;
    q->put(ev);
}

struct dpl_event *
dpl_eventq_get(struct dpl_eventq *evq)
{
    struct dpl_event *ev;
    wqueue_t *q = static_cast<wqueue_t *>(evq->q);

    ev = q->get(DPL_TIMEOUT_NEVER);

    if (ev) {
        ev->ev_queued = 0;
    }

    return ev;
}

void
dpl_eventq_run(struct dpl_eventq *evq)
{
    struct dpl_event *ev;
    ev = dpl_eventq_get(evq);
    dpl_event_run(ev);
}


// ========================================================================
//                         Event Implementation
// ========================================================================

void
dpl_event_init(struct dpl_event *ev, dpl_event_fn *fn,
                   void *arg)
{
    memset(ev, 0, sizeof(*ev));
    ev->ev_cb = fn;
    ev->ev_arg = arg;
}

bool
dpl_event_is_queued(struct dpl_event *ev)
{
    return ev->ev_queued;
}

void *
dpl_event_get_arg(struct dpl_event *ev)
{
    return ev->ev_arg;
}

void
dpl_event_set_arg(struct dpl_event *ev, void *arg)
{
    ev->ev_arg = arg;
}

void
dpl_event_run(struct dpl_event *ev)
{
    if(ev == NULL)
	return;
    assert(ev->ev_cb != NULL);
    ev->ev_cb(ev);
}

}
