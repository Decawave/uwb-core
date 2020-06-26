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

#define ACCEPTABLE_ERROR (1.0e-15)

/* Test atan */
TEST_CASE_SELF(test_strtod)
{
    float64_t out;
    float64_t exp;
    int rc;

    union {
        float64_t soft;
        double hard;
        uint64_t integer;
    } out_u, exp_u;
    char buf[32];

    /* Case 1 */
    exp = SOFTFLOAT_INIT64(strtod("0.0", NULL));
    out = strtod_soft("0.0", NULL);
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 2 */
    exp = SOFTFLOAT_INIT64(strtod("1.0", NULL));
    out = strtod_soft("1.0", NULL);
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 3 */
    exp = SOFTFLOAT_INIT64(strtod("-1.2345", NULL));
    out = strtod_soft("-1.2345", NULL);
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 4 */
    exp_u.soft = SOFTFLOAT_INIT64(strtod("-1.2345e02", NULL));
    out_u.soft = strtod_soft("-1.2345e02", NULL);
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    if (rc && fabs(exp_u.hard - out_u.hard) > ACCEPTABLE_ERROR) {
        printf("%f - %f = %e\n",
               exp_u.hard,
               out_u.hard,
               fabs(exp_u.hard - out_u.hard));
        TEST_ASSERT(!rc);
    }

    /* Case 5 */
    exp_u.soft = SOFTFLOAT_INIT64(strtod("-1.2345e-02", NULL));
    out_u.soft = strtod_soft("-1.2345e-02", NULL);
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    if (rc && fabs(exp_u.hard - out_u.hard) > ACCEPTABLE_ERROR) {
        printf("%f - %f = %e\n",
               exp_u.hard,
               out_u.hard,
               fabs(exp_u.hard - out_u.hard));
        TEST_ASSERT(!rc);
    }

    /* Case 6 - testing all numbers -2.0 to 2.0 in 0.0001 increments */
    for (int i=-20000;i<20000;i++) {
        double f = i/10000.0;
        snprintf(buf, sizeof(buf), "%f", f);
        exp_u.soft = SOFTFLOAT_INIT64(strtod(buf, NULL));
        out_u.soft = strtod_soft(buf, NULL);
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
