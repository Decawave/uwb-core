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

/* PATH LOSS TEST */
TEST_CASE_SELF(path_loss_test)
{
    int rc;
    float out;
    float exp;

    exp = 150.54967;
    out = uwb_rng_path_loss(1, 1, 1, 1);
    rc = epsilon_same_float(exp, out);
    TEST_ASSERT(rc);

    out = uwb_rng_path_loss(0, 0, 0, 0);
    rc = isinf(out);
    TEST_ASSERT(rc);

    exp = 144.54967;
    out = uwb_rng_path_loss(-1, -1, -1, -1);
    rc = epsilon_same_float(exp, out);
    TEST_ASSERT(rc);

    exp = 130.96605;
    out = uwb_rng_path_loss(1, 2, 3, 4);
    rc = epsilon_same_float(exp, out);
    TEST_ASSERT(rc);

    exp = 166.46605;
    out = uwb_rng_path_loss(0.1, 0.2, 0.3, 0.4);
    rc = epsilon_same_float(exp, out);
    TEST_ASSERT(rc);
}
