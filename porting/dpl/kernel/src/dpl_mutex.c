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
dpl_mutex_init(struct dpl_mutex *mu)
{
    if (!mu) {
        return DPL_INVALID_PARAM;
    }
    mutex_init(&mu->lock);
    return DPL_OK;
}

dpl_error_t
dpl_mutex_release(struct dpl_mutex *mu)
{
    if (!mu) {
        return DPL_INVALID_PARAM;
    }

    mutex_unlock(&mu->lock);
    return DPL_OK;
}

dpl_error_t
dpl_mutex_pend(struct dpl_mutex *mu, uint32_t timeout)
{
    int rc = DPL_OK;
    if (!mu) {
        return DPL_INVALID_PARAM;
    }

    if (timeout == DPL_WAIT_FOREVER) {
        rc = mutex_lock_interruptible(&mu->lock);
    } else {
        /* XXX TODO: No real timeout equivalent in kernel? */
        rc = mutex_lock_interruptible(&mu->lock);
    }
    return rc;

}
