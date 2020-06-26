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

/**
 * Error codes
 */

#include "euclid_test.h"
#include "euclid/norm.h"
#include "euclid/triad.h"

/* Test float */
TEST_CASE_SELF(euclid_test_normf)
{
        int rc;
        float out;
        float exp;

        /* Case1 : Test with decimal values.
         *      A = {Ax = 17, Ay = 18, Az = 19}
         *      B = {Ax = 13, By = 86. Bz = 44}
         *      Expected distance: 72.5603179
         */
        exp = 72.5;
        out = normf(&test_triads_f[0], &test_triads_f[1], 3);

        rc = epsilon_same_float(exp, out);

        TEST_ASSERT(rc == 0);

        /* Case2 : Test with floating point numbers.
         *      A = {Ax = 0.456, Ay = 0.112, Az = 0.566}
         *      B = {Ax = 0.652, By = 0.214, Bz = 0.341}
         *      Expected distance: 0.315349
         */
        exp = 0.3;
        out = normf(&test_triads_f[2], &test_triads_f[3], 3);

        rc = epsilon_same_float(out, exp);

        TEST_ASSERT(rc == 0);

        /* Case3 : Test with dimensions limited to: 2.
         *      A = {Ax = 1, Ay = 2, Az = 3}
         *      B = {Ax = 4, By = 5, Bz = 6}
         *      Expected distance: 0.315349
         */
        exp = 4.2;
        out = normf(&test_triads_f[4], &test_triads_f[5], 2);

        rc = epsilon_same_float(out, exp);

        TEST_ASSERT(rc == 0);

        /* Case4 : Test with negative dimension values.
         *      A = {Ax = 1, Ay = 2, Az = 3}
         *      B = {Ax = 4, By = 5, Bz = 6}
         *      Expected distance: 0.315349
         */
        exp = 1223.9;
        out = normf(&test_triads_f[6], &test_triads_f[7], 3);

        rc = epsilon_same_float(out, exp);

        TEST_ASSERT(rc == 0);
}

/* Test double */
TEST_CASE_SELF(euclid_test_norm)
{
        int rc;
        double out;
        double exp;

        /* Case1 : Test with decimal values.
         *      A = {Ax = 17, Ay = 18, Az = 19}
         *      B = {Ax = 13, By = 86. Bz = 44}
         *      Expected distance: 72.56032
         */
        exp = 72.5603197;
        out = norm(&test_triads[0], &test_triads[1], 3);

        rc = epsilon_same_double(out, exp);

        TEST_ASSERT(rc == 0);

        /* Case2 : Test with floating point numbers.
         *      A = {Ax = 0.456, Ay = 0.112, Az = 0.566}
         *      B = {Ax = 0.652, By = 0.214, Bz = 0.341}
         *      Expected distance: 0.315349
         */
        exp = 0.3156644;
        out = norm(&test_triads[2], &test_triads[3], 3);

        rc = epsilon_same_double(out, exp);

        TEST_ASSERT(rc == 0);

        /* Case3 : Test with dimensions limited to: 2.
         *      A = {Ax = 1, Ay = 2, Az = 3}
         *      B = {Ax = 4, By = 5, Bz = 6}
         *      Expected distance: 0.315349
         */
        exp = 4.242641;
        out = norm(&test_triads[4], &test_triads[5], 2);

        rc = epsilon_same_double(out, exp);

        TEST_ASSERT(rc == 0);

        /* Case4 : Test with negative dimension values.
         *      A = {Ax = 1, Ay = 2, Az = 3}
         *      B = {Ax = 4, By = 5, Bz = 6}
         *      Expected distance: 0.315349
         */
        exp = 1223.956224;
        out = norm(&test_triads[6], &test_triads[7], 3);

        rc = epsilon_same_double(out, exp);

        TEST_ASSERT(rc == 0);
}