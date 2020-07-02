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
#include <syscfg/syscfg.h>
#include <datetime/datetime.h>

#if MYNEWT_VAL(PANMASTER_CLI)

#include <string.h>

#include <defs/error.h>
#include <flash_map/flash_map.h>
#include <hal/hal_bsp.h>

#include <shell/shell.h>
#include <console/console.h>
#include <streamer/streamer.h>

#include "panmaster/panmaster.h"
#include "panmaster_priv.h"
#include "imgmgr/imgmgr.h"
#include <bootutil/image.h>

static int panmaster_cli_cmd(const struct shell_cmd *cmd, int argc, char **argv, struct streamer *streamer);

#if MYNEWT_VAL(SHELL_CMD_HELP)
const struct shell_param cmd_pm_param[] = {
    {"list", ""},
    {"add", "<euid> [addr] add node"},
    {"del", "<euid> delete node"},
    {"pslot", "<euid> <slot_id> set permanent slot (use slot_id=-1 to remove)"},
    {"role", "<euid> <role> set role)"},
    {"dump", ""},
    {"clear", "erase list"},
    {"compr", ""},
    {"sort", ""},
    {NULL,NULL},
};

const struct shell_cmd_help cmd_pm_help = {
    "panmaster commands", "<cmd>", cmd_pm_param
};
#endif

static struct shell_cmd shell_panmaster_cmd = SHELL_CMD_EXT("panm", panmaster_cli_cmd, &cmd_pm_help);

static void
list_nodes_load_cb(struct panmaster_node *node, void *cb_arg)
{
    struct list_nodes_extract *lne = (struct list_nodes_extract*)cb_arg;

    if (node->index < lne->index_max  &&
        node->index >= lne->index_off) {
        memcpy(&lne->nodes[node->index - lne->index_off], node,
               sizeof(struct panmaster_node));
    }
}

#define LIST_NODES_BLK_NNODES (32)
static void
list_nodes_blk(struct streamer *streamer)
{
    int i,j;
    struct panmaster_node_idx *node_idx;
    struct os_timeval tv;
    struct os_timeval utctime;
    struct os_timezone timezone;
    struct list_nodes_extract lne;
    int lne_nodes_sz = sizeof(struct panmaster_node)*LIST_NODES_BLK_NNODES;
    lne.nodes = (struct panmaster_node*)malloc(lne_nodes_sz);

    if (!lne.nodes) {
        streamer_printf(streamer, "err:mem\n");
        return;
    }

    char ver_str[32];
    int num_nodes;
    char buf[32];

    panmaster_node_idx(&node_idx, &num_nodes);
    os_gettimeofday(&utctime, &timezone);
    streamer_printf(streamer, "#idx, addr, role, slot, p,  lease, euid,             flags,          date-added, fw-ver\n");
    for (i=0;i<num_nodes;i+=LIST_NODES_BLK_NNODES) {
        lne.index_off = i;
        lne.index_max = i+LIST_NODES_BLK_NNODES;
        memset(lne.nodes, 0xffff, lne_nodes_sz);
        panmaster_load(list_nodes_load_cb, &lne);

        for (j=0;j<LIST_NODES_BLK_NNODES;j++) {

            if (lne.nodes[j].addr == 0xffff) {
                continue;
            }

            streamer_printf(streamer, "%4d, ", i+j);
            streamer_printf(streamer, "%4x, ", lne.nodes[j].addr);
            int slot_id = node_idx[lne.nodes[j].index].slot_id;
            if (lne.nodes[j].has_perm_slot) {
                slot_id = lne.nodes[j].slot_id;
            }
            streamer_printf(streamer, "%4X, ", lne.nodes[j].role);
            if (slot_id != 0xffff) {
                streamer_printf(streamer, "%4d, ", slot_id);
            } else {
                streamer_printf(streamer, "    , ");
            }
            streamer_printf(streamer, "%s, ", (lne.nodes[j].has_perm_slot)?"p":" ");

            if (node_idx[lne.nodes[j].index].lease_ends) {
                os_get_uptime(&tv);
                int32_t le_ms = node_idx[lne.nodes[j].index].lease_ends;
                int32_t now_ms = tv.tv_sec*1000 + tv.tv_usec/1000;
                le_ms = le_ms - now_ms;
                if (le_ms < 0) le_ms = 0;
                streamer_printf(streamer, "%4ld.%ld, ", le_ms/1000, (le_ms-1000*(le_ms/1000))/100);
            } else {
                streamer_printf(streamer, "      , ");
            }

            streamer_printf(streamer, "%016llX, ", lne.nodes[j].euid);
            streamer_printf(streamer, "%5X, ", lne.nodes[j].flags);

            utctime.tv_sec = lne.nodes[j].first_seen_utc;
            utctime.tv_usec = 0;
            datetime_format(&utctime, &timezone, buf, sizeof(buf));
            buf[19]='\0';
            streamer_printf(streamer, "%s, ", buf);
            struct image_version fw_ver = {
                .iv_major = lne.nodes[j].fw_ver.iv_major,
                .iv_minor = lne.nodes[j].fw_ver.iv_minor,
                .iv_revision = lne.nodes[j].fw_ver.iv_revision,
                .iv_build_num = lne.nodes[j].fw_ver.iv_build_num,
            };
            imgr_ver_str(&fw_ver, ver_str);

            streamer_printf(streamer, "%s", ver_str);
            streamer_printf(streamer, "\n");
        }
    }
    free(lne.nodes);
}

static void
dump_cb(struct panmaster_node *n, void *cb_arg)
{
    char ver_str[32];
    struct streamer *streamer = (struct streamer *)cb_arg;
    streamer_printf(streamer, "%3d %04x %016llx %x %x %d %d ", n->index, n->addr, n->euid,
                   n->flags, n->role, n->has_perm_slot, n->slot_id);

    struct image_version fw_ver = {
        .iv_major = n->fw_ver.iv_major,
        .iv_minor = n->fw_ver.iv_minor,
        .iv_revision = n->fw_ver.iv_revision,
        .iv_build_num = n->fw_ver.iv_build_num,
    };
    imgr_ver_str(&fw_ver, ver_str);
    streamer_printf(streamer, "%s\n", ver_str);

}

static void
dump(struct streamer *streamer)
{
    streamer_printf(streamer, "# index addr euid flags role pslot slot_id fw-ver\n");
    panmaster_load(dump_cb, streamer);
}

static int
panmaster_cli_cmd(const struct shell_cmd *cmd, int argc, char **argv, struct streamer *streamer)
{
    int rc;
    int slot_id, role;
    uint16_t addr;
    uint64_t euid;
    struct panmaster_node *node;

    if (argc < 2) {
        streamer_printf(streamer, "Too few args\n");
        return 0;
    }
    if (!strcmp(argv[1], "list")) {
        list_nodes_blk(streamer);
    } else if (!strcmp(argv[1], "add")) {
        if (argc < 3) {
            streamer_printf(streamer, "euid needed\n");
            return 0;
        }
        euid = strtoll(argv[2], NULL, 16);
        if (!euid) {
            return 0;
        }
        if (argc > 3) {
            addr = strtoll(argv[3], NULL, 16);
            panmaster_add_node(addr, 0, (uint8_t*)&euid);
            return 0;
        }

        rc = panmaster_idx_find_node(euid, 0, &node);
        panmaster_postprocess();
        if (!rc) {
            streamer_printf(streamer, "Added node euid: 0x%llX, addr 0x%X\n",
                           euid, node->addr);
        } else {
            streamer_printf(streamer, "Add node err\n");
        }
    } else if (!strcmp(argv[1], "del")) {
        if (argc < 3) {
            streamer_printf(streamer, "euid needed\n");
            return 0;
        }
        euid = strtoll(argv[2], NULL, 16);
        panmaster_delete_node(euid);
    } else if (!strcmp(argv[1], "pslot")) {
        if (argc < 4) {
            streamer_printf(streamer, "euid+slot_id needed\n");
            return 0;
        }
        euid = strtoll(argv[2], NULL, 16);
        if (!euid) {
            return 0;
        }
        slot_id = strtoll(argv[3], NULL, 0);

        rc = panmaster_idx_find_node(euid, 0, &node);
        if (!rc) {
            streamer_printf(streamer, "0x%llX: pslot -> ", euid);
            if (slot_id > -1) {
                node->slot_id = slot_id;
                node->has_perm_slot = 1;
                streamer_printf(streamer, "%d\n ", slot_id);
            } else {
                node->slot_id = 0;
                node->has_perm_slot = 0;
                streamer_printf(streamer, "<removed>\n");
            }
            panmaster_save_node(node);
        } else {
            streamer_printf(streamer, "err\n");
        }
    } else if (!strcmp(argv[1], "role")) {
        if (argc < 4) {
            streamer_printf(streamer, "euid+role needed\n");
            return 0;
        }
        euid = strtoll(argv[2], NULL, 16);
        if (!euid) {
            return 0;
        }
        role = strtoll(argv[3], NULL, 0);

        rc = panmaster_idx_find_node(euid, 0, &node);
        if (!rc) {
            streamer_printf(streamer, "0x%llX: role -> %d\n ", euid, role);
            node->role = role;
            panmaster_save_node(node);
        } else {
            streamer_printf(streamer, "err\n");
        }
    } else if (!strcmp(argv[1], "clear")) {
        panmaster_clear_list();
    } else if (!strcmp(argv[1], "compr")) {
        panmaster_compress();
    } else if (!strcmp(argv[1], "sort")) {
        panmaster_sort();
    } else if (!strcmp(argv[1], "dump")) {
        dump(streamer);
    } else {
        streamer_printf(streamer, "Unknown cmd\n");
    }
    return 0;
}

int
panmaster_cli_register(void)
{
    return shell_cmd_register(&shell_panmaster_cmd);
}
#endif /* MYNEWT_VAL(PANMASTER_CLI) */
