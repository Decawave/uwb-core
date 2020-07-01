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

#include "uwb_rng_json_test.h"

#pragma pack(1)

TEST_CASE_SELF(uwb_rng_json_test_read)
{
    int rc;
    char *output1 = "{\"utime\": 1,\"seq\": 2,\"uid\": 4,\"ouid\": 5,\"sts\": 7.0000,\"ppm\": 6.0000, \"raz\": [1.0000,2.0000,3.0000],\"braz\": [1.0000,2.0000,3.0000],\"rssi\": [1.0000,2.0000,3.0000],\"los\": [1.0000,2.0000,3.0000],}";
    char *output2 = "{\"utime\": 1234,\"raz\": [null]}";

    /**
     * Test case 1
     */

    rng_json_t json_exp1 = {
        .utime = 1,
        .seq   = 2,
        .uid   = 4,
        .ouid  = 5,
        .ppm   = 6,
        .sts   = 7,
        .raz   = {{1, 2, 3}},
        .braz  = {{1, 2, 3}},
        .los   = {1, 2, 3},
        .rssi  = {1, 2, 3},
    };

    rng_json_t json_in1;
    rng_json_read(&json_in1, output1);

    rc = compare_json_rng_structs(&json_exp1, &json_in1);
    TEST_ASSERT(rc);

    rc = compare_rssi(json_exp1.rssi, json_in1.rssi);
    TEST_ASSERT(rc);

    rc = compare_los(json_exp1.los, json_in1.los);
    TEST_ASSERT(rc);

    rc = compare_raz(&json_exp1.raz, &json_in1.raz);
    TEST_ASSERT(rc);

    /**
     * Test case 2
     */

    rng_json_t json_exp2 = {
        .utime = 1234,
        .uid   = 0,
        .ouid  = 0,
        .ppm   = DPL_FLOAT64_NAN(),
        .sts   = DPL_FLOAT64_NAN(),
        .raz   = {{DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()}},
        .braz  = {{DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()}},
        .los   = {DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()},
        .rssi  = {DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()},
    };

    rng_json_t json_in2;

    rng_json_read(&json_in2, output2);

    rc = compare_json_rng_structs(&json_exp2, &json_in2);
    TEST_ASSERT(rc);

    rc = compare_rssi(json_exp2.rssi, json_in2.rssi);
    TEST_ASSERT(rc);

    rc = compare_los(json_exp2.los, json_in2.los);
    TEST_ASSERT(rc);

    rc = compare_raz(&json_exp2.raz, &json_in2.raz);
    TEST_ASSERT(rc);

}
