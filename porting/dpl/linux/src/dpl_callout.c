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
#include <signal.h>
#include <time.h>

#include "dpl/dpl_callout.h"

static void
dpl_callout_timer_cb(union sigval sv)
{
    struct dpl_callout *c = (struct dpl_callout *)sv.sival_ptr;
    assert(c);

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
    struct sigevent         event;

    /* Initialize the callout. */
    memset(c, 0, sizeof(*c));
    c->c_ev.ev_cb = ev_cb;
    c->c_ev.ev_arg = ev_arg;
    c->c_evq = evq;
    c->c_active = false;

    event.sigev_notify = SIGEV_THREAD;
    event.sigev_value.sival_ptr = c;     // put callout obj in signal args
    event.sigev_notify_function = dpl_callout_timer_cb;
    event.sigev_notify_attributes = NULL;

    timer_create(CLOCK_REALTIME, &event, &c->c_timer);
}

bool dpl_callout_is_active(struct dpl_callout *c)
{
    // TODO: seek native posix method to determine whether timer_t is active.
    // TODO: fix bug where one-shot timer is still active after fired.
    return c->c_active;
}

int dpl_callout_inited(struct dpl_callout *c)
{
    return (c->c_timer != NULL);
}

dpl_error_t dpl_callout_reset(struct dpl_callout *c,
				      dpl_time_t ticks)
{
    struct itimerspec its;

    if (ticks < 0) {
        return DPL_EINVAL;
    }

    if (ticks == 0) {
        ticks = 1;
    }

    c->c_ticks = dpl_time_get() + ticks;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;                     // one shot
    its.it_value.tv_sec = (ticks / 1000000);
    its.it_value.tv_nsec = (ticks % 1000000) * 1000; // expiration
    its.it_value.tv_nsec %= 1000000000;
    c->c_active = true;
    timer_settime(c->c_timer, 0, &its, NULL);

    return DPL_OK;
}

int dpl_callout_queued(struct dpl_callout *c)
{
    struct itimerspec its;
    timer_gettime(c->c_timer, &its);

    return ((its.it_value.tv_sec > 0) ||
            (its.it_value.tv_nsec > 0));
}

void dpl_callout_stop(struct dpl_callout *c)
{
    if (!dpl_callout_inited(c)) {
        return;
    }

    struct itimerspec its;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    timer_settime(c->c_timer, 0, &its, NULL);
    c->c_active = false;
}

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
    dpl_time_t rt;
    uint32_t exp;

    struct itimerspec its;
    timer_gettime(co->c_timer, &its);

    exp = its.it_value.tv_sec * 1000000;

    if (exp > now) {
        rt = exp - now;
    } else {
        rt = 0;
    }

    return rt;
}
