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

#include <dpl/dpl.h>
#include <linux/errno.h>

dpl_error_t
dpl_sem_init(struct dpl_sem *sem, uint16_t tokens)
{
    if (!sem) {
        return DPL_INVALID_PARAM;
    }

    sema_init(&sem->lock, tokens);
    return DPL_OK;
}
EXPORT_SYMBOL(dpl_sem_init);

dpl_error_t
dpl_sem_release(struct dpl_sem *sem)
{
    if (!sem) {
        return DPL_INVALID_PARAM;
    }

    up(&sem->lock);

    return DPL_OK;
}
EXPORT_SYMBOL(dpl_sem_release);

static uint16_t
get_count(struct semaphore *sem)
{
    int c=0;
    unsigned long flags;

    raw_spin_lock_irqsave(&sem->lock, flags);
    c=sem->count;
    raw_spin_unlock_irqrestore(&sem->lock, flags);
    return c;
}

uint16_t
dpl_sem_get_count(struct dpl_sem *sem)
{
    if (!sem) {
        return DPL_INVALID_PARAM;
    }
    return (get_count(&sem->lock));
}
EXPORT_SYMBOL(dpl_sem_get_count);

dpl_error_t
dpl_sem_pend(struct dpl_sem *sem, dpl_time_t timeout)
{
    int rc = DPL_OK;
    if (!sem) {
        return DPL_INVALID_PARAM;
    }
    if (timeout == DPL_WAIT_FOREVER) {
        rc = down_interruptible(&sem->lock);
    } else {
        rc = down_timeout(&sem->lock, (timeout * HZ)/DPL_TICKS_PER_SEC);
    }
    return rc;
}
EXPORT_SYMBOL(dpl_sem_pend);
