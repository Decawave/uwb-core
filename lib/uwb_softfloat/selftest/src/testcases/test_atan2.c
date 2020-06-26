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

#include "soft_test.h"
#include "softfloat/softfloat.h"
#include <math.h>

/* Test atan */
TEST_CASE_SELF(test_atan2)
{
        float64_t out;
        float64_t exp;
        int rc;

        /** Case 1
         * y = +0 (-0)
         * x = +0 (-0)
         */
        exp = SOFTFLOAT_INIT64(atan2(0.0, 0.0));
        out = atan2_soft(SOFTFLOAT_INIT64(0.0), SOFTFLOAT_INIT64(0.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 2
         * y = +0 (-0)
         * x is less than 0
         */
        exp = SOFTFLOAT_INIT64(atan2(0.0, -1.0));
        out = atan2_soft(SOFTFLOAT_INIT64(0.0), SOFTFLOAT_INIT64(-1.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 3
         * y = +0 (-0)
         * x = is greater than 0
         */
        exp = SOFTFLOAT_INIT64(atan2(0.0, 1.0));
        out = atan2_soft(SOFTFLOAT_INIT64(0.0), SOFTFLOAT_INIT64(1.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 4
         * y = is less than 0
         * x = 0 (-0)
         */
        exp = SOFTFLOAT_INIT64(atan2(-1.0, 0.0));
        out = atan2_soft(SOFTFLOAT_INIT64(-1.0), SOFTFLOAT_INIT64(0.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 5
         * y = is greater than 0
         * x = 0 (-0)
         */
        exp = SOFTFLOAT_INIT64(atan2(1.0, 0.0));
        out = atan2_soft(SOFTFLOAT_INIT64(1.0), SOFTFLOAT_INIT64(0.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 6
         * x = is NaN
         * y = is not NaN
         */
        exp = SOFTFLOAT_INIT64(atan2(NAN, 0.0));
        out = atan2_soft(SOFTFLOAT_INIT64(NAN), SOFTFLOAT_INIT64(0.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 7
         * x = is not NaN
         * y = is NaN
         */
        exp = SOFTFLOAT_INIT64(atan2(0.0, NAN));
        out = atan2_soft(SOFTFLOAT_INIT64(0.0), SOFTFLOAT_INIT64(NAN));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 8
         * x = is NaN
         * y = is NaN
         */
        exp = SOFTFLOAT_INIT64(atan2(NAN, NAN));
        out = atan2_soft(SOFTFLOAT_INIT64(NAN), SOFTFLOAT_INIT64(NAN));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 9
         * x = is negative infinity
         * y = finite greater than 0
         */
        exp = SOFTFLOAT_INIT64(atan2(1.0, -INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(1.0), SOFTFLOAT_INIT64(-INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 10
         * x = is negative infinity
         * y = finite less than 0
         */
        exp = SOFTFLOAT_INIT64(atan2(-1.0, -INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(-1.0), SOFTFLOAT_INIT64(-INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 11
         * x = is positive infinity
         * y = finite less than 0
         */
        exp = SOFTFLOAT_INIT64(atan2(-1.0, INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(-1.0), SOFTFLOAT_INIT64(INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 12
         * x = is positive infinity
         * y = finite more than 0
         */
        exp = SOFTFLOAT_INIT64(atan2(1.0, INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(1.0), SOFTFLOAT_INIT64(INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 13
         * x = is finite
         * y = is positive infinity
         */
        exp = SOFTFLOAT_INIT64(atan2(INFINITY, 1.0));
        out = atan2_soft(SOFTFLOAT_INIT64(INFINITY), SOFTFLOAT_INIT64(1.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 14
         * x = is finite
         * y = is negative infinity
         */
        exp = SOFTFLOAT_INIT64(atan2(-INFINITY, 1.0));
        out = atan2_soft(SOFTFLOAT_INIT64(-INFINITY), SOFTFLOAT_INIT64(1.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 15
         * x = is positive infinity
         * y = is negative infinity
         */
        exp = SOFTFLOAT_INIT64(atan2(-INFINITY, INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(-INFINITY), SOFTFLOAT_INIT64(INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 15
         * x = is negtive infinity
         * y = is negative infinity
         */
        exp = SOFTFLOAT_INIT64(atan2(-INFINITY, -INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(-INFINITY), SOFTFLOAT_INIT64(-INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 16
         * x = is positive infinity
         * y = is positive infinity
         */
        exp = SOFTFLOAT_INIT64(atan2(INFINITY, INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(INFINITY), SOFTFLOAT_INIT64(INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /** Case 17
         * x = is positive infinity
         * y = is negative infinity
         */
        exp = SOFTFLOAT_INIT64(atan2(-INFINITY, INFINITY));
        out = atan2_soft(SOFTFLOAT_INIT64(-INFINITY), SOFTFLOAT_INIT64(INFINITY));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);
}
