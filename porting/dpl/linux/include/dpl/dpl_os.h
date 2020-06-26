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

#ifndef _DPL_OS_H_
#define _DPL_OS_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "dpl/dpl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_WAIT_FOREVER (-1)
#define DPL_ALIGNMENT 4
#define DPL_OS_ALIGNMENT (DPL_ALIGNMENT)

#define SYSINIT_PANIC_MSG(msg) __assert_fail(msg, __FILE__, __LINE__, __func__)

#define SYSINIT_PANIC_ASSERT_MSG(rc, msg) do \
{                                            \
    if (!(rc)) {                             \
        SYSINIT_PANIC_MSG(msg);              \
    }                                        \
} while (0)

#define DPL_ALIGN(__n, __a) (                             \
        (((__n) & ((__a) - 1)) == 0)                   ? \
            (__n)                                      : \
            ((__n) + ((__a) - ((__n) & ((__a) - 1))))    \
        )

typedef uint32_t dpl_sr_t;
#define DPL_ENTER_CRITICAL(_sr) (_sr = dpl_hw_enter_critical())
#define DPL_EXIT_CRITICAL(_sr) (dpl_hw_exit_critical(_sr))
#define DPL_ASSERT_CRITICAL() assert(dpl_hw_is_in_critical())

#ifdef __cplusplus
}
#endif

#endif  /* _DPL_OS_H_ */
