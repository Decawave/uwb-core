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
#include <dpl/dpl_types.h>
#include <linux/workqueue.h>

static struct dpl_eventq dflt_evq = {0};

struct dpl_eventq *
dpl_eventq_dflt_get(void)
{
    /* Returning a workqueue with the wq pointing to NULL */
    return &dflt_evq;
}
EXPORT_SYMBOL(dpl_eventq_dflt_get);

void
dpl_eventq_init(struct dpl_eventq *evq)
{
    /* Singlethread because this mimics the mynewt behaviour */
    evq->wq = create_singlethread_workqueue("dpl_wq");
}

void
dpl_eventq_deinit(struct dpl_eventq *evq)
{
    flush_workqueue(evq->wq);
    destroy_workqueue(evq->wq);
    evq->wq = 0;
}

bool
dpl_eventq_is_empty(struct dpl_eventq *evq)
{
    /* Not implemented */
    return false;
}

int
dpl_eventq_inited(struct dpl_eventq *evq)
{
    return (evq->wq != NULL);
}

void
dpl_eventq_put(struct dpl_eventq *evq, struct dpl_event *ev)
{
    struct workqueue_struct *wq = evq->wq;

    if (atomic_read(&ev->ev_queued) == 1) {
        return;
    }

    atomic_set(&ev->ev_queued, 1);
    if (!wq) {
        /* If the eventq isn't set, use the default work queue */
        schedule_work(&ev->work);
    } else {
        queue_work(wq, &ev->work);
    }
}

struct dpl_event *
dpl_eventq_get(struct dpl_eventq *evq)
{
    assert(0);
    return 0;
}

void
dpl_eventq_run(struct dpl_eventq *evq)
{
    assert(0);
#if 0
    struct dpl_event *ev;
    ev = dpl_eventq_get(evq);
    dpl_event_run(ev);
#endif
}


// ========================================================================
//                         Event Implementation
// ========================================================================

static void dpl_event_work_handler(struct work_struct *work)
{
    struct dpl_event *ev;
    ev = container_of(work, struct dpl_event, work);
    atomic_set(&ev->ev_queued, 0);
    if (ev->ev_cb) {
        ev->ev_cb(ev);
    }
}

void
dpl_event_init(struct dpl_event *ev, dpl_event_fn *fn,
               void *arg)
{
    memset(ev, 0, sizeof(*ev));
    INIT_WORK(&ev->work, dpl_event_work_handler);
    ev->ev_cb = fn;
    ev->ev_arg = arg;
}

bool
dpl_event_is_queued(struct dpl_event *ev)
{
    return (atomic_read(&ev->ev_queued) == 1);
}

void *
dpl_event_get_arg(struct dpl_event *ev)
{
    return ev->ev_arg;
}
EXPORT_SYMBOL(dpl_event_get_arg);

void
dpl_event_set_arg(struct dpl_event *ev, void *arg)
{
    ev->ev_arg = arg;
}

void
dpl_event_run(struct dpl_event *ev)
{
    if(ev == NULL) return;
    assert(ev->ev_cb != NULL);
    ev->ev_cb(ev);
}
