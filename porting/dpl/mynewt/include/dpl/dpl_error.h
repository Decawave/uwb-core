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

#ifndef _DPL_ERROR_H_
#define _DPL_ERROR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum dpl_error {
    DPL_OK = 0,
    DPL_ENOMEM = 1,
    DPL_EINVAL = 2,
    DPL_INVALID_PARAM = 3,
    DPL_MEM_NOT_ALIGNED = 4,
    DPL_BAD_MUTEX = 5,
    DPL_TIMEOUT = 6,
    DPL_ERR_IN_ISR = 7,
    DPL_ERR_PRIV = 8,
    DPL_OS_NOT_STARTED = 9,
    DPL_ENOENT = 10,
    DPL_EBUSY = 11,
    DPL_ERROR = 12,
};

typedef enum dpl_error dpl_error_t;

#ifdef __cplusplus
}
#endif

#endif  /* __DPL_ERROR_H_ */
