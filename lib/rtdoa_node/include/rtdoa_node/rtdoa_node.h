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

#ifndef _UWBEXT_RTDOA_NODE_H_
#define _UWBEXT_RTDOA_NODE_H_

#if MYNEWT_VAL(RTDOA_NODE_ENABLED)

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>

struct uwb_dev_status rtdoa_request(struct rtdoa_instance *rtdoa, uint64_t delay);

void rtdoa_node_pkg_init(void);
void rtdoa_node_free(struct uwb_dev * inst);
struct uwb_rng_config * rtdoa_node_config(struct uwb_dev * inst);

#ifdef __cplusplus
}
#endif

#endif // RTDOA_NODE_ENABLED
#endif //_UWBEXT_RTDOA_NODE_H_
