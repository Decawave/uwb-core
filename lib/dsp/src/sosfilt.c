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

#include <assert.h>
#include <dsp/sosfilt.h>

sos_instance_t * sosfilt_init(sos_instance_t * inst, uint16_t nsize)
{
    uint8_t i;

    if (inst == NULL) {
        inst = (sos_instance_t *) malloc(sizeof(sos_instance_t) + nsize *
                                                sizeof(biquad_instance_t *));
        assert(inst);
        memset(inst, 0, sizeof(sos_instance_t));
        inst->status.selfmalloc = 1;
        for (i = 0; i < nsize; i++) {
            inst->biquads[i] = biquad_init(NULL);
        }
        } else {
            assert(inst->nsize == nsize);
        }
        inst->nsize = nsize;
        return inst;
}

void sosfilt_free(sos_instance_t * inst)
{

    uint8_t i;

    assert(inst);

    for (i = 0; i < inst->nsize; i++) {
        biquad_free(inst->biquads[i]);
    }
}

dpl_float32_t sosfilt(sos_instance_t * inst, dpl_float32_t x,
                                        dpl_float32_t b[], dpl_float32_t a[])
{
    uint8_t i;
    dpl_float32_t result = x;

    for (i = 0; i < inst->nsize; i++)
        result = biquad(inst->biquads[i], result, &b[i*BIQUAD_N], &a[i*BIQUAD_N], inst->clk);

    inst->clk++;

    return result;
}
