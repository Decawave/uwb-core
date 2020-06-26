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

#ifndef _BCAST_OTA_H_
#define _BCAST_OTA_H_

#include <inttypes.h>

struct os_mbuf;
struct flash_area;

#define MGMT_GROUP_ID_BOTA   (65)

#define BOTA_FLAGS_SET_PERMANENT (0x0001)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _bcast_ota_mode_t{
    BCAST_MODE_NONE,
    BCAST_MODE_RESET_OFFSET,
    BCAST_MODE_RESEND_END
} bcast_ota_mode_t;

typedef void new_fw_cb(void);
struct os_mbuf_pool;

int bcast_ota_get_packet(int src_slot, bcast_ota_mode_t mode, int max_transfer_unit,
                         struct os_mbuf **rsp, uint64_t flags);
struct os_mbuf* bcast_ota_get_reset_mbuf(void);

void bcast_ota_nmgr_module_init(void);
void bcast_ota_set_mpool(struct os_mbuf_pool *mbuf_pool);

void bcast_ota_set_new_fw_cb(new_fw_cb *cb);

#ifdef __cplusplus
}
#endif

#endif /* _PANMASTER_H */
