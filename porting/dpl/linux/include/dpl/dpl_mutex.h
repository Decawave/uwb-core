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

#ifndef _DPL_MUTEX_H_
#define _DPL_MUTEX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "dpl/dpl_types.h"
#include "dpl/dpl_error.h"

/*
 * Mutexes
 */

dpl_error_t dpl_mutex_init(struct dpl_mutex *mu);
dpl_error_t dpl_mutex_pend(struct dpl_mutex *mu, dpl_time_t timeout);
dpl_error_t dpl_mutex_release(struct dpl_mutex *mu);

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_MUTEX_H_ */
