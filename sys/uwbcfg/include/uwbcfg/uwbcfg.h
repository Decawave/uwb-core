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

#ifndef __SYS_UWBCFG_H_
#define __SYS_UWBCFG_H_
#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGMT_GROUP_ID_UWBCFG   (0x103)

typedef int (*uwbcfg_update_handler_t)(void);

struct uwbcfg_cbs {
    SLIST_ENTRY(uwbcfg_cbs) uc_list;
    uwbcfg_update_handler_t uc_update;
};

SLIST_HEAD(uwbcfg_cbs_head, uwbcfg_cbs);

int uwbcfg_register(struct uwbcfg_cbs *handler);
int uwbcfg_apply(void);
void uwbcfg_pkg_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
