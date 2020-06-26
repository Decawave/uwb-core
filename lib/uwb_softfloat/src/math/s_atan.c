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

//#include "softfloat/softfloat.h"
#include "softfloat/softfloat.h"
#include <softfloat/softfloat_types.h>

float64_t atan_soft(float64_t x)
{
        float64_t atanhi[] = {
                SOFTFLOAT_INIT64(4.63647609000806093515e-01), /* atan(0.5)hi 0x3FDDAC67, 0x0561BB4F */
                SOFTFLOAT_INIT64(7.85398163397448278999e-01), /* atan(1.0)hi 0x3FE921FB, 0x54442D18 */
                SOFTFLOAT_INIT64(9.82793723247329054082e-01), /* atan(1.5)hi 0x3FEF730B, 0xD281F69B */
                SOFTFLOAT_INIT64(1.57079632679489655800e+00), /* atan(inf)hi 0x3FF921FB, 0x54442D18 */
      };

        float64_t atanlo[] = {
                SOFTFLOAT_INIT64(2.26987774529616870924e-17), /* atan(0.5)lo 0x3C7A2B7F, 0x222F65E2 */
                SOFTFLOAT_INIT64(3.06161699786838301793e-17), /* atan(1.0)lo 0x3C81A626, 0x33145C07 */
                SOFTFLOAT_INIT64(1.39033110312309984516e-17), /* atan(1.5)lo 0x3C700788, 0x7AF0CBBD */
                SOFTFLOAT_INIT64(6.12323399573676603587e-17), /* atan(inf)lo 0x3C91A626, 0x33145C07 */
        };

        float64_t aT[] = {
                SOFTFLOAT_INIT64( 3.33333333333329318027e-01), /* 0x3FD55555, 0x5555550D */
                SOFTFLOAT_INIT64(-1.99999999998764832476e-01), /* 0xBFC99999, 0x9998EBC4 */
                SOFTFLOAT_INIT64( 1.42857142725034663711e-01), /* 0x3FC24924, 0x920083FF */
                SOFTFLOAT_INIT64(-1.11111104054623557880e-01), /* 0xBFBC71C6, 0xFE231671 */
                SOFTFLOAT_INIT64( 9.09088713343650656196e-02), /* 0x3FB745CD, 0xC54C206E */
                SOFTFLOAT_INIT64(-7.69187620504482999495e-02), /* 0xBFB3B0F2, 0xAF749A6D */
                SOFTFLOAT_INIT64( 6.66107313738753120669e-02), /* 0x3FB10D66, 0xA0D03D51 */
                SOFTFLOAT_INIT64(-5.83357013379057348645e-02), /* 0xBFADDE2D, 0x52DEFD9A */
                SOFTFLOAT_INIT64( 4.97687799461593236017e-02), /* 0x3FA97B4B, 0x24760DEB */
                SOFTFLOAT_INIT64(-3.65315727442169155270e-02), /* 0xBFA2B444, 0x2C6A6C2F */
                SOFTFLOAT_INIT64( 1.62858201153657823623e-02), /* 0x3F90AD3A, 0xE322DA11 */
        };

        float64_t one = SOFTFLOAT_INIT64(1.0);
        float64_t huge = SOFTFLOAT_INIT64(1.0e300);
        float64_t w, s1, s2, z;
        int ix, hx, id;

        hx = __HI(x);
        ix = hx & 0x7fffffff;

        if (ix >= 0x44100000) {
                if (ix > 0x7ff00000 || (ix == 0x7ff00000 && (__LO(x) != 0)))
                        return f64_add(x, x);
                if (hx > 0) {
                        return f64_add(atanhi[3], atanlo[3]);
                } else {
                        return f64_sub(f64_sub(SOFTFLOAT_INIT64(0.0), atanhi[3]), atanlo[3]);
                }
        }

        if (ix < 0x3fdc0000) {
                if (ix < 0x3e200000) {
                        if (f64_lt(one, f64_add(huge, x)))
                                return x;
                }
                id = -1;
        } else {
                x = soft_abs(x);
                if (ix < 0x3ff30000) {
                        if (ix < 0x3fe60000) {
                                id = 0;
                                x = f64_div(f64_sub(f64_mul(SOFTFLOAT_INIT64(2.0), x), one), f64_add(x, SOFTFLOAT_INIT64(2.0)));
                        } else {
                                id = 1;
                                x = f64_div(f64_sub(x, one), f64_add(x, one));
                        }
                } else {
                        if (ix < 0x40038000) {
                                id = 2;
                                x = f64_div(f64_sub(x, SOFTFLOAT_INIT64(1.5)), f64_add(one, f64_mul(SOFTFLOAT_INIT64(1.5), x)));
                        } else {
                                id = 3;
                                x = f64_div(SOFTFLOAT_INIT64(-1.0), x);
                        }
                }
        }
        z = f64_mul(x, x);
        w = f64_mul(z, z);

        s1 = f64_mul(z, f64_add(aT[0], f64_mul(w, f64_add(aT[2], f64_mul(w, f64_add(aT[4], f64_mul(w, f64_add(aT[6], f64_mul(w, f64_add(aT[8], f64_mul(w , aT[10])))))))))));
        s2 = f64_mul(w, f64_add(aT[1], f64_mul(w, f64_add(aT[3], f64_mul(w, f64_add(aT[5], f64_mul(w, f64_add(aT[7], f64_mul(w, aT[9])))))))));

        if (id < 0) {
                return f64_sub(x, f64_mul(x, f64_add(s1, s2)));
        } else {
                z = f64_sub(atanhi[id], f64_sub(f64_sub(f64_mul(x, f64_add(s1, s2)), atanlo[id]), x));
                return (hx < 0) ? f64_sub(SOFTFLOAT_INIT64(0.0), z) : z;
        }
}
