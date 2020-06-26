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

#ifndef _DPL_H_
#define _DPL_H_

#include <stddef.h>
#include <stdint.h>
#ifndef DPL_CFG_ALIGNMENT
#define DPL_CFG_ALIGNMENT   8
#endif
#ifndef OS_CFG_ALIGN_4
#define OS_CFG_ALIGN_4   0
#endif
#include "syscfg/syscfg.h"
#include "dpl/dpl_atomic.h"
#include "dpl/dpl_callout.h"
#include <dpl/dpl_error.h>
#include "dpl/dpl_eventq.h"
#include "dpl/dpl_mutex.h"
#include <dpl/dpl_types.h>
#include "dpl/dpl_os.h"
#include "dpl/dpl_sem.h"
#include "dpl/dpl_tasks.h"
#include "dpl/dpl_time.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef dpl_hw_enter_critical
#define dpl_hw_enter_critical(_X) (0)
#endif
#define dpl_hw_exit_critical(_X) {}

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_H_ */
