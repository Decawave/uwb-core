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

#include <string.h>
#include <stdio.h>

#include <syscfg/syscfg.h>
#include <config/config.h>

#include <uwb/uwb.h>
#include <uwb/uwb_mac.h>
#include <uwbcfg/uwbcfg.h>
#include "uwbcfg_priv.h"
#if MYNEWT_VAL(DW1000_DEVICE_0)
#include <dw1000/dw1000_hal.h>
#endif

/** Differing from dw1000_power_value in that fine is interpreted as integer steps
 *  Thus to get 15.5dB from fine, set FINE to 31 */
#define power_value(COARSE,FINE) ((COARSE<<5) + FINE)

#if MYNEWT_VAL(DW1000_DEVICE_0)
static void
check_preamble_code(struct uwb_dev * inst, uint8_t *arg_code)
{
    int new_code = 0;
    int ch = inst->config.channel;
    int prf = inst->config.prf;
    uint8_t code = *arg_code;
    if (prf == DWT_PRF_16M) {
        if (ch == 1 && code != 1 && code != 2)              new_code = 1;
        if ((ch == 2 || ch==5) && code != 3 && code != 4)   new_code = 3;
        if (ch == 3 && code != 5 && code != 6)              new_code = 5;
        if ((ch == 4 || ch == 7) && code != 7 && code != 8) new_code = 7;
    } else {
        if (ch == 1 || ch==2 || ch==3 || ch==5) {
            if (code < 9 || code > 12)  new_code = 9;
        } else { /* channels 4 and 7 */
            if (code < 17 || code > 20) new_code = 17;
        }
    }
    if (new_code) {
        UC_WARN("inv pream code (using %d)\n", new_code);
        *arg_code = new_code;
    }
}

uint16_t
remap_framefilter(uint16_t cfg_ff)
{
    uint16_t r=0;
    if (cfg_ff & UWB_FF_BEACON_EN) r|= DWT_FF_BEACON_EN;
    if (cfg_ff & UWB_FF_DATA_EN)   r|= DWT_FF_DATA_EN;
    if (cfg_ff & UWB_FF_ACK_EN)    r|= DWT_FF_ACK_EN;
    if (cfg_ff & UWB_FF_MAC_EN)    r|= DWT_FF_MAC_EN;
    if (cfg_ff & UWB_FF_RSVD_EN)   r|= DWT_FF_RSVD_EN;
    if (cfg_ff & UWB_FF_COORD_EN)  r|= DWT_FF_COORD_EN;
    return r;
}
#endif

int
uwbcfg_commit_to_inst_dw1000(struct uwb_dev * inst, char cfg[CFGSTR_MAX][CFGSTR_STRLEN])
{
#if MYNEWT_VAL(DW1000_DEVICE_0)
    uint8_t prf, coarse, fine, txpwr, paclen;

    /* Only proceed if this device is a dw1000 */
    if(inst->device_id != DWT_DEVICE_ID) {
        return 0;
    }

    /* Set the PRF */
    conf_value_from_str(cfg[CFGSTR_PRF], CONF_INT8, (void*)&prf, 0);
    if (prf == 16) {
        inst->config.prf = DWT_PRF_16M;
    } else if (prf == 64) {
        inst->config.prf = DWT_PRF_64M;
    } else {
        UC_WARN("inv prf %d\n", prf);
    }

    /* Check that the rx and tx preamble codes are legal for the ch+prf combo */
    check_preamble_code(inst, &inst->config.rx.preambleCodeIndex);
    check_preamble_code(inst, &inst->config.tx.preambleCodeIndex);

    switch (inst->config.channel) {
    case (1): inst->config.txrf.PGdly = TC_PGDELAY_CH1;break;
    case (2): inst->config.txrf.PGdly = TC_PGDELAY_CH2;break;
    case (3): inst->config.txrf.PGdly = TC_PGDELAY_CH3;break;
    case (4): inst->config.txrf.PGdly = TC_PGDELAY_CH4;break;
    case (7): inst->config.txrf.PGdly = TC_PGDELAY_CH7;break;
    case (5): inst->config.txrf.PGdly = TC_PGDELAY_CH5;break;
    default:
        UC_WARN("inv ch\n");
        break;
    }

    /* Data rate */
    if (!strcmp(cfg[CFGSTR_DATARATE], "6m8")) {
        inst->config.dataRate = DWT_BR_6M8;
    } else if (!strcmp(cfg[CFGSTR_DATARATE], "850k")) {
        inst->config.dataRate = DWT_BR_850K;
    } else if (!strcmp(cfg[CFGSTR_DATARATE], "110k")) {
        inst->config.dataRate = DWT_BR_110K;
    } else {
        UC_WARN("inv datarate\n");
    }

    /* PAC length */
    conf_value_from_str(cfg[CFGSTR_RX_PACLEN], CONF_INT8, (void*)&paclen, 0);
    switch (paclen) {
    case (8):  inst->config.rx.pacLength = DWT_PAC8;break;
    case (16): inst->config.rx.pacLength = DWT_PAC16;break;
    case (32): inst->config.rx.pacLength = DWT_PAC32;break;
    case (64): inst->config.rx.pacLength = DWT_PAC64;break;
    default:
        UC_WARN("inv paclen\n");
    }

    /* SFDType, only 0 or 1 allowed on dw1000 */
    if (inst->config.rx.sfdType>1) {
        UC_WARN("inv sfdType\n");
    }

    /* Tx Power */
    conf_value_from_str(cfg[CFGSTR_TXRF_PWR_COARSE], CONF_INT8, (void*)&coarse, 0);
    conf_value_from_str(cfg[CFGSTR_TXRF_PWR_FINE], CONF_INT8, (void*)&fine, 0);

    txpwr = inst->config.txrf.BOOSTNORM;
    switch (coarse) {
    case(18): txpwr = power_value(DW1000_txrf_config_18db, fine);break;
    case(15): txpwr = power_value(DW1000_txrf_config_15db, fine);break;
    case(12): txpwr = power_value(DW1000_txrf_config_12db, fine);break;
    case(9):  txpwr = power_value(DW1000_txrf_config_9db, fine);break;
    case(6):  txpwr = power_value(DW1000_txrf_config_6db, fine);break;
    case(3):
        txpwr = power_value(DW1000_txrf_config_3db, fine);
        break;
    case(0):
        txpwr = power_value(DW1000_txrf_config_0db, fine);
        break;
    default:
        UC_WARN("inv coarse txpwr\n");
    }
    inst->config.txrf.BOOSTNORM = txpwr;
    inst->config.txrf.BOOSTP500 = txpwr;
    inst->config.txrf.BOOSTP250 = txpwr;
    inst->config.txrf.BOOSTP125 = txpwr;

    /* Remap the uwb_mac style framefilter to something that
     * dw1000 understands */
    inst->config.rx.frameFilter = remap_framefilter(
        inst->config.rx.frameFilter);
#endif
    return 0;
}
