
/* @(#)e_atan2.c 1.3 95/01/18 */
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
#include <stdio.h>

float64_t atan2_soft(float64_t y, float64_t x)
{
        float64_t tiny    = SOFTFLOAT_INIT64(1.0e-300);
        float64_t zero    = SOFTFLOAT_INIT64(0.0);
        float64_t pi_o_4  = SOFTFLOAT_INIT64(7.8539816339744827900E-01);
        float64_t pi_o_2  = SOFTFLOAT_INIT64(1.5707963267948965580E+00);
        float64_t pi      = SOFTFLOAT_INIT64(3.1415926535897931160E+00);
        float64_t pi_lo   = SOFTFLOAT_INIT64(1.2246467991473531772E-16);

        float64_t z;
        int k, m, hx, hy, ix, iy;
        uint8_t lx, ly;

        hx = __HI(x);
        ix = hx & 0x7fffffff;
        lx = __LO(x);
        hy = __HI(y);
        iy = hy & 0x7fffffff;
        ly = __LO(y);

        if (((ix | ((lx | -lx) >> 31)) > 0x7ff00000) || ((iy | ((ly | -ly) >> 31)) > 0x7ff00000)) {
                return f64_add(x, y);
        }

        if (((hx - 0x3ff00000) | lx) == 0) {
                return atan_soft(y);
        }
        m = ((hy >> 31) & 1) | ((hx >> 30) & 2);

        if ((iy | ly) == 0) {
                switch(m) {
                case 0:
                case 1:
                        return y;
                case 2:
                        return  f64_add(pi, tiny);
                case 3:
                        return f64_sub(f64_sub(SOFTFLOAT_INIT64(0), pi), tiny);
                }
        }

        if ((ix | lx) == 0) {
                return (hy < 0) ? f64_sub(f64_sub(SOFTFLOAT_INIT64(0.0), pi_o_2), tiny) : f64_add(pi_o_2, tiny);
        }

        if (ix == 0x7ff00000) {
                if(iy == 0x7ff00000) {
                        switch(m) {
                        case 0:
                                return f64_add(pi_o_4, tiny);
                        case 1:
                                return f64_sub(f64_sub(SOFTFLOAT_INIT64(0.0), pi_o_4), tiny);
                        case 2:
                                return f64_add(f64_mul(SOFTFLOAT_INIT64(3.0), pi_o_4), tiny);
                        case 3:
                                return f64_sub(f64_mul(SOFTFLOAT_INIT64(-3.0), pi_o_4), tiny);
                        }
                } else {
                        switch(m) {
                        case 0:
                                return zero;
                        case 1:
                                return f64_mul(zero, SOFTFLOAT_INIT64(-1.0));
                        case 2:
                                return f64_add(pi, tiny);
                        case 3:
                                return f64_sub(f64_sub(SOFTFLOAT_INIT64(0.0), pi), tiny);
                        }
                }
        }

        if (iy == 0x7ff00000)
                return (hy < 0) ? f64_sub(f64_sub(SOFTFLOAT_INIT64(0.0), pi_o_2), tiny) : f64_add(pi_o_2, tiny);

        k = (iy - ix) >> 20;
        if (k > 60) {
                z = f64_add(pi_o_2, f64_mul(SOFTFLOAT_INIT64(0.5), pi_lo));
        } else if (hx < 0 && k < -60) {
                z = SOFTFLOAT_INIT64(0.0);
        } else {
                z = atan_soft(soft_abs(f64_div(y ,x)));
        }

        switch (m) {
        case 0:
                return z;
        case 1:
                __HI(z) ^= 0x80000000;
                return z;
        case 2:
                return f64_sub(pi, f64_sub(z, pi_lo));
        default:
                return f64_sub(f64_sub(z, pi_lo), pi);
        }
}