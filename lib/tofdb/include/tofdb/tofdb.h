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

#ifndef _TOFDB_H_
#define _TOFDB_H_

#include <inttypes.h>
#include <bootutil/image.h>
struct image_version;

struct tofdb_node {
    uint16_t addr;           /*!< Local id, 16bit */
    uint32_t last_updated;
    float tof;
    float sum;
    float sum_sq;
    uint32_t num;
};

#ifdef __cplusplus
extern "C" {
#endif

int tofdb_get_tof(uint16_t addr, uint32_t *tof);
int tofdb_set_tof(uint16_t addr, uint32_t tof);

#ifdef __cplusplus
}
#endif

#endif /* _TOFDB_H */
