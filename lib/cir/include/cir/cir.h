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

#ifndef _CIR_H_
#define _CIR_H_

#include <stdlib.h>
#include <stdint.h>
#include <dpl/dpl.h>
#include <stats/stats.h>
#include <uwb/uwb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cir_status_t{
    uint16_t selfmalloc:1;
    uint16_t initialized:1;
    uint16_t valid:1;
    uint16_t lde_override:1;
}cir_status_t;

struct cir_instance;

/**
 * Get the Phase Difference of Arrival
 *
 * @param master  Pointer to struct cir_instance.
 * @param slave   Pointer to struct cir_instance (optional if receiver can extract pdoa with a single receiver).
 *
 * @return pdoa Phase difference of arrival in radians
 */
typedef dpl_float32_t (*cir_get_pdoa_func_t)(struct cir_instance * master, struct cir_instance *slave);

/**
 * Enable/disable CIR for next RX cycle
 *
 * @param cir  Pointer to struct cir_instance.
 * @param mode True = Enable, False = Disable
 *
 * @return pdoa Phase difference of arrival in radians
 */
typedef void (*cir_enable_func_t)(struct cir_instance * cir, bool mode);

struct cir_driver_funcs {
    cir_get_pdoa_func_t cf_cir_get_pdoa;
    cir_enable_func_t cf_cir_enable;
};

struct cir_instance {
    const struct cir_driver_funcs *cir_funcs;
    cir_status_t status;
};


/**
 * Get the Phase Difference of Arrival
 *
 * @param master  Pointer to struct cir_instance.
 * @param slave   Pointer to struct cir_instance (optional).
 *
 * @return pdoa Phase difference of arrival in radians
 */
static inline dpl_float32_t
cir_get_pdoa(struct cir_instance * master, struct cir_instance *slave)
{
    return (master->cir_funcs->cf_cir_get_pdoa(master, slave));
}

/**
 * Enable CIR for next RX cycle
 *
 * @param cir  Pointer to struct cir_instance.
 *
 * @return pdoa Phase difference of arrival in radians
 */
static inline void
cir_enable(struct cir_instance * cir, bool mode)
{
    (cir->cir_funcs->cf_cir_enable(cir, mode));
}

/**
 * Calculate Angle of Arrival
 *
 * @param pdoa  Phase difference of arrival in radians
 * @param wavelength  Wavelength in meters
 * @param antenna_separation  Antenna separation in meters
 *
 * @return aoa Angle of arrival in radians
 */
dpl_float32_t cir_calc_aoa(dpl_float32_t pdoa, dpl_float32_t wavelength, dpl_float32_t antenna_separation);

#ifdef __cplusplus
}
#endif

#endif /* _CIR_H_ */
