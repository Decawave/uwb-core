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

#include <stdio.h>
#include <string.h>

#include "os/mynewt.h"
#include "testutil/testutil.h"

#include "euclid/triad.h"
#include "euclid/norm.h"

union _triadf_t test_triads_f[] = {
        [0] = {
                .x = 17,
                .y = 18,
                .z = 19,
        },

        [1] = {
                .x = 13,
                .y = 86,
                .z = 44,
        },

        [2] = {
                .x = 0.4564561,
                .y = 0.1121121,
                .z = 0.5665661,
        },

        [3] = {
                .x = 0.6526521,
                .y = 0.2142141,
                .z = 0.3413411,
        },

        [4] = {
                .x = 1,
                .y = 2,
                .z = 3,
        },

        [5] = {
                .x = 4,
                .y = 5,
                .z = 6,
        },

        [6] = {
                .x = -0.3333331,
                .y = -0.3213211,
                .z = -0.8898991,
        },

        [7] = {
                .x = -321,
                .y = -632,
                .z = -999,
        },
};

union _triad_t test_triads[] = {
        [0] = {
                .x = 17,
                .y = 18,
                .z = 19,
        },

        [1] = {
                .x = 13,
                .y = 86,
                .z = 44,
        },

        [2] = {
                .x = 0.45645614564561,
                .y = 0.11211211121121,
                .z = 0.56656615665661,
        },

        [3] = {
                .x = 0.65265216526521,
                .y = 0.21421412142141,
                .z = 0.34134113413411,
        },

        [4] = {
                .x = 1,
                .y = 2,
                .z = 3,
        },

        [5] = {
                .x = 4,
                .y = 5,
                .z = 6,
        },

        [6] = {
                .x = -0.33333313333331,
                .y = -0.32132113213211,
                .z = -0.88989918898991,
        },

        [7] = {
                .x = -321,
                .y = -632,
                .z = -999,
        },
};

bool epsilon_same_float(float a, float b)
{
        return (fabs(a - b) < __FLT_EPSILON__);
}

bool epsilon_same_double(double a, double b)
{
        return (fabs(a - b) < __DBL_EPSILON__);
}

TEST_CASE_DECL(euclid_test_norm)
TEST_CASE_DECL(euclid_test_normf)

TEST_SUITE(euclid_test_all)
{
        euclid_test_norm();
        euclid_test_normf();
}

int
main(int argc, char **argv)
{
    euclid_test_all();
    return tu_any_failed;
}