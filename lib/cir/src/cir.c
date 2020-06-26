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

/**
 * @file cir.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date Nov 29 2019
 * @brief Channel Impulse Response
 *
 * @details
 *
 */

#include <stdio.h>
#include <math.h>
#include <dpl/dpl.h>
#include <cir/cir.h>

#if __KERNEL__
void cir_chrdev_pkg_init(void);
int cir_chrdev_pkg_down(int reason);
#endif

/*!
 * @fn cir_calc_aoa(float pdoa, float wavelength, float antenna_separation)
 *
 * @brief Calculate the phase difference between receivers
 *
 * @param pdoa       - phase difference of arrival in radians
 * @param wavelength - wavelength of the radio channel used in meters
 * @param antenna_separation - distance between centers of antennas in meters
 *
 * output parameters
 *
 * returns angle of arrival - float, in radians
 */
dpl_float32_t
cir_calc_aoa(dpl_float32_t pdoa, dpl_float32_t wavelength, dpl_float32_t antenna_separation)
{
    dpl_float32_t aoa, pd_dist;
#if __KERNEL__
    dpl_float32_t divider = DPL_FLOAT32_MUL(DPL_FLOAT32_INIT(2.0f), DPL_FLOAT32_INIT(M_PI));
    divider = DPL_FLOAT32_MUL(divider, wavelength);
    pd_dist = DPL_FLOAT32_DIV(pdoa, divider);
    pd_dist = DPL_FLOAT32_DIV(pd_dist, antenna_separation);
    aoa = DPL_FLOAT32_FROM_F64(DPL_FLOAT64_ASIN(DPL_FLOAT64_FROM_F32(pd_dist)));
#else
    pd_dist = pdoa / (DPL_FLOAT32_INIT(2.0f)*M_PI) * wavelength;
    aoa = asinf(pd_dist / antenna_separation);
#endif
    return aoa;
}


/**
 * @fn cir_pkg_init(void)
 * @brief initialise
 *
 * @return void
 */
void
cir_pkg_init(void)
{
#ifdef __KERNEL__
    cir_chrdev_pkg_init();
    pr_info("uwbcir: cat /dev/uwbcir\n");
#endif  /* __KERNEL__ */
}

/**
 * @fn cir_pkg_down(void)
 * @brief Uninitialise cir
 *
 * @return void
 */
int
cir_pkg_down(int reason)
{
#if __KERNEL__
    cir_chrdev_pkg_down(reason);
#endif
    return 0;
}
