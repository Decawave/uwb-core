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

/* Test log10 */
TEST_CASE_SELF(test_log10)
{
        float64_t out;
        float64_t exp;
        int rc;

        /* Case 1 */
        exp = SOFTFLOAT_INIT64(log10(1.0));
        out = log10_soft(SOFTFLOAT_INIT64(1.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /* Case 2 */
        exp = SOFTFLOAT_INIT64(log10(1000.0));
        out = log10_soft(SOFTFLOAT_INIT64(1000.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /* Case 3 */
        exp = SOFTFLOAT_INIT64(log10(200));
        out = log10_soft(SOFTFLOAT_INIT64(200));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /* Case 4 */
        exp = SOFTFLOAT_INIT64(log10(10));
        out = log10_soft(SOFTFLOAT_INIT64(10));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /* Case 5 */
        exp = SOFTFLOAT_INIT64(log10(10000.0));
        out = log10_soft(SOFTFLOAT_INIT64(10000.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

        /* Case 6 */
        exp = SOFTFLOAT_INIT64(log10(400.0));
        out = log10_soft(SOFTFLOAT_INIT64(400.0));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

         /* Case 7 */
        exp = SOFTFLOAT_INIT64(log10(0.01));
        out = log10_soft(SOFTFLOAT_INIT64(0.01));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

         /* Case 8 */
        exp = SOFTFLOAT_INIT64(log10(0.001));
        out = log10_soft(SOFTFLOAT_INIT64(0.001));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);

         /* Case 9 */
        exp = SOFTFLOAT_INIT64(log10(0.12345));
        out = log10_soft(SOFTFLOAT_INIT64(0.12345));
        rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
        TEST_ASSERT(!rc);
}