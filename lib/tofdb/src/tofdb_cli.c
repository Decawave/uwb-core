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

#include "uwb_rng/uwb_rng.h"
#include "tofdb/tofdb.h"

struct tofdb_node* tofdb_get_nodes();

static int tofdb_cli_cmd(int argc, char **argv);

#if MYNEWT_VAL(SHELL_CMD_HELP)
const struct shell_param cmd_tofdb_param[] = {
    {"list", ""},
    {NULL,NULL},
};

const struct shell_cmd_help cmd_tofdb_help = {
	"tofdb", "<cmd>", cmd_tofdb_param
};
#endif


static struct shell_cmd shell_tofdb_cmd = {
    .sc_cmd = "tdb",
    .sc_cmd_func = tofdb_cli_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
    .help = &cmd_tofdb_help
#endif
};


static void
list_nodes()
{
    int i;
    struct tofdb_node *nodes;
    struct os_timeval tv;

    nodes = tofdb_get_nodes();
    console_printf("#idx, addr,    tof,  tof(m),    #,  stddev, age(s)\n");
    for (i=0;i<MYNEWT_VAL(TOFDB_MAXNUM_NODES);i++) {
        if (!nodes[i].addr) {
            continue;
        }
        console_printf("%4d, ", i);
        console_printf("%4x, ", nodes[i].addr);
        console_printf("%6ld, ", (uint32_t)nodes[i].tof);
        float ave = nodes[i].tof;
        float stddev = uwb_rng_tof_to_meters((uint32_t)sqrtf(nodes[i].sum_sq/nodes[i].num - ave*ave));
        ave = uwb_rng_tof_to_meters((uint32_t)(nodes[i].sum/nodes[i].num));
        console_printf("%3d.%03d, ", (int)ave, (int)(fabsf(ave-(int)ave)*1000));
        console_printf("%4ld, ", nodes[i].num);
        if (nodes[i].num>1) {
            console_printf("%3d.%03d, ", (int)stddev, (int)(fabsf(stddev-(int)stddev)*1000));
        } else {
            console_printf("%7s, ","");
        }

        if (nodes[i].last_updated) {
            os_get_uptime(&tv);
            uint32_t age = os_cputime_ticks_to_usecs(os_cputime_get32() -
                                                     nodes[i].last_updated);
            uint32_t age_s = age/1000000;
            console_printf("%4ld.%ld", age_s, (age-1000000*(age_s))/100000);
        }
        console_printf("\n");
    }
}


static int
tofdb_cli_cmd(int argc, char **argv)
{
    if (argc < 2) {
        return 0;
    }
    if (!strcmp(argv[1], "list")) {
        list_nodes();
    } else {
        console_printf("Unknown cmd\n");
    }
    return 0;
}

int
tofdb_cli_register(void)
{
    return shell_cmd_register(&shell_tofdb_cmd);
}
#endif /* MYNEWT_VAL(TOFDB_CLI) */
