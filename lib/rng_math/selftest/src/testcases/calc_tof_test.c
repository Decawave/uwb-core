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

#include "rng_math_test.h"
#ifndef UINT_MAX
#define UINT_MAX	(~0U)
#endif

/* TWR TO TOF TEST */
TEST_CASE_SELF(calc_tof_test)
{
    dpl_float64_t out = 0.0;

    /**
     * SINGLE SIDED
     */

    /**
     * 5 meter distance measurements:
     *      Frame:
     *              Response timestamp: 254076713
     *              Request timestamp: 175480898
     *              Transmission timestamp: 1470169168
     *              Reception timestamp: 1391575515
     *              Skew: 0
     */
    out = calc_tof_ss(254076713, 175480898, 1470169168, 1391575515, DPL_FLOAT64_INIT(0));
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 5);

    /**
     * 10 meter distance measurements:
     *      Frame:
     *              Response timestamp: 1912245852
     *              Request timestamp: 1833647682
     *              Transmission timestamp: 786803792
     *              Reception timestamp: 708209934
     *
     * Sweeping over uint32 range to catch masking issues
     */
    for (uint64_t i=0;i<UINT_MAX;i+=0x100) {
        out = calc_tof_ss(1912245852 + (uint32_t)i,
                          1833647682 + (uint32_t)i,
                          786803792  + (uint32_t)i,
                          708209934  + (uint32_t)i,
                          DPL_FLOAT64_INIT(0));
        TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 10);
    }
    /**
     * 9 meter distance measurements:
     *      Frame:
     *              Response timestamp: 1075470169
     *              Request timestamp: 996872258
     *              Transmission timestamp: 1024692304
     *              Reception timestamp: 946098392
     */
    out = calc_tof_ss(1075470169, 996872258, 1024692304, 946098392, DPL_FLOAT64_INIT(0));
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 9);

    /**
     * 1 meter distance measurements with values that will give issues with 64/32bit math:
     */
    out = calc_tof_ss(0xcafe80d8, 0xc74f4042, 0x745650, 0xfcc517a4, DPL_FLOAT64_INIT(0));
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 1);

    /**
     * Single sided with variables set to maximum available value
     */
    out = calc_tof_ss(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, DPL_FLOAT64_INIT(0));
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 0);

    /**
     * DOUBLE SIDED
     */

    /**
     * 9 meter distance measurements:
     *      Frame:
     *              Response timestamp: 1075470169
     *              Request timestamp: 996872258
     *              Transmission timestamp: 1024692304
     *              Reception timestamp: 946098392
     *      First frame:
     *              Response timestamp: 2675898111
     *              Request timestamp: 2597300290
     *              Transmission timestamp: 1024692304
     *              Reception timestamp: 946098392
     */
    out = calc_tof_ds(2675898111, 2597300290, 1024692304, 946098392,
                      1075470169, 996872258, 1024692304, 946098392);
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 9);

    /**
     * 10 meter distance measurements:
     *      Frame:
     *              Response timestamp: 2333274316
     *              Request timestamp: 2254086210
     *              Transmission timestamp: 3947030096
     *              Reception timestamp: 3867846252
     *      First frame:
     *              Response timestamp: 3867846252
     *              Request timestamp: 3789248066
     *              Transmission timestamp: 2254086224
     *              Reception timestamp: 2175492353
     *
     * Sweeping over uint32 range to catch masking issues
     */
    for (uint64_t i=0;i<UINT_MAX;i+=0x100) {
        out = calc_tof_ds(3867846252 + (uint32_t)i,
                          3789248066 + (uint32_t)i,
                          2254086224 + (uint32_t)i,
                          2175492353 + (uint32_t)i,
                          2333274316 + (uint32_t)i,
                          2254086210 + (uint32_t)i,
                          3947030096 + (uint32_t)i,
                          3867846252 + (uint32_t)i);
        TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 10);
    }

    /**
     * 5 meter distance measurements:
     *      Frame:
     *              Response timestamp: 3396658744
     *              Request timestamp: 3334250050
     *              Transmission timestamp: 2180579920
     *              Reception timestamp: 2118173324
     *      First frame:
     *              Response timestamp: 2118173324
     *              Request timestamp: 2056354370
     *              Transmission timestamp: 3334250064
     *              Reception timestamp: 3272433282
     */
    out = calc_tof_ds(2118173324, 2056354370, 3334250064, 3272433282,
                      3396658744, 3334250050, 2180579920, 2118173324);
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 5);


    /**
     * 1 meter distance measurements with values likely to give trouble with 64/32-bit unsigned math:
     */
    out = calc_tof_ds(0xfff0b714, 0xfc417642, 0xb7e93850, 0xb439f952,
                      0xbba17ae4, 0xb7e93842, 0x3a8f650, 0xfff0b714);
    TEST_ASSERT(DPL_FLOAT64_F64_TO_U64(uwb_rng_tof_to_meters(out)) == 1);

    /**
     * Double sided with variables set to maximum
     */
    out = calc_tof_ds(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
                      0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
    TEST_ASSERT(isnan(uwb_rng_tof_to_meters(out)));
}
