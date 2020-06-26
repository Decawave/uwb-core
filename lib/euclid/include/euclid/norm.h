/*
  Copyright 2017 Paul Kettle

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

/**
 * @file norm.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date March 27 2017
 * @brief Euclidean distance for a triad L2-norm
 *
 */

#ifndef __NORM_H__
#define __NORM_H__

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <euclid/triad.h>

#define L0_NORM 1
#define L1_NORM 2
#define L2_NORM 3

/**
 * @brief
 * @param p1 and p2 and cartesion coordinate triads
 * @param dim is set to L1_NORM or L2_NORM
 */

float normf(triadf_t * p1, triadf_t * p2, uint8_t dim);
double norm(triad_t * p1, triad_t * p2, uint8_t dim);

#endif
