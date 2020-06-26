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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "dpl/dpl.h"

dpl_error_t
dpl_mutex_init(struct dpl_mutex *mu)
{
    if (!mu) {
        return DPL_INVALID_PARAM;
    }

    pthread_mutexattr_init(&mu->attr);
    pthread_mutexattr_settype(&mu->attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mu->lock, &mu->attr);

    return DPL_OK;
}

dpl_error_t
dpl_mutex_release(struct dpl_mutex *mu)
{
    if (!mu) {
        return DPL_INVALID_PARAM;
    }

    if (pthread_mutex_unlock(&mu->lock)) {
        return DPL_BAD_MUTEX;
    }

    return DPL_OK;
}

dpl_error_t
dpl_mutex_pend(struct dpl_mutex *mu, uint32_t timeout)
{
    int err;

    if (!mu) {
        return DPL_INVALID_PARAM;
    }

    if (timeout == DPL_WAIT_FOREVER) {
        err = pthread_mutex_lock(&mu->lock);
    } else {
        err = clock_gettime(CLOCK_REALTIME, &mu->wait);
        if (err) {
            return DPL_ERROR;
        }

        mu->wait.tv_sec  += timeout / DPL_TICKS_PER_SEC;
        mu->wait.tv_nsec += (timeout % DPL_TICKS_PER_SEC) * (1000000000/DPL_TICKS_PER_SEC);
        if (mu->wait.tv_nsec > 1000000000) {
            mu->wait.tv_sec += 1;
            mu->wait.tv_nsec -= 1000000000;
        }

        err = pthread_mutex_timedlock(&mu->lock, &mu->wait);
        if (err == ETIMEDOUT) {
            return DPL_TIMEOUT;
        }
    }

    return (err) ? DPL_ERROR : DPL_OK;
}
