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

#include <string.h>
#include <stdio.h>

#include <dpl/dpl.h>
#include <log/log.h>
#include <syscfg/syscfg.h>
#include <sysinit/sysinit.h>
#include <config/config.h>

#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwbcfg/uwbcfg.h>
#include "uwbcfg_priv.h"

#if __KERNEL__
#include <softfloat/softfloat.h>

#else
double strtod_soft( const char *nptr, char **endptr );
#endif


struct log _uwbcfg_log;

static struct uwbcfg_cbs_head uwbcfg_callbacks;

static char *uwbcfg_get(int argc, char **argv, char *val, int val_len_max);
static int uwbcfg_set(int argc, char **argv, char *val);
int uwbcfg_commit(void);
static int uwbcfg_export(void (*export_func)(char *name, char *val),
  enum conf_export_tgt tgt);

char g_uwb_config[CFGSTR_MAX][CFGSTR_STRLEN] = {
    MYNEWT_VAL(UWBCFG_DEF_CH),                /* channel */
#if MYNEWT_VAL(DW1000_DEVICE_0)
    MYNEWT_VAL(UWBCFG_DEF_PRF),               /* prf */
#endif
    MYNEWT_VAL(UWBCFG_DEF_DATARATE),          /* datarate */
    MYNEWT_VAL(UWBCFG_DEF_PACLEN),            /* rx_paclen */
    MYNEWT_VAL(UWBCFG_DEF_RX_PREAM_CIDX),     /* rx_pream_cidx */
    MYNEWT_VAL(UWBCFG_DEF_RX_SFD_TYPE),       /* rx_sfdType */
    MYNEWT_VAL(UWBCFG_DEF_RX_SFD_TO),         /* rx_sfd_to */
    MYNEWT_VAL(UWBCFG_DEF_RX_PHR_MODE),       /* rx_phrMode */
#if MYNEWT_VAL(DW3000_DEVICE_0)
    MYNEWT_VAL(UWBCFG_DEF_RX_PHR_RATE),       /* rx_phrRate */
    MYNEWT_VAL(UWBCFG_DEF_RX_STS_MODE),       /* rx sts mode */
    MYNEWT_VAL(UWBCFG_DEF_RX_STS_LEN),        /* rx sts length */
    MYNEWT_VAL(UWBCFG_DEF_RX_PDOA_MODE),      /* rx pdoa mode */
#endif
    MYNEWT_VAL(UWBCFG_DEF_RX_DIAG_EN),        /* rx diagnostics enable bitfield */
    MYNEWT_VAL(UWBCFG_DEF_TX_PREAM_CIDX),     /* tx_pream_cidx */
    MYNEWT_VAL(UWBCFG_DEF_TX_PREAM_LEN),      /* tx_pream_len */
    MYNEWT_VAL(UWBCFG_DEF_TXRF_POWER_COARSE), /* txrf_power_coarse */
    MYNEWT_VAL(UWBCFG_DEF_TXRF_POWER_FINE),   /* txrf_power_fine */
    MYNEWT_VAL(UWBCFG_DEF_RX_ANTDLY),         /* rx_antdly */
    MYNEWT_VAL(UWBCFG_DEF_TX_ANTDLY),         /* tx_antdly */
    MYNEWT_VAL(UWBCFG_DEF_RX_ANTSEP),            /* rx_ant_separation */
    MYNEWT_VAL(UWBCFG_DEF_EXT_CLKDLY),        /* external clockdelay */
    MYNEWT_VAL(UWBCFG_DEF_ROLE),              /* role */
    MYNEWT_VAL(UWBCFG_DEF_FRAME_FILTER),      /* MAC Frame Filter */
    MYNEWT_VAL(UWBCFG_DEF_XTAL_TRIM),         /* Crystal Trim value, 0xff == not set */
#if MYNEWT_VAL(CIR_ENABLED)
    MYNEWT_VAL(UWBCFG_DEF_CIR_SIZE),          /* Number of bins to extract from CIR */
    MYNEWT_VAL(UWBCFG_DEF_CIR_OFFSET),        /* Offset relative leading edge to start extracting CIR from */
#endif
};

const char* g_uwbcfg_str[] = {
    "channel",
#if MYNEWT_VAL(DW1000_DEVICE_0)
    "prf",
#endif
    "datarate",
    "rx_paclen",
    "rx_pream_cidx",
    "rx_sfdtype",
    "rx_sfd_to",
    "rx_phrmode",
#if MYNEWT_VAL(DW3000_DEVICE_0)
    "rx_phr_rate",
    "rx_sts_mode",
    "rx_sts_len",
    "rx_pdoa_mode",
#endif
    "rx_diag_en",
    "tx_pream_cidx",
    "tx_pream_len",
    "txrf_power_coarse",
    "txrf_power_fine",
    "rx_antdly",
    "tx_antdly",
    "rx_ant_separation",
    "ext_clkdly",
    "role",
    "frame_filter",
    "xtal_trim",
#if MYNEWT_VAL(CIR_ENABLED)
    "cir_size",
    "cir_offs",
#endif
};

static struct conf_handler uwbcfg_handler = {
    .ch_name = "uwb",
    .ch_get = uwbcfg_get,
    .ch_set = uwbcfg_set,
    .ch_commit = uwbcfg_commit,
    .ch_export = uwbcfg_export,
};

static char *
uwbcfg_get(int argc, char **argv, char *val, int val_len_max)
{
    int i;
    if (argc == 1) {
        for (i=0;i<CFGSTR_MAX;i++) {
            if (!strcmp(argv[0], g_uwbcfg_str[i])) return g_uwb_config[i];
        }
    }
    return NULL;
}

static int
uwbcfg_set(int argc, char **argv, char *val)
{
    int i;
    if (argc == 1) {
        for (i=0;i<CFGSTR_MAX;i++) {
            if (!strcmp(argv[0], g_uwbcfg_str[i]))
                return CONF_VALUE_SET(val, CONF_STRING, g_uwb_config[i]);
        }
    }
    return DPL_ENOENT;
}

char*
uwbcfg_internal_get(int idx)
{
    if (idx < CFGSTR_MAX) {
        return g_uwb_config[idx];
    }
    return 0;
}

int
uwbcfg_internal_set(int idx, char* val)
{
    if (idx < CFGSTR_MAX && strlen(val) < sizeof(g_uwb_config[0])) {
        memcpy(g_uwb_config[idx], val, strlen(val));
        return 0;
    }
    return -1;
}

int
uwbcfg_commit_to_inst(struct uwb_dev * inst, char cfg[CFGSTR_MAX][CFGSTR_STRLEN])
{
    uint8_t paclen;
    int sfd_len=0;
    uint16_t preamble_len;
    uint8_t  txP;
    uint16_t sfd_timeout;
    struct uwbcfg_cbs *cb;

    conf_value_from_str(cfg[CFGSTR_CH], CONF_INT8, (void*)&(inst->config.channel), 0);

    /* Data rate. Only set sfd_len and attrib here. Remaining updates
     * in hw specific function */
    if (!strcmp(cfg[CFGSTR_DATARATE], "6m8")) {
        sfd_len = 8;
        inst->attrib.Tdsym = DPL_FLOAT32_INIT(0.12821f);
    } else if (!strcmp(cfg[CFGSTR_DATARATE], "850k")) {
        sfd_len = 8;
        inst->attrib.Tdsym = DPL_FLOAT32_INIT(1.02564f);
#if MYNEWT_VAL(DW1000_DEVICE_0)
    } else if (!strcmp(cfg[CFGSTR_DATARATE], "110k")) {
        sfd_len = 64;
        inst->attrib.Tdsym = DPL_FLOAT32_INIT(8.20513f);
#endif
    }

    /* PAC length, updates in hw specific function */
    conf_value_from_str(cfg[CFGSTR_RX_PACLEN], CONF_INT8, (void*)&paclen, 0);
    conf_value_from_str(cfg[CFGSTR_RX_SFDTYPE], CONF_INT8,
                        (void*)&inst->config.rx.sfdType, 0);
    conf_value_from_str(cfg[CFGSTR_RX_SFD_TO], CONF_INT16, (void*)&sfd_timeout, 0);
    inst->config.rx.phrMode = (cfg[CFGSTR_RX_PHRMODE][0] == 's')? DWT_PHRMODE_STD : DWT_PHRMODE_EXT;

    /* RX diagnostics */
    if (inst->rxdiag) {
        conf_value_from_str(cfg[CFGSTR_RX_DIAG_EN], CONF_INT16,
                            (void*)&inst->rxdiag->enabled, 0);
        inst->config.rxdiag_enable = (inst->rxdiag->enabled != 0);
    }

    /* RX and TX preamble codes */
    conf_value_from_str(cfg[CFGSTR_RX_PREAM_CIDX], CONF_INT8,
                        (void*)&(inst->config.rx.preambleCodeIndex), 0);
    conf_value_from_str(cfg[CFGSTR_TX_PREAM_CIDX], CONF_INT8,
                        (void*)&(inst->config.tx.preambleCodeIndex), 0);

    /* Tx Power, sts, and pdoa mode set in hw specific function */

    /* Antenna dlys will be updated in dwX000 automatically next time it wakes up */
    conf_value_from_str(cfg[CFGSTR_RX_ANTDLY], CONF_INT16, (void*)&inst->rx_antenna_delay, 0);
    conf_value_from_str(cfg[CFGSTR_TX_ANTDLY], CONF_INT16, (void*)&inst->tx_antenna_delay, 0);

    /* Antenna separation, used in aoa calculations */
    inst->rx_ant_separation = DPL_FLOAT32_FROM_F64(strtod_soft(cfg[CFGSTR_RX_ANTSEP], 0));
    //pr_info("antsep: "DPL_FLOAT32_PRINTF_PRIM"\n", DPL_FLOAT32_PRINTF_VALS(inst->rx_ant_separation));

    /* External clock delay */
    conf_value_from_str(cfg[CFGSTR_EXT_CLKDLY], CONF_INT32, (void*)&inst->ext_clock_delay, 0);

    /* Role */
    conf_value_from_str(cfg[CFGSTR_ROLE], CONF_INT16, (void*)&inst->role, 0);

    /* MAC Frame filter, using default for dw3000.
     * This will be remapped to dw1000 equivalents */
    conf_value_from_str(cfg[CFGSTR_FRAME_FILTER], CONF_INT16,
                        (void*)&inst->config.rx.frameFilter, 0);

    /* Xtal trim */
    conf_value_from_str(cfg[CFGSTR_XTAL_TRIM], CONF_INT8,
                        (void*)&(inst->config.rx.xtalTrim), 0);

#if MYNEWT_VAL(CIR_ENABLED)
    conf_value_from_str(cfg[CFGSTR_CIR_SIZE], CONF_INT16,
                        (void*)&(inst->config.rx.cirSize), 0);
    inst->config.cir_enable = (inst->config.rx.cirSize > 0);
    if (inst->config.rx.cirSize > 1024) {
        UC_WARN("inv cir_size\n");
        inst->config.rx.cirSize = 1024;
    }
    conf_value_from_str(cfg[CFGSTR_CIR_OFFSET], CONF_INT16,
                        (void*)&(inst->config.rx.cirOffset), 0);
#endif

    /* Preamble */
    txP = inst->config.tx.preambleLength;
    sfd_timeout = inst->config.rx.sfdTimeout;
    conf_value_from_str(cfg[CFGSTR_TX_PREAM_LEN], CONF_INT16,
                        (void*)&preamble_len, 0);

    switch (preamble_len)
    {
    case (64):   txP = DWT_PLEN_64  ;break;
    case (128):  txP = DWT_PLEN_128 ;break;
    case (256):  txP = DWT_PLEN_256 ;break;
    case (512):  txP = DWT_PLEN_512 ;break;
    case (1024): txP = DWT_PLEN_1024;break;
    case (2048): txP = DWT_PLEN_2048;break;
    case (4096): txP = DWT_PLEN_4096;break;
    default:
        UC_WARN("inv preamb_len\n");
        break;
    }

    /* Calculate the SFD timeout */
    if (sfd_timeout < 1) {
        sfd_timeout = (preamble_len + 1 + sfd_len - paclen);
    }
    inst->config.tx.preambleLength = txP;
    inst->config.rx.sfdTimeout = sfd_timeout;
    inst->attrib.nsfd = sfd_len;
    inst->attrib.nsync = preamble_len;

#if MYNEWT_VAL(DW1000_DEVICE_0)
    uwbcfg_commit_to_inst_dw1000(inst, cfg);
#endif
#if MYNEWT_VAL(DW3000_DEVICE_0)
    uwbcfg_commit_to_inst_dw3000(inst, cfg);
#endif
    /* Callback to allow host application to decide when to update config
       of chip */
    SLIST_FOREACH(cb, &uwbcfg_callbacks, uc_list) {
        if (cb->uc_update) {
            cb->uc_update();
        }
    }
    return 0;
}

int
uwbcfg_commit(void)
{
    int i;
    struct uwb_dev *udev;
    for (i = 0; i < MYNEWT_VAL(UWB_DEVICE_MAX); i++) {
        udev = uwb_dev_idx_lookup(i);
        if (!udev) {
            continue;
        }
        uwbcfg_commit_to_inst(udev, g_uwb_config);
    }
    return 0;
}

static int
uwbcfg_export(void (*export_func)(char *name, char *val),
  enum conf_export_tgt tgt)
{
    int i;
    char b[32];
    for (i=0;i<CFGSTR_MAX;i++) {
        snprintf(b, sizeof(b), "%s/%s", uwbcfg_handler.ch_name, g_uwbcfg_str[i]);
        export_func(b, g_uwb_config[i]);
    }
    return 0;
}

int
uwbcfg_register(struct uwbcfg_cbs *handler)
{
    SLIST_INSERT_HEAD(&uwbcfg_callbacks, handler, uc_list);
    return 0;
}

int
uwbcfg_apply(void)
{
    return uwbcfg_commit();
}

int
uwbcfg_pkg_init(void)
{
#ifndef __KERNEL__
    int rc;
    rc = conf_register(&uwbcfg_handler);
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

    /* Init log and Config */
    log_register("uwbcfg", &_uwbcfg_log, &log_console_handler,
                 NULL, LOG_SYSLEVEL);

    SLIST_INIT(&uwbcfg_callbacks);

#if MYNEWT_VAL(UWBCFG_NMGR)
    uwbcfg_nmgr_module_init();
#endif

#if MYNEWT_VAL(UWBCFG_CLI)
    uwbcfg_cli_register();
#endif

#if MYNEWT_VAL(UWBCFG_APPLY_AT_INIT)
    uwbcfg_commit();
#endif
#ifdef __KERNEL__
    uwbcfg_sysfs_init();
#endif
    return 0;
}

int
uwbcfg_pkg_down(int reason)
{
#ifdef __KERNEL__
    uwbcfg_sysfs_deinit();
#endif
    return 0;
}
