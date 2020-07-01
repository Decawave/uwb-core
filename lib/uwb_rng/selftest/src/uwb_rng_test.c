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
 * KIND, either expess or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdio.h>

#include "uwb_rng_json_test.h"
#include <math.h>

TEST_CASE_DECL(uwb_rng_json_test_read)
TEST_CASE_DECL(uwb_rng_json_test_write)

TEST_SUITE(uwb_rng_test_all)
{
    uwb_rng_json_test_read();
    uwb_rng_json_test_write();
}

bool epsilon_same_float(float a, float b)
{
        return (fabs(a - b) < __FLT_EPSILON__);
}

bool epsilon_same_double(double a, double b)
{
        return (fabs(a - b) < __DBL_EPSILON__);
}

bool compare_rssi(dpl_float64_t *rssi_exp, dpl_float64_t *rssi_in)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (isnan(rssi_exp[i]) && isnan(rssi_in[i])) {
            return true;
        } else if (rssi_exp[i] == rssi_in[i]) {
            return true;
        }
    }

    return true;
}

bool compare_los(dpl_float64_t *los_exp, dpl_float64_t *los_in)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (isnan(los_exp[i]) && isnan(los_in[i])) {
            return true;
        } else if (los_exp[i] == los_in[i]) {
            return true;
        }
    }

    return true;
}

bool compare_axis(triad_t *raz_exp, triad_t *raz_in)
{
    return isnan(raz_exp->x) && isnan(raz_in->x);
}

bool compare_spherical(triad_t *raz_exp, triad_t *raz_in)
{
    return isnan(raz_exp->range) && isnan(raz_in->range);
}

bool compare_triad_arr(triad_t *raz_exp, triad_t *raz_in)
{
    return isnan(raz_exp->array[0]) && isnan(raz_in->array[0]);
}

bool compare_raz(triad_t *raz_exp, triad_t *raz_in)
{
    int i;
    int array_size;
    bool axis_ctrl = true;
    bool spherical_ctrl = true;
    bool array_ctrl = true;

    if (compare_axis(raz_exp, raz_in) == false) {
        if (raz_exp->x != raz_in->x) {
            axis_ctrl = false;
        }
        if (raz_exp->y != raz_in->y) {
            axis_ctrl = false;
        }
        if (raz_exp->z != raz_in->z) {
            axis_ctrl = false;
        }
    }

    if (compare_spherical(raz_exp, raz_in) == false) {
        if (raz_exp->range != raz_in->range) {
            spherical_ctrl = false;
        }
        if (raz_exp->azimuth != raz_in->azimuth) {
            spherical_ctrl = false;
        }
        if (raz_exp->zenith != raz_in->zenith) {
            spherical_ctrl = false;
        }
    }

    array_size = sizeof(raz_in->array);

    if (compare_triad_arr(raz_exp, raz_in) == false) {
        for (i = 0; i < array_size; i++) {
            if (!epsilon_same_float(raz_exp->array[i], raz_exp->array[i])) {
                array_ctrl = false;
            }
        }
    }

    return axis_ctrl && spherical_ctrl && array_ctrl;
}

bool check_nan_or_equal(dpl_float64_t exp, dpl_float64_t in)
{
    if (isnan(exp) && isnan(in)) {
        return true;
    } else if (exp == in) {
        return true;
    }

    return false;
}

bool compare_json_rng_structs(rng_json_t *json_exp, rng_json_t *json_in)
{
    int rc;

    if (json_exp->utime != json_in->utime) {
        return false;
    }
    if (json_exp->seq != json_in->seq) {
        return false;
    }
    if (json_exp->uid != json_in->uid) {
        return false;
    }
    if (json_exp->ouid != json_in->ouid) {
        return false;
    }

    rc = check_nan_or_equal(json_exp->ppm, json_in->ppm);
    if (rc == 0) {
        return false;
    }

    rc = check_nan_or_equal(json_exp->sts, json_in->sts);
    if (rc == 0) {
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    uwb_rng_test_all();
    return tu_any_failed;
}
