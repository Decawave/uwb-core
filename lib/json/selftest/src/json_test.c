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
#include "json_test.h"
#include "json_test_priv.h"

#define JSON_BIGBUF_SIZE 192

TEST_CASE_DECL(json_encode_test)
TEST_CASE_DECL(json_decode_test)

TEST_SUITE(json_test_all)
{
    bigbuf = malloc(JSON_BIGBUF_SIZE);
    TEST_ASSERT_FATAL(bigbuf != NULL);

    json_encode_test();
    json_decode_test();

free(bigbuf);
}

int main(int argc, char **argv)
{
    json_test_all();
    return tu_any_failed;
}
