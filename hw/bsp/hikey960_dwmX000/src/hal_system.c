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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#include "os/os_dev.h"
#include "hal/hal_system.h"
#include "hal/hal_flash.h"
#include "native_bsp.h"
#include "native_uart_cfg_priv.h"

void
hal_system_reset(void)
{

#if MYNEWT_VAL(HAL_SYSTEM_RESET_CB)
    hal_system_reset_cb();
#endif

#if MYNEWT_VAL(SELFTEST)
    /* Don't hang in the middle of a unit test. */
    assert(0);
#endif

    while(1);
}

/*
 * When running projects within simulator, it's useful to be able to
 * pass operating info as parameters to simulator program.
 * This routine is meant to parse those.
 */
static void
usage(char *progname, int rc)
{
    const char msg1[] = "Usage: ";
    const char msg2[] =
      "\n [-f flash_file][-u uart_log_file][--uart0 <file>][--uart1 <file>]\n"
      "     -f flash_file tells where binary flash file is located. It gets\n"
      "        created if it doesn't already exist.\n"
      "     -u uart_log_file puts all UART data exchanges into a logfile.\n"
      "     -uart0 uart0_file connects UART0 to character device uart0_file.\n"
      "     -uart1 uart1_file connects UART1 to character device uart1_file.\n";

    rc = write(2, msg1, strlen(msg1));
    rc |= write(2, progname, strlen(progname));
    rc |= write(2, msg2, strlen(msg2));
    exit(rc);
}

void
mcu_sim_parse_args(int argc, char **argv)
{
    int ch;
    char *progname;
    struct option options[] = {
        { "flash",      required_argument,      0, 'f' },
        { "uart_log",   required_argument,      0, 'u' },
        { "help",       no_argument,            0, 'h' },
        { "uart0",      required_argument,      0, 0 },
        { "uart1",      required_argument,      0, 0 },
        { "hwid",       required_argument,      0, 'i' },
        { NULL }
    };
    int opt_idx;
    extern int main(int argc, char **arg);

    progname = argv[0];
    while ((ch = getopt_long(argc, argv, "hf:u:i:", options, &opt_idx)) !=
            -1) {
        switch (ch) {
        case 'f':
            native_flash_file = optarg;
            break;
        case 'u':
            native_uart_log_file = optarg;
            break;
        case 'h':
            usage(progname, 0);
            break;
        case 0:
            switch (opt_idx) {
            case 0:
                native_flash_file = optarg;
                break;
            case 1:
                native_uart_log_file = optarg;
                break;
            case 2:
                usage(progname, 0);
                break;
            case 3:
                native_uart_dev_strs[0] = optarg;
                break;
            case 4:
                native_uart_dev_strs[1] = optarg;
                break;
            default:
                usage(progname, -1);
                break;
            }
            break;
        default:
            usage(progname, -1);
            break;
        }
    }
}

int hal_debugger_connected(void)
{
    return 0;
}

#if 1
void __attribute__((weak)) flash_map_init(void) { /* Dummy */ }
void __attribute__((weak)) log_init(void) { /* Dummy */ }
void __attribute__((weak)) mfg_init(void) { /* Dummy */ }
void __attribute__((weak)) modlog_init(void) { /* Dummy */ }
void __attribute__((weak)) uwb_pan_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) nmgr_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) nmgr_uwb_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) twr_ss_nrng_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) nrng_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) survey_pkg_init(void) { /* Dummy */ }

void __attribute__((weak)) nffs_pkg_init(void) { /* Dummy */ }
void __attribute__((weak)) tu_init(void) { /* Dummy */ }
#endif
