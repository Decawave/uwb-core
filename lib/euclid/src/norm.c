/*
 * Copyright 2017 Paul Kettle
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file norm.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date March 27 2017
 * @brief Euclidean distance.
 * @details
 * ## Algorithm Details
 * The Euclidean distance for two triads is
 *
 * \f$(\hat{x},\hat{y},\hat{z})\f$ and anchor \f$(x_i,y_i,z_i)\f$ is \f$\sqrt{(\hat{x} - x_i)^2+(\hat{y} - y_i)^2+(\hat{z} - z_i)^2}\f$
 *
 * The Euclidean distance is the L2_NORM for 3D and the L1_NORM for 2D usecase.
 */

#include <assert.h>
#include <math.h>
#include <euclid/triad.h>

/**
 * @brief Euclidean distance
 * @param p1 first tryad point
 * @param p2 second tryad point
 * @param dim select L0_NORM, L1_NORM or L2_NORM
 */
double norm(triad_t * p1, triad_t * p2, uint8_t dim){

    assert(dim<4);

    double distance = 0.;
    for (uint8_t i=0; i < dim; i++)
        distance += (p1->array[i] - p2->array[i]) * (p1->array[i] - p2->array[i]);

    return sqrt(distance);
}

float normf(triadf_t * p1, triadf_t * p2, uint8_t dim){

    assert(dim<4);

    float distance = 0.;
    for (uint8_t i=0; i < dim; i++)
        distance += (p1->array[i] - p2->array[i]) * (p1->array[i] - p2->array[i]);

    return sqrtf(distance);
}
