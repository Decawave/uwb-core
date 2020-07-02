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
#define JSON_BIGBUF_SIZE 192

TEST_CASE_SELF(uwb_rng_json_test_write)
{
    int rc;
    char *output1 = "{\"utime\": 1,\"seq\": 2,\"c\": 3,\"uid\": 4,\"ouid\": 5,\"raz\": [1.000,2.000,3.000],\"braz\": [1.000,2.000,3.000],\"rssi\": [1.000,2.000,3.000],\"los\": [1.000,2.000,3.000],\"ppm\": 6.000,\"sts\": 7.000}";
    char *output2 = "{\"utime\": 0,\"raz\": [null]}";
    char *output3 = "{\"utime\": 1,\"seq\": 2,\"c\": 3,\"uid\": 4,\"raz\": [0.000,0.000,0.000],\"braz\": [1.000,2.000,3.000],\"rssi\": [1.000,2.000,3.000],\"los\": [0.000,0.000,0.000]}";
    char *output4 = "{\"utime\": 0,\"raz\": [0.000,0.000,0.000],\"braz\": [0.000,0.000,0.000],\"rssi\": [0.000,0.000,0.000],\"los\": [0.000,0.000,0.000],\"ppm\": 0.000,\"sts\": 0.000}";

    //
    /**
     * Test case 1 General write
     */
    rng_json_t json1 = {
        .utime = 1,
        .seq   = 2,
        .code  = 3,
        .uid   = 4,
        .ouid  = 5,
        .ppm   = 6,
        .sts   = 7,
        .raz   = {{1, 2, 3}},
        .braz  = {{1, 2, 3}},
        .los   = {1, 2, 3},
        .rssi  = {1, 2, 3},
    };
    rc = rng_json_write(&json1);
    TEST_ASSERT(rc == 0);

    rc = strcmp(json1.iobuf, output1);
    if (rc) {
        printf("\ngot:\n%s\nexpected:\n%s\n", json1.iobuf, output1);
    }
    TEST_ASSERT(rc == 0);

    /**
     * Test case 2 check NAN handling
     */
    rng_json_t json2 = {
        .utime = 0,
        .uid   = 0,
        .ouid  = 0,
        .ppm   = DPL_FLOAT64_NAN(),
        .sts   = DPL_FLOAT64_NAN(),
        .raz   = {{DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()}},
        .braz  = {{DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()}},
        .los   = {DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()},
        .rssi  = {DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN(), DPL_FLOAT64_NAN()},
    };
    rc = rng_json_write(&json2);
    TEST_ASSERT(rc == 0);

    rc = strcmp(json2.iobuf, output2);
    TEST_ASSERT(rc == 0);

    /**
     * Test case 3 writing with missing fields
     */
    rng_json_t json3 = {
        .utime = 1,
        .seq   = 2,
        .code  = 3,
        .uid   = 4,
        .ppm   = DPL_FLOAT64_NAN(),
        .sts   = DPL_FLOAT64_NAN(),
        .braz  = {{1, 2, 3}},
        .rssi  = {1, 2, 3},
    };
    rc = rng_json_write(&json3);
    TEST_ASSERT(rc == 0);

    rc = strcmp(json3.iobuf, output3);
    TEST_ASSERT(rc == 0);

    /**
     * Test case 4 writing empty JSON structure
     */
    rng_json_t json4 = {};
    rc = rng_json_write(&json4);
    TEST_ASSERT(rc == 0);

    rc = strcmp(json4.iobuf, output4);
    TEST_ASSERT(rc == 0);
}
