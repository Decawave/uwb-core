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
#include <uwbcfg/uwbcfg.h>
#include "uwbcfg_priv.h"
#if MYNEWT_VAL(DW3000_DEVICE_0)
#include <dw3000-c0/dw3000_hal.h>
#endif

int
uwbcfg_commit_to_inst_dw3000(struct uwb_dev * inst, char cfg[CFGSTR_MAX][CFGSTR_STRLEN])
{
#if MYNEWT_VAL(DW3000_DEVICE_0)
    uint8_t pdoa_mode, coarse, fine, txpwr, paclen;
    uint16_t sts_len;

    /* Only proceed if this device is a dw3000 */
    if((inst->device_id&(DEV_ID_RIDTAG_BIT_MASK|DEV_ID_MODEL_BIT_MASK)) != DWT_DEVICE_ID) {
        return 0;
    }

    switch (inst->config.channel) {
    case (5): inst->config.txrf.PGdly = TC_PGDELAY_CH5;break;
    case (9): inst->config.txrf.PGdly = TC_PGDELAY_CH9;break;
    default:
        UC_WARN("inv ch\n");
        break;
    }

    /* Data rate */
    if (!strcmp(cfg[CFGSTR_DATARATE], "6m8")) {
        inst->config.dataRate = DWT_BR_6M8;
    } else if (!strcmp(cfg[CFGSTR_DATARATE], "850k")) {
        inst->config.dataRate = DWT_BR_850K;
    } else {
        UC_WARN("inv datarate\n");
    }

    /* PHR rate */
    conf_value_from_str(cfg[CFGSTR_RX_PHR_RATE], CONF_INT8,
                        (void*)&inst->config.rx.phrRate, 0);
    inst->attrib.phr_rate = inst->config.rx.phrRate;

    /* PAC length */
    conf_value_from_str(cfg[CFGSTR_RX_PACLEN], CONF_INT8, (void*)&paclen, 0);
    switch (paclen) {
    case (4):  inst->config.rx.pacLength = DWT_PAC4;break;
    case (8):  inst->config.rx.pacLength = DWT_PAC8;break;
    case (16): inst->config.rx.pacLength = DWT_PAC16;break;
    case (32): inst->config.rx.pacLength = DWT_PAC32;break;
    default:
        UC_WARN("inv paclen\n");
    }

    switch (inst->config.rx.sfdType) {
    case (0): break;            /* Short, 8bit standard */
    case (1): break;            /* Nonstandard, 8bit */
    case (2): break;            /* Nonstandard, 16bit */
    case (3): break;            /* 4zBinary */
    default:
        UC_WARN("inv sfdType\n");
    }

    /* Sts */
    inst->config.rx.stsMode = DWT_STS_MODE_OFF;

    if (strstr(cfg[CFGSTR_RX_STS_MODE], "1")) {
        inst->config.rx.stsMode = DWT_STS_MODE_1;
    } else if (strstr(cfg[CFGSTR_RX_STS_MODE], "2")) {
        inst->config.rx.stsMode = DWT_STS_MODE_2;
    } else {
        inst->config.rx.stsMode = DWT_STS_MODE_OFF;
    }

    if (strstr(cfg[CFGSTR_RX_STS_MODE], "sdc")) {
        inst->config.rx.stsMode |= DWT_STS_MODE_SDC;
    }
    if (strstr(cfg[CFGSTR_RX_STS_MODE], "4z")) {
        //inst->config.rx.stsMode |= DWT_STS_MODE_4Z;
        assert(0);
    }

    conf_value_from_str(cfg[CFGSTR_RX_STS_LEN], CONF_INT16, (void*)&sts_len, 0);
    if (sts_len < 32 || sts_len > 2040 || sts_len%8>0) {
        UC_WARN("inv rx_sts_len\n");
    }
    inst->config.rx.stsLength = (sts_len>>3)-1;
    if (inst->config.rx.stsMode != DWT_STS_MODE_OFF) {
        /* +2 There's a gap of one symbol on either side of the STS */
        inst->attrib.nstssync = sts_len + 2;
    } else {
        inst->attrib.nstssync = 0;
    }

    /* PDOA Mode */
    conf_value_from_str(cfg[CFGSTR_RX_PDOA_MODE], CONF_INT8, (void*)&pdoa_mode, 0);
    if (pdoa_mode < 4) {
        inst->config.rx.pdoaMode = pdoa_mode;
    } else {
        UC_WARN("inv rx_pdoa_mode\n");
    }

    /* Tx Power */
    conf_value_from_str(cfg[CFGSTR_TXRF_PWR_COARSE], CONF_INT8, (void*)&coarse, 0);
    conf_value_from_str(cfg[CFGSTR_TXRF_PWR_FINE], CONF_INT8, (void*)&fine, 0);

    if (fine > 63) {
        fine = 63;
        UC_WARN("inv txrf_power_fine, <=63\n");
    }

    txpwr = inst->config.txrf.BOOSTNORM;
    switch (coarse) {
    case(9):
        txpwr = dw3000_power_value(DW3000_txrf_config_VMC11_9db, fine);
        break;
    case(6):
        txpwr = dw3000_power_value(DW3000_txrf_config_VMC10_6db, fine);
        break;
    case(3):
        txpwr = dw3000_power_value(DW3000_txrf_config_VMC01_3db, fine);
        break;
    case(0):
        txpwr = dw3000_power_value(DW3000_txrf_config_VMC00_0db, fine);
        break;
    default:
        UC_WARN("inv coarse txpwr, [0,3,6,9]\n");
    }
    inst->config.txrf.BOOSTNORM = txpwr;
    inst->config.txrf.BOOSTP500 = txpwr;
    inst->config.txrf.BOOSTP250 = txpwr;
    inst->config.txrf.BOOSTP125 = txpwr;

#endif
    return 0;
}
