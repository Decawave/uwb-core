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
#include <string.h>
#include <assert.h>
#include <math.h>
#include <dpl/dpl_types.h>
#include "rng_math/rng_math.h"

#define MASK32 (0xFFFFFFFFUL)

/**
 * @fn uwb_rng_tof_to_meters(float ToF)
 * @brief API to calculate range in meters from time of flight base on type of ranging
 *
 * @param ToF Time of flight in float.
 *
 * @return range in meters
 */
dpl_float64_t
uwb_rng_tof_to_meters(dpl_float64_t ToF)
{
    if (DPL_FLOAT64_ISNAN(ToF)) {
        return DPL_FLOAT64_NAN();
    }
    /* ToF * (299792458.0l/1.000293l) * (1.0/499.2e6/128.0) */
    dpl_float64_t tmp = DPL_FLOAT64_INIT((299792458.0l/1.000293l) * (1.0/499.2e6/128.0));
    return DPL_FLOAT64_MUL(ToF, tmp);
}

dpl_float64_t
calc_tof_ss(uint32_t response_timestamp, uint32_t request_timestamp,
            uint64_t transmission_timestamp,
            uint64_t reception_timestamp,  dpl_float64_t skew)
{
    dpl_float64_t ToF = DPL_FLOAT64_I32_TO_F64(0), tmpf;
    uint64_t T1R, T1r;

    T1R = (response_timestamp - request_timestamp);
    T1r = (transmission_timestamp - reception_timestamp) & MASK32;
    tmpf = DPL_FLOAT64_MUL(DPL_FLOAT64_U64_TO_F64(T1r),
                                DPL_FLOAT64_SUB(DPL_FLOAT64_INIT(1.0), skew));
    ToF = DPL_FLOAT64_SUB(DPL_FLOAT64_U64_TO_F64(T1R), tmpf);
    ToF = DPL_FLOAT64_DIV(ToF, DPL_FLOAT64_INIT(2.0));

    return ToF;
}

dpl_float64_t
calc_tof_ds(uint32_t first_response_timestamp, uint32_t first_request_timestamp,
            uint64_t first_transmission_timestamp, uint64_t first_reception_timestamp,
            uint32_t response_timestamp, uint32_t request_timestamp,
            uint64_t transmission_timestamp, uint64_t reception_timestamp)
{
    dpl_float64_t ToF = DPL_FLOAT64_I32_TO_F64(0);
    uint64_t T1R, T1r, T2R, T2r;
    int64_t nom, denom;

    T1R = (first_response_timestamp - first_request_timestamp);
    T1r = (first_transmission_timestamp  - first_reception_timestamp) & MASK32;
    T2R = (response_timestamp - request_timestamp);
    T2r = (transmission_timestamp - reception_timestamp) & MASK32;
    nom = T1R * T2R  - T1r * T2r;
    denom = T1R + T2R  + T1r + T2r;

    if (denom == 0) {
        return DPL_FLOAT64_NAN();
    }

    ToF = DPL_FLOAT64_DIV(DPL_FLOAT64_I64_TO_F64(nom),
                                                DPL_FLOAT64_I64_TO_F64(denom));
    return ToF;
}

/**
 * @fn uwb_rng_twr_to_tof_sym(twr_frame_t twr[], uwb_dataframe_code_t code)
 * @brief API to calculate time of flight for symmetric type of ranging.
 *
 * @param twr[]  Pointer to twr buffers.
 * @param code   Represents mode of ranging UWB_DATA_CODE_SS_TWR enables single sided two way ranging UWB_DATA_CODE_DS_TWR enables double sided
 * two way ranging UWB_DATA_CODE_DS_TWR_EXT enables double sided two way ranging with extended frame.
 *
 * @return Time of flight
 */
uint32_t
calc_tof_sym_ss(uint32_t response_timestamp, uint32_t request_timestamp,
                uint64_t transmission_timestamp, uint64_t reception_timestamp)
{
    uint32_t ToF = 0;

    ToF = ((response_timestamp - request_timestamp) - (transmission_timestamp -
            reception_timestamp)) / 2;

    return ToF;
}

uint32_t
calc_tof_sym_ds(uint32_t first_response_timestamp, uint32_t first_request_timestamp,
                uint64_t first_transmission_timestamp, uint64_t first_reception_timestamp,
                uint32_t response_timestamp, uint32_t request_timestamp,
                uint64_t transmission_timestamp, uint64_t reception_timestamp)
{
    uint32_t ToF = 0;
    uint64_t T1R, T1r, T2R, T2r;

    T1R = (first_response_timestamp - first_request_timestamp);
    T1r = (first_transmission_timestamp  - first_reception_timestamp) & MASK32;
    T2R = (response_timestamp - request_timestamp);
    T2r = (transmission_timestamp  - reception_timestamp) & MASK32;
    ToF = (T1R - T1r + T2R - T2r) >> 2;

    return ToF;
}

/**
 * @fn uwb_rng_path_loss(float Pt, float G, float fc, float R)
 * @brief calculate rng path loss using range parameters and return signal level.
 * and allocated PANIDs and SLOTIDs.
 *
 * @param Pt      Transmit power in dBm.
 * @param G       Antenna Gain in dB.
 * @param Fc      Centre frequency in Hz.
 * @param R       Range in meters.
 *
 * @return Pr received signal level dBm
 */
#ifndef __KERNEL__
float
uwb_rng_path_loss(float Pt, float G, float fc, float R){
    float Pr = Pt + 2 * G + 20 * log10(299792458.0l/1.000293l) - 20 * log10(4 * M_PI * fc * R);
    assert(Pr >= 0);
    return Pr;
}
#endif
