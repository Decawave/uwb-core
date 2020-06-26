/* @(#)s_atan.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

#include <softfloat/softfloat.h>
#include <softfloat/softfloat_types.h>

float64_t fmod_soft(float64_t x, float64_t y)
{
    float64_t A = f64_div(x, y);
    if (f64_lt(x, SOFTFLOAT_INIT64(0.0))) {
        A = f64_roundToInt(A, softfloat_round_max, true);
    } else {
        A = f64_roundToInt(A, softfloat_round_min, true);
    }
    A = f64_mul(A, y);
    A = f64_sub(x, A);
    return A;
}

float32_t fmodf_soft(float32_t x, float32_t y)
{
    float32_t A = f32_div(x, y);
    if (f32_lt(x, SOFTFLOAT_INIT32(0.0f))) {
        A = f32_roundToInt(A, softfloat_round_max, true);
    } else {
        A = f32_roundToInt(A, softfloat_round_min, true);
    }
    A = f32_mul(A, y);
    A = f32_sub(x, A);
    return A;
}
