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

/**
 * @file desense_cli.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2020
 * @brief Command debug interface
 *
 * @details
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_gpio.h>
#include <shell/shell.h>
#include <console/console.h>
#include <console/ticks.h>
#include <streamer/streamer.h>

#include <desense/desense.h>

#ifdef __KERNEL__
void desense_sysfs_init(void);
void desense_sysfs_deinit(void);
void desense_debugfs_init(void);
void desense_debugfs_deinit(void);
#endif

#ifndef __KERNEL__
static int desense_cli_cmd(const struct shell_cmd *cmd, int argc, char **argv, struct streamer *streamer);

#if MYNEWT_VAL(SHELL_CMD_HELP)
const struct shell_param cmd_desense_param[] = {
    {"rx",  "<inst> listen for request"},
    {"req", "[inst] <addr> send request"},
    {"set", "[variable] [value] update test parameters"},
    {"txon", "<inst> <data length> <delay_ns> Enable aggressor tx"},
    {"txoff", "Disable aggressor tx"},
    {"res",  "<inst> show results"},
    {NULL,NULL},
};

const struct shell_cmd_help cmd_desense_help = {
    "desense tst", "desense tests", cmd_desense_param
};
#endif

static struct shell_cmd shell_desense_cmd = SHELL_CMD_EXT("desense", desense_cli_cmd, &cmd_desense_help);

static struct desense_test_parameters test_params = {
    .ms_start_delay = 1000,
    .ms_test_delay = 10,
    .strong_coarse_power = 9,
    .strong_fine_power = 31,
    .n_strong = 5,
    .test_coarse_power = 3,
    .test_fine_power = 9,
    .n_test = 100
};

static void
dump_params(struct desense_test_parameters *tp, struct streamer *streamer)
{
    streamer_printf(streamer, "Desense RF Test parameters:\n");
    streamer_printf(streamer, "  ms_start_delay      %6d\n", tp->ms_start_delay);
    streamer_printf(streamer, "  ms_test_delay       %6d\n", tp->ms_test_delay);
    streamer_printf(streamer, "  strong_coarse_power %6d\n", tp->strong_coarse_power);
    streamer_printf(streamer, "  strong_fine_power   %6d\n", tp->strong_fine_power);
    streamer_printf(streamer, "  n_strong            %6d\n", tp->n_strong);
    streamer_printf(streamer, "  test_coarse_power   %6d\n", tp->test_coarse_power);
    streamer_printf(streamer, "  test_fine_power     %6d\n", tp->test_fine_power);
    streamer_printf(streamer, "  n_test              %6d\n", tp->n_test);
}

static void
desense_cli_too_few_args(struct streamer *streamer)
{
    streamer_printf(streamer, "Too few args\n");
}

static int
desense_cli_cmd(const struct shell_cmd *cmd, int argc, char **argv, struct streamer *streamer)
{
    struct uwb_dev * dev = 0;
    struct uwb_desense_instance * desense = 0;
    uint16_t inst_n = 0;
    uint16_t addr = 0;
    uint32_t value;
    uint32_t delay_ns, length;

    if (argc < 2) {
        desense_cli_too_few_args(streamer);
        return 0;
    }

    if (!strcmp(argv[1], "rx")) {
        if (argc < 3) {
            inst_n=0;
        } else {
            inst_n = strtol(argv[2], NULL, 0);
        }
        dev = uwb_dev_idx_lookup(inst_n);
        if (!dev) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense = (struct uwb_desense_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_RF_DESENSE);
        if (!desense) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense_listen(desense);
        streamer_printf(streamer, "Listening on 0x%04X\n", dev->my_short_address);
    } else if (!strcmp(argv[1], "req")) {
        if (argc < 3) {
            desense_cli_too_few_args(streamer);
            return 0;
        } else if (argc < 4) {
            inst_n = 0;
            addr = strtol(argv[2], NULL, 0);
        } else {
            inst_n = strtol(argv[2], NULL, 0);
            addr = strtol(argv[3], NULL, 0);
        }
        dev = uwb_dev_idx_lookup(inst_n);
        if (!dev) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense = (struct uwb_desense_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_RF_DESENSE);
        if (!desense) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense_send_request(desense, addr, &test_params);
        streamer_printf(streamer, "request sent to 0x%04X\n", addr);
    } else if (!strcmp(argv[1], "set")) {
        if (argc < 3) {
            dump_params(&test_params, streamer);
            return 0;
        } else if (argc < 4) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        value = strtol(argv[3], NULL, 0);
        if (!strcmp(argv[2], "ms_start_delay"))      test_params.ms_start_delay = value;
        if (!strcmp(argv[2], "ms_test_delay"))       test_params.ms_test_delay = value;
        if (!strcmp(argv[2], "strong_coarse_power")) test_params.strong_coarse_power = value;
        if (!strcmp(argv[2], "strong_fine_power"))   test_params.strong_fine_power = value;
        if (!strcmp(argv[2], "n_strong"))            test_params.n_strong = value;
        if (!strcmp(argv[2], "test_coarse_power"))   test_params.test_coarse_power = value;
        if (!strcmp(argv[2], "test_fine_power"))     test_params.test_fine_power = value;
        if (!strcmp(argv[2], "n_test"))              test_params.n_test = value;
        dump_params(&test_params, streamer);
    } else if (!strcmp(argv[1], "txon")) {
        /*  desense txon <inst> <data length> <delay_ns>  */
        if (argc < 5) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        inst_n = strtol(argv[2], NULL, 0);
        length = strtol(argv[3], NULL, 0);
        delay_ns = strtol(argv[4], NULL, 0);

        dev = uwb_dev_idx_lookup(inst_n);
        if (!dev) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense = (struct uwb_desense_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_RF_DESENSE);
        if (!desense) {
            desense_cli_too_few_args(streamer);
            return 0;
        }

        desense_txon(desense, length, delay_ns);
    } else if (!strcmp(argv[1], "txoff")) {
        /*  desense txoff <inst> */
        if (argc < 3) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        inst_n = strtol(argv[2], NULL, 0);

        dev = uwb_dev_idx_lookup(inst_n);
        if (!dev) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense = (struct uwb_desense_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_RF_DESENSE);
        if (!desense) {
            desense_cli_too_few_args(streamer);
            return 0;
        }

        desense_txoff(desense);
    } else if (!strcmp(argv[1], "res")) {
        if (argc < 3) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        inst_n = strtol(argv[2], NULL, 0);

        dev = uwb_dev_idx_lookup(inst_n);
        if (!dev) {
            desense_cli_too_few_args(streamer);
            return 0;
        }
        desense = (struct uwb_desense_instance*)uwb_mac_find_cb_inst_ptr(dev, UWBEXT_RF_DESENSE);
        if (!desense) {
            desense_cli_too_few_args(streamer);
            return 0;
        }

        desense_dump_data(desense, streamer);
        desense_dump_stats(desense, streamer);
    } else {
        streamer_printf(streamer, "Unknown cmd\n");
    }

    return 0;
}
#endif  /* ifndef __KERNEL__ */


int
desense_cli_register(void)
{
    int rc=0;
#ifndef __KERNEL__
    rc = shell_cmd_register(&shell_desense_cmd);
#endif
    return rc;
}

int
desense_cli_down(int reason)
{
    return 0;
}
