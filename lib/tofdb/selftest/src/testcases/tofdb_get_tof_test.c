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

TEST_CASE_SELF(tofdb_get_tof_test)
{
    uint32_t tof = 0;
    int rc;

    /* Test case 1
     *
     * Simply get value from nodes
     *
     */

    /* Firstly let's fill single node */
    rc = tofdb_set_tof(0x123, 666);

    TEST_ASSERT(rc == OS_OK);

    /* Getting the value back from address 0x123 */
    rc = tofdb_get_tof(0x123, &tof);

    TEST_ASSERT(rc == OS_OK);

    TEST_ASSERT(tof == 666);

    clear_nodes();
    tof = 0;

    /* Test case 2
     *
     * Getting value from invalid address
     *
     */
    rc = tofdb_set_tof(0x123, 666);

    TEST_ASSERT(rc == OS_OK);

    rc = tofdb_get_tof(0x124, &tof);

    TEST_ASSERT(rc == OS_ENOENT);

    clear_nodes();
    tof = 0;

    /* Test case 3
     *
     * Passing NULL as tof
     *
     */
    rc = tofdb_get_tof(0x100, NULL);

    TEST_ASSERT(rc == OS_EINVAL);

    clear_nodes();
    tof = 0;
}
