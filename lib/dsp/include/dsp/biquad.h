/**
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


#ifndef _BIQUAD_H_
#define _BIQUAD_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dpl/dpl_types.h>

#define BIQUAD_N 3

typedef struct _biquad_status_t{
    uint16_t selfmalloc:1;
    uint16_t initialized:1;
}biquad_status_t;

typedef struct _biquad_instance_t{
    biquad_status_t status;
    dpl_float32_t num[BIQUAD_N];
    dpl_float32_t den[BIQUAD_N];
}biquad_instance_t;

biquad_instance_t * biquad_init(biquad_instance_t * inst);
void biquad_free(biquad_instance_t * inst);
dpl_float32_t biquad(biquad_instance_t * inst, dpl_float32_t x,
                        dpl_float32_t b[], dpl_float32_t a[], uint16_t clk);

#endif
