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

/* TOF TO METERS TEST */
TEST_CASE_SELF(calc_tof_to_meters_test)
{
    int rc;
    dpl_float64_t ToF = 0.0;
    dpl_float64_t out = 0.0;
    dpl_float64_t exp = 0.0;

    ToF = 1.0;
    exp = 0.00469030;
    out = uwb_rng_tof_to_meters(ToF);
    rc = epsilon_same_float(out, exp);
    TEST_ASSERT(rc);

    ToF = 0.0;
    exp = 0.0;
    out = uwb_rng_tof_to_meters(ToF);
    rc = epsilon_same_float(out, exp);
    TEST_ASSERT(rc);

    ToF = 123456789;
    exp = 579060.450834;
    out = uwb_rng_tof_to_meters(ToF);
    rc = epsilon_same_float(out, exp);
    TEST_ASSERT(rc);

    ToF = 0.00009;
    exp = 0.000000422;
    out = uwb_rng_tof_to_meters(ToF);
    rc = epsilon_same_float(out, exp);
    TEST_ASSERT(rc);

    ToF = 213.202;
    exp = 1.000000464;
    out = uwb_rng_tof_to_meters(ToF);
    rc = epsilon_same_float(out, exp);
    TEST_ASSERT(rc);
}
