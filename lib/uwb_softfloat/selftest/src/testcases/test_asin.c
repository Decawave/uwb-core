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

#define ACCEPTABLE_ERROR (1.0e-12)

/* Test asin */
TEST_CASE_SELF(test_asin)
{
    float64_t out;
    float64_t exp;
    int rc;
    union {
        float64_t soft;
        double hard;
        uint64_t integer;
    } out_u, exp_u;

    /* Case 1 */
    exp = SOFTFLOAT_INIT64(asin(0.0));
    out = asin_soft(SOFTFLOAT_INIT64(0.0));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 2 */
    exp = SOFTFLOAT_INIT64(asin(0.456));
    out = asin_soft(SOFTFLOAT_INIT64(0.456));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 3 */
    exp = SOFTFLOAT_INIT64(asin(0.999));
    out = asin_soft(SOFTFLOAT_INIT64(0.999));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 3 */
    exp = SOFTFLOAT_INIT64(asin(-0.456));
    out = asin_soft(SOFTFLOAT_INIT64(-0.456));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 4 */
    exp = SOFTFLOAT_INIT64(asin(-0.999));
    out = asin_soft(SOFTFLOAT_INIT64(-0.999));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 5 - testing all numbers -2.0 to 2.0 in 0.1 increments */
    for (int i=-200;i<200;i++) {
        double f = i/100.0;
        exp_u.soft = SOFTFLOAT_INIT64(asin(f));
        out_u.soft = asin_soft(SOFTFLOAT_INIT64(f));
        rc = memcmp(&exp_u.soft.v, &out_u.soft.v, sizeof(float64_t));
        if (rc && fabs(exp_u.hard - out_u.hard) > ACCEPTABLE_ERROR) {
            printf("%f: %f - %f = %e\n",
                   f,
                   exp_u.hard,
                   out_u.hard,
                   fabs(exp_u.hard - out_u.hard));
            TEST_ASSERT(!rc);
        }
    }

}
