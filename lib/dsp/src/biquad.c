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

#include <dsp/biquad.h>
#include <stdio.h>
#include <assert.h>

/**
 * Initilize biquad
 */
biquad_instance_t * biquad_init(biquad_instance_t * inst)
{

    if (inst == NULL) {
        inst = (biquad_instance_t *) malloc(sizeof(biquad_instance_t));
        assert(inst);
        memset(inst, 0, sizeof(biquad_instance_t));
        inst->status.selfmalloc = 1;
    }

    return inst;
}

void biquad_free(biquad_instance_t * inst)
{
    if (inst->status.selfmalloc)
        free(inst);
}


dpl_float32_t biquad(biquad_instance_t * inst, dpl_float32_t x,
                            dpl_float32_t b[], dpl_float32_t a[], uint16_t clk)
{

    dpl_float32_t y = 0;
    int16_t i;
    // Implementing y(n) = (-a1 y(n-1) -a2 y(n-2) + b0 x(n) + b1 x(n-1) + b2 x(n-2))/a0
    // taps := [y(n), y(n-1), y(n-2), x(n), x(n-1), x(n-2)]
    // b:= [b0, b1, b2]
    // a:= [a0, a1, a2]

    inst->den[clk % BIQUAD_N] = y;

    for (i = 0; i < BIQUAD_N; i++) {
        y = DPL_FLOAT32_ADD(y, DPL_FLOAT32_MUL(b[i], inst->num[(clk - i + BIQUAD_N) % (BIQUAD_N)]));
    }

    for (i = 1; i < BIQUAD_N; i++) {
        y = DPL_FLOAT32_SUB(y, DPL_FLOAT32_MUL(a[i], inst->den[(clk - i + BIQUAD_N) % (BIQUAD_N)]));
    }

    y = DPL_FLOAT32_DIV(y, a[0]);

    return y;
}
