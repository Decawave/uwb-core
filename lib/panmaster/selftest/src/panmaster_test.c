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

#include "panmaster_test.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stats/stats.h>

TEST_CASE_DECL(pan_os_set_callbacks)
TEST_CASE_DECL(pan_os_add_nodes)
TEST_CASE_DECL(pan_os_node_roles)
TEST_CASE_DECL(pan_os_lease_time)
TEST_CASE_DECL(pan_os_slot_id)
TEST_CASE_DECL(pan_os_same_slot_id)
TEST_CASE_DECL(pan_os_lease_time_expire)

TEST_SUITE(panmaster_test_all)
{
    pan_os_set_callbacks();
    pan_os_add_nodes();
    pan_os_node_roles();
    pan_os_lease_time();
    pan_os_slot_id();
    pan_os_same_slot_id();
    pan_os_lease_time_expire();
}

int main(int argc, char **argv)
{
    panmaster_test_all();
    return tu_any_failed;
}
