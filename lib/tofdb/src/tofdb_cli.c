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

#include <os/mynewt.h>
#include <syscfg/syscfg.h>
#include <datetime/datetime.h>

#if MYNEWT_VAL(TOFDB_CLI)

#include <string.h>
#include <math.h>

#include <defs/error.h>

#include <shell/shell.h>
#include <console/console.h>
#include <streamer/streamer.h>

#include "rng_math/rng_math.h"
#include "tofdb/tofdb.h"

struct tofdb_node* tofdb_get_nodes();

static int tofdb_cli_cmd(const struct shell_cmd *cmd, int argc, char **argv, struct streamer *streamer);

#if MYNEWT_VAL(SHELL_CMD_HELP)
const struct shell_param cmd_tofdb_param[] = {
    {"list", ""},
    {NULL,NULL},
};

const struct shell_cmd_help cmd_tofdb_help = {
	"tofdb", "<cmd>", cmd_tofdb_param
};
#endif

static struct shell_cmd shell_tofdb_cmd = SHELL_CMD_EXT("tdb", tofdb_cli_cmd, &cmd_tofdb_help);

static void
list_nodes(struct streamer *streamer)
{
    int i;
    struct tofdb_node *nodes;
    struct os_timeval tv;

    nodes = tofdb_get_nodes();
    streamer_printf(streamer, "#idx, addr,    tof,  tof(m),    #,  stddev, age(s)\n");
    for (i=0;i<MYNEWT_VAL(TOFDB_MAXNUM_NODES);i++) {
        if (!nodes[i].addr) {
            continue;
        }
        streamer_printf(streamer, "%4d, ", i);
        streamer_printf(streamer, "%4x, ", nodes[i].addr);
        streamer_printf(streamer, "%6ld, ", (uint32_t)nodes[i].tof);
        float ave = nodes[i].tof;
        float stddev = uwb_rng_tof_to_meters((uint32_t)sqrtf(nodes[i].sum_sq/nodes[i].num - ave*ave));
        ave = uwb_rng_tof_to_meters((uint32_t)(nodes[i].sum/nodes[i].num));
        streamer_printf(streamer, "%3d.%03d, ", (int)ave, (int)(fabsf(ave-(int)ave)*1000));
        streamer_printf(streamer, "%4ld, ", nodes[i].num);
        if (nodes[i].num>1) {
            streamer_printf(streamer, "%3d.%03d, ", (int)stddev, (int)(fabsf(stddev-(int)stddev)*1000));
        } else {
            streamer_printf(streamer, "%7s, ","");
        }

        if (nodes[i].last_updated) {
            os_get_uptime(&tv);
            uint32_t age = os_cputime_ticks_to_usecs(os_cputime_get32() -
                                                     nodes[i].last_updated);
            uint32_t age_s = age/1000000;
            streamer_printf(streamer, "%4ld.%ld", age_s, (age-1000000*(age_s))/100000);
        }
        streamer_printf(streamer, "\n");
    }
}


static int
tofdb_cli_cmd(const struct shell_cmd *cmd, int argc, char **argv, struct streamer *streamer)
{
    if (argc < 2) {
        return 0;
    }
    if (!strcmp(argv[1], "list")) {
        list_nodes(streamer);
    } else {
        streamer_printf(streamer, "Unknown cmd\n");
    }
    return 0;
}

int
tofdb_cli_register(void)
{
    return shell_cmd_register(&shell_tofdb_cmd);
}
#endif /* MYNEWT_VAL(TOFDB_CLI) */
