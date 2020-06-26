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

#include "dsp_test.h"

TEST_CASE_SELF(dsp_test_polyval)
{
    int rc;
    dpl_float32_t exp;
    dpl_float32_t result;
    dpl_float32_t *p = NULL;

    /**
     * Test case 1
     * x = 2
     * p = {1, 2, 3}
     * polynomial = 1 * x^2 + 2 * x + 3
     * expected result = 11
     */
    p = malloc(3 * sizeof(float));
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    exp = 11;
    result = polyval(p, 2, 3);
    rc = epsilon_same_float(result, exp);
    TEST_ASSERT(rc);
    free(p);

    /**
     * Test case 2
     * x = 0
     * p = {0, 0, 0}
     * polynomial = 0 * x^2 + 0 * x + 0
     * expected result = 0
     */
    p = malloc(3 * sizeof(float));
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    exp = 0;
    result = polyval(p, 0, 0);
    rc = epsilon_same_float(result, exp);
    TEST_ASSERT(rc);
    free(p);

    /**
     * Test case 3
     * x = -1
     * p = {1, 2, 3, 4}
     * polynomial = 1 * x^3 + 2 * x^2 + 3 * x + 4
     * expected result = 2
     */
    p = malloc(4 * sizeof(float));
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    p[3] = 4;
    exp = 2;
    result = polyval(p, -1, 4);
    rc = epsilon_same_float(result, exp);
    TEST_ASSERT(rc);
    free(p);

    /**
     * Test case 4
     * x = 0.1
     * p = {0.1, 0.2, 0.3, 0.4, 0.5}
     * polynomial = 0.1 * x^4 + 0.2 * x^3 + 0.3 * x^2 + 0.4 * x + 0.5
     * expected result =
     */
    p = malloc(5 * sizeof(float));
    p[0] = 0.1;
    p[1] = 0.2;
    p[2] = 0.3;
    p[3] = 0.4;
    p[4] = 0.5;
    exp = 0.543210;
    result = polyval(p, 0.1, 5);
    rc = epsilon_same_float(result, exp);
    TEST_ASSERT(rc);
    free(p);
}
