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
 * @file listener_json.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date  May 5 2020
 * @brief
 *
 */

#ifndef _LISTENER_JSON_H_
#define _LISTENER_JSON_H_

#include <json/json.h>
#include <json/json_util.h>
#include <dpl/dpl.h>
#include <dpl/dpl_types.h>
#include <linux/spinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

struct listener_json {
    /* json_decoder must be first element in the structure */
    struct json_decoder decoder;
    struct json_encoder encoder;
    uint64_t utime;
    uint64_t wcs_utime;
    dpl_float64_t rssi;
    uint64_t data_len;
    char data[2048];
    /* */
    spinlock_t lock;
    char iobuf[PAGE_SIZE];
    uint16_t idx;
};

int listener_json_write(struct listener_json * json);
void listener_chrdev_destroy(int idx);
int listener_chrdev_create(int idx);
int  listener_chrdev_output(int idx, char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
