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


#ifndef _SOSFILT_H_
#define _SOSFILT_H_

#include <dsp/biquad.h>

typedef struct _sos_status_t{
    uint16_t selfmalloc:1;
    uint16_t initialized:1;
}sos_status_t;

typedef struct _sos_instance_t {
    sos_status_t status;
    uint8_t clk;
    uint8_t nsize;
    biquad_instance_t * biquads[];
}sos_instance_t;

sos_instance_t * sosfilt_init(sos_instance_t * inst, uint16_t nsize);
void sosfilt_free(sos_instance_t * inst);
float sosfilt(sos_instance_t * inst, float x, float b[], float a[]);

#endif
