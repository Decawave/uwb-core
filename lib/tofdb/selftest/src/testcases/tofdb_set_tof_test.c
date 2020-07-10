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
 * KIND, either expess or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "tofdb_test.h"
#include "rng_math/rng_math.h"
#include <math.h>

struct tofdb_node* tofdb_get_nodes();

TEST_CASE_SELF(tofdb_set_tof_test)
{
    uint32_t tof = 0;
    int rc = 1;
    int i = 0;
    int val_ctrl = 1;

    /* Test case 1
     *
     * Test setting single value
     *
     */
    rc = tofdb_set_tof(0x1, 1);

    TEST_ASSERT(rc == OS_OK);

    rc = tofdb_get_tof(0x1, &tof);

    TEST_ASSERT(rc == OS_OK);

    TEST_ASSERT(tof == 1);

    /* Clear nodes */
    clear_nodes();

    /* Test case 2
     *
     * Fill up all possible nodes
     *
     */
    for (i = 1; i < MYNEWT_VAL(TOFDB_MAXNUM_NODES); i++) {
        rc = tofdb_set_tof(i, i);
        TEST_ASSERT(rc == OS_OK);
    }

    /* Check values in nodes */
    for (i = 1; i < MYNEWT_VAL(TOFDB_MAXNUM_NODES); i++) {
        rc = tofdb_get_tof(i, &tof);
        TEST_ASSERT(rc == OS_OK);
        if (tof != i) {
            val_ctrl = 0;
            continue;
        }
    }

    TEST_ASSERT(val_ctrl);

    /* Clear nodes */
    clear_nodes();

    /* Test case 3
     *
     * Setting tof values to 0
     *
     */
    for (i = 0; i < MYNEWT_VAL(TOFDB_MAXNUM_NODES); i++) {
        rc = tofdb_set_tof(i, 0);
        TEST_ASSERT(rc == OS_OK);
    }

    /* Check values in nodes */
    for (i = 1; i < MYNEWT_VAL(TOFDB_MAXNUM_NODES); i++) {
        rc = tofdb_get_tof(i, &tof);
        TEST_ASSERT(rc == OS_OK);
        if (tof != 0) {
            val_ctrl = 0;
            continue;
        }
    }

    TEST_ASSERT(val_ctrl);

    /* Clear nodes */
    clear_nodes();


    /* Test case 4
     *
     * Check 2M filtering functionality
     *
     */
    rc = tofdb_set_tof(0x1, 200);

    TEST_ASSERT(rc == OS_OK);

    rc = tofdb_set_tof(0x1, 250);

    TEST_ASSERT(rc == OS_OK);

    rc = tofdb_get_tof(0x1, &tof);

    TEST_ASSERT(rc == OS_OK && tof == 200);

    /* Clear nodes */
    clear_nodes();

    /* Test case 5
     *
     * Check OS_ENOMEM
     *
     */
    for (i = 1; i <= MYNEWT_VAL(TOFDB_MAXNUM_NODES); i++) {
        rc = tofdb_set_tof(i, 1);
        TEST_ASSERT(rc == OS_OK);
    }

    rc = tofdb_set_tof(40, 1);

    TEST_ASSERT(rc == OS_ENOMEM);
}

