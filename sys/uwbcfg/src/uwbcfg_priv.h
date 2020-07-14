/**
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

#ifndef __UWBCFG_PRIV_H_
#define __UWBCFG_PRIV_H_

#include <log/log.h>
#define LOG_MODULE_UWBCFG (92)
#define UC_INFO(...)     LOG_INFO(&_uwbcfg_log, LOG_MODULE_UWBCFG, __VA_ARGS__)
#define UC_DEBUG(...)    LOG_DEBUG(&_uwbcfg_log, LOG_MODULE_UWBCFG, __VA_ARGS__)
#define UC_WARN(...)     LOG_WARN(&_uwbcfg_log, LOG_MODULE_UWBCFG, __VA_ARGS__)
#define UC_ERR(...)      LOG_ERROR(&_uwbcfg_log, LOG_MODULE_UWBCFG, __VA_ARGS__)

enum {
    CFGSTR_CH=0,
#if MYNEWT_VAL(DW1000_DEVICE_0)
    CFGSTR_PRF,
#endif
    CFGSTR_DATARATE,
    CFGSTR_RX_PACLEN,
    CFGSTR_RX_PREAM_CIDX,
    CFGSTR_RX_SFDTYPE,
    CFGSTR_RX_SFD_TO,
    CFGSTR_RX_PHRMODE,
#if MYNEWT_VAL(DW3000_DEVICE_0)
    CFGSTR_RX_PHR_RATE,
    CFGSTR_RX_STS_MODE,
    CFGSTR_RX_STS_LEN,
    CFGSTR_RX_PDOA_MODE,
#endif
    CFGSTR_RX_DIAG_EN,
    CFGSTR_TX_PREAM_CIDX,
    CFGSTR_TX_PREAM_LEN,
    CFGSTR_TXRF_PWR_COARSE,
    CFGSTR_TXRF_PWR_FINE,
    CFGSTR_RX_ANTDLY,
    CFGSTR_TX_ANTDLY,
    CFGSTR_RX_ANTSEP,
    CFGSTR_EXT_CLKDLY,
    CFGSTR_ROLE,
    CFGSTR_FRAME_FILTER,
    CFGSTR_XTAL_TRIM,
#if MYNEWT_VAL(CIR_ENABLED)
    CFGSTR_CIR_SIZE,
    CFGSTR_CIR_OFFSET,
#endif
    CFGSTR_MAX
};


#define CFGSTR_STRLEN (8)
#ifdef __cplusplus
extern "C" {
#endif

struct uwb_dev;
extern struct log _uwbcfg_log;
extern char g_uwb_config[CFGSTR_MAX][CFGSTR_STRLEN];
extern const char* g_uwbcfg_str[CFGSTR_MAX];

int uwbcfg_commit_to_inst_dw1000(struct uwb_dev * inst, char cfg[CFGSTR_MAX][CFGSTR_STRLEN]);
int uwbcfg_commit_to_inst_dw3000(struct uwb_dev * inst, char cfg[CFGSTR_MAX][CFGSTR_STRLEN]);


char* uwbcfg_internal_get(int idx);
int uwbcfg_internal_set(int idx, char* val);
int uwbcfg_commit(void);
void uwbcfg_nmgr_module_init(void);
int uwbcfg_cli_register(void);
void uwbcfg_sysfs_init(void);
void uwbcfg_sysfs_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
