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

#include <assert.h>
#include <string.h>

#include "dpl/dpl.h"

#if MYNEWT_VAL(DPL_SYSCLI_ENABLED)
#include "console/console.h"

#include "streamer/streamer.h"
#include "shell/shell.h"

int
shell_dpl_mpool_display_cmd(const struct shell_cmd *cmd, int argc, char **argv,
                            struct streamer *streamer)
{
    struct dpl_mempool *mp;
    struct dpl_mempool_info omi;
    char *name;
    int found;

    name = NULL;
    found = 0;

    if (argc > 1 && strcmp(argv[1], "")) {
        name = argv[1];
    }

    streamer_printf(streamer, "Mempools: \n");
    mp = NULL;
    streamer_printf(streamer, "%32s %5s %4s %4s %4s\n",
                    "name", "blksz", "cnt", "free", "min");
    while (1) {
        mp = dpl_mempool_info_get_next(mp, &omi);
        if (mp == NULL) {
            break;
        }

        if (name) {
            if (strcmp(name, omi.omi_name)) {
                continue;
            } else {
                found = 1;
            }
        }

        streamer_printf(streamer, "%32s %5d %4d %4d %4d\n", omi.omi_name,
                       omi.omi_block_size, omi.omi_num_blocks,
                       omi.omi_num_free, omi.omi_min_free);
    }

    if (name && !found) {
        streamer_printf(streamer, "Couldn't find a memory pool with name %s\n",
                name);
    }

    return 0;
}

#if MYNEWT_VAL(SHELL_CMD_HELP)
static const struct shell_param mpool_params[] = {
    {"", "mpool name"},
    {NULL, NULL}
};

static const struct shell_cmd_help mpool_help = {
    .summary = "show system mpool",
    .usage = NULL,
    .params = mpool_params,
};
#endif

static const struct shell_cmd dpl_commands[] = {
    SHELL_CMD_EXT("dplmpool", shell_dpl_mpool_display_cmd, &mpool_help),
    { 0 },
};

void
shell_dpl_register(void)
{
    const struct shell_cmd *cmd;
    int rc;

    for (cmd = dpl_commands; cmd->sc_cmd != NULL; cmd++) {
        rc = shell_cmd_register(cmd);
        SYSINIT_PANIC_ASSERT_MSG(
            rc == 0, "Failed to register OS shell commands");
    }
}
#endif
