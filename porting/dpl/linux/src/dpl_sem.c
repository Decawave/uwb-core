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
#include <errno.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>

#include "dpl/dpl.h"

dpl_error_t
dpl_sem_init(struct dpl_sem *sem, uint16_t tokens)
{
    if (!sem) {
        return DPL_INVALID_PARAM;
    }
    int rc = sem_init(&sem->lock, 0, tokens);

    return DPL_OK;
}

dpl_error_t
dpl_sem_release(struct dpl_sem *sem)
{
    int err;

    if (!sem) {
        return DPL_INVALID_PARAM;
    }

    err = sem_post(&sem->lock);

    return (err) ? DPL_ERROR : DPL_OK;
}


uint16_t
dpl_sem_get_count(struct dpl_sem *sem)
{
    int count;

    assert(sem);
    assert(&sem->lock);
    sem_getvalue(&sem->lock, &count);

    return count;
}




dpl_error_t
dpl_sem_pend(struct dpl_sem *sem, dpl_time_t timeout)
{
    int err = 0;
    struct timespec wait;
    if (!sem) {
        return DPL_INVALID_PARAM;
    }

    if (timeout == DPL_WAIT_FOREVER) {
    rewait_forever:
        err = sem_wait(&sem->lock);
        if (err && errno == EINTR) {
            goto rewait_forever;
        }
    } else {
        err = clock_gettime(CLOCK_REALTIME, &wait);
        if (err) {
            return DPL_ERROR;
        }

        wait.tv_sec  += timeout / DPL_TICKS_PER_SEC;
        wait.tv_nsec += (timeout % DPL_TICKS_PER_SEC) * (1000000000/DPL_TICKS_PER_SEC);
        if (wait.tv_nsec > 1000000000) {
            wait.tv_sec += 1;
            wait.tv_nsec -= 1000000000;
        }
    rewait:
        err = sem_timedwait(&sem->lock, &wait);
        if (err && errno == EINTR) {
            goto rewait;
        }
        if (err && errno == ETIMEDOUT) {
            return DPL_TIMEOUT;
        }
    }

    return (err) ? DPL_ERROR : DPL_OK;
}
