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
#include <dsp/polyval.h>

dpl_float32_t polyval(dpl_float32_t p[], dpl_float32_t x, uint16_t nsize) {

    uint8_t i;
    dpl_float32_t _x = x;
    dpl_float32_t y = p[nsize - 1];

    for (i = 1; i < nsize; i++, _x = DPL_FLOAT32_MUL(_x, x)) {
        y = DPL_FLOAT32_ADD(y, DPL_FLOAT32_MUL(_x, p[nsize - i - 1]));
    }

    return y;
}
