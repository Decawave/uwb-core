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
#include "soft_test.h"
#include "softfloat/softfloat.h"
#include <math.h>

/* Test fmod */
TEST_CASE_SELF(test_fmod)
{
    float64_t out;
    float64_t exp;
    float32_t o32;
    float32_t e32;
    int rc;

    /* Case 1 */
    exp = SOFTFLOAT_INIT64(fmod(0.0, 1.0));
    out = fmod_soft(SOFTFLOAT_INIT64(0.0), SOFTFLOAT_INIT64(1.0));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 1 */
    exp = SOFTFLOAT_INIT64(fmod(5.5, 1.0));
    out = fmod_soft(SOFTFLOAT_INIT64(5.5), SOFTFLOAT_INIT64(1.0));
    rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    TEST_ASSERT(!rc);

    /* Case 2 */
    exp = SOFTFLOAT_INIT64(fmod(-12.4, 3.14));
    out = fmod_soft(SOFTFLOAT_INIT64(-12.4), SOFTFLOAT_INIT64(3.14));
    //rc = memcmp(&exp.v, &out.v, sizeof(float64_t));
    rc = f64_lt(SOFTFLOAT_INIT64(1.0e-8), soft_abs(f64_sub(exp, out)));
    TEST_ASSERT(!rc);

    /* Case 3 */
    e32 = SOFTFLOAT_INIT32(fmodf(0.0f, 1.0f));
    o32 = fmodf_soft(SOFTFLOAT_INIT32(0.0f), SOFTFLOAT_INIT32(1.0f));
    rc = memcmp(&e32.v, &o32.v, sizeof(float32_t));
    TEST_ASSERT(!rc);

    /* Case 4 */
    e32 = SOFTFLOAT_INIT32(fmodf(5.5f, 1.0f));
    o32 = fmodf_soft(SOFTFLOAT_INIT32(5.5f), SOFTFLOAT_INIT32(1.0f));
    rc = memcmp(&e32.v, &o32.v, sizeof(float32_t));
    TEST_ASSERT(!rc);

    /* Case 5 */
    e32 = SOFTFLOAT_INIT32(fmodf(-12.4f, 3.14f));
    o32 = fmodf_soft(SOFTFLOAT_INIT32(-12.4f), SOFTFLOAT_INIT32(3.14f));
    //rc = memcmp(&exp.v, &out.v, sizeof(float32_t));
    rc = f32_lt(SOFTFLOAT_INIT32(1.0e-6), soft_abs32(f32_sub(e32, o32)));
    TEST_ASSERT(!rc);
}
