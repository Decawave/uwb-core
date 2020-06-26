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

#include "os/mynewt.h"

#if MYNEWT_VAL(PANMASTER_FCB)

#include <fcb/fcb.h>
#include <string.h>
#include <stdio.h>

#include "panmaster/panmaster.h"
#include "panmaster/panmaster_fcb.h"
#include "panmaster_priv.h"

#define PANM_FCB_VERS		2

struct panm_fcb_load_cb_arg {
    panm_load_cb cb;
    void *cb_arg;
};

int
panm_fcb_src(struct panm_fcb *pm)
{
    int rc;

    pm->pm_fcb.f_version = PANM_FCB_VERS;
    pm->pm_fcb.f_scratch_cnt = 1;

    while (1) {
        rc = fcb_init(&pm->pm_fcb);
        if (rc) {
            return OS_INVALID_PARM;
        }

        /*
         * Check if system was reset in middle of emptying a sector. This
         * situation is recognized by checking if the scratch block is missing.
         */
        if (fcb_free_sector_cnt(&pm->pm_fcb) < 1) {
            flash_area_erase(pm->pm_fcb.f_active.fe_area, 0,
              pm->pm_fcb.f_active.fe_area->fa_size);
        } else {
            break;
        }
    }

    return OS_OK;
}


static int
fcb_load_cb(struct fcb_entry *loc, void *arg)
{
    struct panm_fcb_load_cb_arg *argp;
    struct panmaster_node tmpnode;
    int rc;
    memset(&tmpnode,0,sizeof(struct panmaster_node));

    argp = (struct panm_fcb_load_cb_arg *)arg;

    if (loc->fe_data_len != sizeof(struct panmaster_node) &&
        loc->fe_data_len != sizeof(struct panmaster_node) - sizeof(struct pan_image_version)) {
        return 1;
    }

    rc = flash_area_read(loc->fe_area, loc->fe_data_off, &tmpnode, loc->fe_data_len);
    if (rc) {
        return 0;
    }

    argp->cb(&tmpnode, argp->cb_arg);
    return 0;
}

int
panm_fcb_load(struct panm_fcb *pm, panm_load_cb cb, void *cb_arg)
{
    struct panm_fcb_load_cb_arg arg;
    int rc;

    arg.cb = cb;
    arg.cb_arg = cb_arg;
    rc = fcb_walk(&pm->pm_fcb, 0, fcb_load_cb, &arg);
    if (rc) {
        return OS_EINVAL;
    }
    return OS_OK;
}

static void
node_idx_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct panmaster_node_idx *nodes = (struct panmaster_node_idx*)cb_arg;

    if (node->index <= MYNEWT_VAL(PANMASTER_MAXNUM_NODES)) {
        nodes[node->index].addr = node->addr;
        nodes[node->index].euid = node->euid;
        nodes[node->index].role = node->role;
        nodes[node->index].has_perm_slot = node->has_perm_slot;
        if (node->has_perm_slot) {
            nodes[node->index].slot_id = node->slot_id;
            nodes[node->index].lease_ends = 0;
        }
    }
}

int
panm_fcb_load_idx(struct panm_fcb *pm, struct panmaster_node_idx *nodes)
{
    return panm_fcb_load(pm, node_idx_load_cb, (void*)nodes);
}

static void
find_node_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct find_node_s *fn = (struct find_node_s*)cb_arg;

    if ((fn->find.index != 0     && fn->find.index == node->index) ||
        (fn->find.addr != 0xffff && fn->find.addr == node->addr) ||
        (fn->find.euid != 0 && fn->find.euid == node->euid)) {
        memcpy(fn->results, node, sizeof(struct panmaster_node));
        fn->is_found = 1;
    }
}

int
panm_fcb_find_node(struct panm_fcb *pf, struct find_node_s *fns)
{
    fns->is_found = 0;
    return panm_fcb_load(pf, find_node_load_cb, (void*)fns);
}


void
panm_fcb_compress(struct panm_fcb *pm)
{
    int rc;
    struct panmaster_node buf1;
    struct panmaster_node buf2;
    struct fcb_entry loc1;
    struct fcb_entry loc2;
    int copy;

    rc = fcb_append_to_scratch(&pm->pm_fcb);
    if (rc) {
        return; /* XXX */
    }

    loc1.fe_area = NULL;
    loc1.fe_elem_off = 0;
    while (fcb_getnext(&pm->pm_fcb, &loc1) == 0) {
        if (loc1.fe_area != pm->pm_fcb.f_oldest) {
            break;
        }
        rc = flash_area_read(loc1.fe_area, loc1.fe_data_off, &buf1,
                             loc1.fe_data_len);
        if (rc) {
            continue;
        }
        loc2 = loc1;
        copy = 1;
        while (fcb_getnext(&pm->pm_fcb, &loc2) == 0) {
            if (loc2.fe_area == pm->pm_fcb.f_oldest) {
                continue;
            }
            rc = flash_area_read(loc2.fe_area, loc2.fe_data_off, &buf2,
                                 loc2.fe_data_len);
            if (rc) {
                continue;
            }
            if (buf1.euid == buf2.euid) {
                copy = 0;
                break;
            }
        }
        if (!copy) {
            continue;
        }

        /*
         * Can't find one. Must copy.
         */
        rc = flash_area_read(loc1.fe_area, loc1.fe_data_off, &buf1,
                             loc1.fe_data_len);
        if (rc) {
            continue;
        }
        rc = fcb_append(&pm->pm_fcb, sizeof(struct panmaster_node), &loc2);
        if (rc) {
            continue;
        }
        rc = flash_area_write(loc2.fe_area, loc2.fe_data_off, &buf1,
                              sizeof(struct panmaster_node));
        if (rc) {
            continue;
        }
        fcb_append_finish(&pm->pm_fcb, &loc2);
    }
    rc = fcb_rotate(&pm->pm_fcb);
    if (rc) {
        /* XXXX */
        ;
    }
}

static int
panm_fcb_append(struct panm_fcb *pm, uint8_t *buf, int len)
{
    int rc;
    int i;
    struct fcb_entry loc;

    for (i = 0; i < 10; i++) {
        rc = fcb_append(&pm->pm_fcb, len, &loc);
        if (rc != FCB_ERR_NOSPACE) {
            break;
        }
        panm_fcb_compress(pm);
    }
    if (rc) {
        return OS_EINVAL;
    }
    rc = flash_area_write(loc.fe_area, loc.fe_data_off, buf, len);
    if (rc) {
        return OS_EINVAL;
    }
    fcb_append_finish(&pm->pm_fcb, &loc);
    return OS_OK;
}

int
panm_fcb_save(struct panm_fcb *pm, struct panmaster_node *node)
{
    if (!node) {
        return OS_INVALID_PARM;
    }

    return panm_fcb_append(pm, (uint8_t*)node, sizeof(struct panmaster_node));
}

int
panm_fcb_clear(struct panm_fcb *pm)
{
    return fcb_clear(&pm->pm_fcb);
}

static void
sort_nodes_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct panmaster_node *nodes = (struct panmaster_node*)cb_arg;

    if (node->index < MYNEWT_VAL(PANMASTER_MAXNUM_NODES)) {
        memcpy(&nodes[node->index], node, sizeof(struct panmaster_node));
    }
}

void
panm_fcb_sort(struct panm_fcb *pm)
{
    int i;
    int index = 0;
    int laddr = 0xffff;
    int laddr_i;
    struct panmaster_node *nodes;
    int lne_nodes_sz = sizeof(struct panmaster_node)*
        MYNEWT_VAL(PANMASTER_MAXNUM_NODES);

    nodes = (struct panmaster_node*)malloc(lne_nodes_sz);
    if (!nodes) {
        return;
    }

    memset(nodes, 0xff, lne_nodes_sz);
    panmaster_load(sort_nodes_load_cb, nodes);

    /* Clear nodes storage area */
    panm_fcb_clear(pm);

    /* Save nodes from memory in sorted order */
    for(index=0;index<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);index++) {
        laddr = 0xffff;

        /* find lowest addr of all nodes */
        for(i=0;i<MYNEWT_VAL(PANMASTER_MAXNUM_NODES);i++) {
            if (nodes[i].addr == 0xffff) {
                continue;
            }
            if (nodes[i].addr < laddr) {
                laddr = nodes[i].addr;
                laddr_i = i;
            }
        }
        if (laddr == 0xffff) {
            break;
        }

        nodes[laddr_i].index = index;
        panm_fcb_save(pm, &nodes[laddr_i]);
        nodes[laddr_i].addr = 0xffff;
    }

    free(nodes);
}

#endif
