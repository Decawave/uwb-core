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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "dpl/dpl_callout.h"
#define slog(fmt, ...) \
    pr_info("uwbcore:%s():%d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

static void dpl_callout_work_handler(struct work_struct *work)
{
    struct dpl_callout *c;
    struct delayed_work *dlyd_work;
    dlyd_work = to_delayed_work(work);
    c = container_of(dlyd_work, struct dpl_callout, dlyd_work);

    c->c_active = false;
    if (c->c_evq) {
        dpl_eventq_put(c->c_evq, &c->c_ev);
    } else {
        c->c_ev.ev_cb(&c->c_ev);
    }
}


void dpl_callout_init(struct dpl_callout *c,
                      struct dpl_eventq *evq,
                      dpl_event_fn *ev_cb,
                      void *ev_arg)
{
    /* Initialize the callout. */
    memset(c, 0, sizeof(*c));
    INIT_DELAYED_WORK(&c->dlyd_work, dpl_callout_work_handler);
    dpl_event_init(&c->c_ev, ev_cb, ev_arg);
    c->c_evq = evq;
    c->c_active = false;
}
EXPORT_SYMBOL(dpl_callout_init);

bool dpl_callout_is_active(struct dpl_callout *c)
{
    return c->c_active;
}

int dpl_callout_inited(struct dpl_callout *c)
{
    return (c->c_ev.ev_cb != NULL);
}

int dpl_callout_queued(struct dpl_callout *c)
{
    return delayed_work_pending(&c->dlyd_work);
}

dpl_error_t dpl_callout_reset(struct dpl_callout *c, dpl_time_t ticks)
{
    if (ticks < 0) {
        return DPL_EINVAL;
    }

    if (dpl_callout_queued(c)) {
        cancel_delayed_work_sync(&c->dlyd_work);
    }
    if (ticks == 0) {
        ticks = 1;
    }
    c->c_active = 1;
    c->c_ticks = dpl_time_get() + ticks;

    if (!c->c_evq->wq) {
        /* If the eventq isn't set, use the default work queue */
        schedule_delayed_work(&c->dlyd_work,
                              (ticks * HZ)/DPL_TICKS_PER_SEC);
    } else {
        queue_delayed_work(c->c_evq->wq, &c->dlyd_work,
                           (ticks * HZ)/DPL_TICKS_PER_SEC);
    }

    return DPL_OK;
}
EXPORT_SYMBOL(dpl_callout_reset);

void dpl_callout_stop(struct dpl_callout *c)
{
    if (!dpl_callout_inited(c)) {
        return;
    }
    cancel_delayed_work_sync(&c->dlyd_work);
    c->c_active = false;
}
EXPORT_SYMBOL(dpl_callout_stop);

dpl_time_t
dpl_callout_get_ticks(struct dpl_callout *co)
{
    return co->c_ticks;
}

void
dpl_callout_set_arg(struct dpl_callout *co, void *arg)
{
    co->c_ev.ev_arg = arg;
}

uint32_t
dpl_callout_remaining_ticks(struct dpl_callout *co,
                            dpl_time_t now)
{
    dpl_time_t rt = co->c_ticks - now;
    return rt;
}
