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

#ifndef _PANMASTER_H_
#define _PANMASTER_H_

#include <inttypes.h>
#include "pan_utils/pan_utils.h"
#include "fcb/fcb.h"

struct panmaster_node {
    int64_t  first_seen_utc; /*!< When this node was first seen */
    int64_t  euid;           /*!< Unique id, 64bit */
    uint16_t addr;           /*!< Local id, 16bit */
    union {
        struct {
            uint16_t dummy:11;        /*!< Unused flags */
            uint16_t has_perm_slot:1; /*!< Has Permanent slot */
            uint16_t role:4;          /*!< Network role */
        } __attribute__((__packed__, aligned(1)));
        uint16_t flags;
    };
    uint8_t  index;          /*!< Index into vector */
    uint16_t slot_id;        /*!< slot_id, stored in flash if permanent */
    struct pan_image_version fw_ver;
} __attribute__((__packed__, aligned(1)));

struct panmaster_node_idx {
    uint16_t addr;           /*!< Local id, 16bit */
    uint16_t slot_id;
    int64_t  euid;           /*!< Unique id, 64bit */
    uint8_t role;
    uint16_t has_perm_slot:1; /*!< Has Permanent slot */
    uint16_t save_needed:1;   /*!< Flash needs updating */
    uint32_t lease_ends;
};

struct find_node_s {
    struct panmaster_node find;
    struct panmaster_node *results;
    int is_found;
};


#define PANMASTER_NODE_DEFAULT(N)  {(N).first_seen_utc=0;(N).euid=0;    \
        (N).addr=0xffff;(N).flags=0;(N).has_perm_slot=0;(N).index=0;(N).slot_id=0xffff;(N).role=0; \
        (N).fw_ver.iv_major=0;(N).fw_ver.iv_minor=0;                    \
        (N).fw_ver.iv_revision=0;(N).fw_ver.iv_build_num=0;}
#define PANMASTER_NODE_IDX_DEFAULT(N)  {(N).addr=0xffff;(N).slot_id=0xffff;(N).role=0;}

typedef void (*panm_load_cb)(struct panmaster_node *node, void *cb_arg);

#ifdef __cplusplus
extern "C" {
#endif

bool panrequest_cb(uint64_t euid, struct pan_req_resp *request,
                                                struct pan_req_resp *response);
void postprocess_cb(struct dpl_event * ev);

void panmaster_pkg_init(void);
int panmaster_idx_find_node(uint64_t euid, uint16_t role, struct panmaster_node **node);
int panmaster_find_node_general(struct find_node_s *fns);

int panmaster_load(panm_load_cb cb, void *cb_arg);
void panmaster_save(void);
int panmaster_save_node(struct panmaster_node *node);
void panmaster_update_node(uint64_t euid, struct panmaster_node *upd_node);

void panmaster_node_idx(struct panmaster_node_idx **node_idx_arg, int *num_nodes);
int panmaster_clear_list();
void panmaster_postprocess(void);

void panmaster_add_version(uint64_t euid, struct pan_image_version *ver);
void panmaster_add_node(uint16_t short_addr, uint16_t role, uint8_t *euid_u8);
void panmaster_delete_node(uint64_t euid);

void panmaster_compress();
void panmaster_sort();
uint16_t panmaster_highest_node_addr();

#if MYNEWT_VAL(PANMASTER_CBOR_EXPORT)
struct os_mbuf* panmaster_cbor_nodes_list(struct os_mbuf_pool *mbuf_pool);
int panmaster_cbor_nodes_list_fa(const struct flash_area *fa, int *fa_offset);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PANMASTER_H */
