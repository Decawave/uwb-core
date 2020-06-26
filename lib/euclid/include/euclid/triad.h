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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file triad.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date March 27 2017
 * @brief latitude, longitude, elevation tryad
 *
 */

#ifndef _TRIAD_H_
#define _TRIAD_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief triad definition
 */

#ifndef __KERNEL__
typedef union __triad16_t{
    struct _axis16{
        int16_t x,y,z;
    };
    struct _int16_spherical{
        int16_t range, azimuth, zenith;
    };
    int16_t array[sizeof(struct _axis16)/sizeof(int16_t)];
}triad16_t;

typedef union _triad_t{
    struct _axis{
        double  x,y,z;
    };
    struct _spherical{
        double range, azimuth, zenith;
    };
    double array[sizeof(struct _axis)/sizeof(double)];
}triad_t;

typedef union _triadf_t{
    struct _axisf{
        float  x,y,z;
    };
    struct _sphericalf{
        float range, azimuth, zenith;
    };
    float array[sizeof(struct _axisf)/sizeof(float)];
}triadf_t;

#else

typedef union _triad_t{
    struct _axis{
        dpl_float64_t  x,y,z;
    };
    struct _spherical{
        dpl_float64_t range, azimuth, zenith;
    };
    dpl_float64_t array[sizeof(struct _axis)/sizeof(dpl_float64_t)];
}triad_t;

typedef union _triadf_t{
    struct _axisf{
        dpl_float32_t  x,y,z;
    };
    struct _sphericalf{
        dpl_float32_t range, azimuth, zenith;
    };
    dpl_float32_t array[sizeof(struct _axis)/sizeof(dpl_float32_t)];
}triadf_t;

#endif //__KERNEL__

#ifdef __cplusplus
}
#endif
#endif
