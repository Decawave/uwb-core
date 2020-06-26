/* origin: FreeBSD /usr/src/lib/msun/src/e_asin.c */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
/* asin(x)
 * Method :
 *      Since  asin(x) = x + x^3/6 + x^5*3/40 + x^7*15/336 + ...
 *      we approximate asin(x) on [0,0.5] by
 *              asin(x) = x + x*x^2*R(x^2)
 *      where
 *              R(x^2) is a rational approximation of (asin(x)-x)/x^3
 *      and its remez error is bounded by
 *              |(asin(x)-x)/x^3 - R(x^2)| < 2^(-58.75)
 *
 *      For x in [0.5,1]
 *              asin(x) = pi/2-2*asin(sqrt((1-x)/2))
 *      Let y = (1-x), z = y/2, s := sqrt(z), and pio2_hi+pio2_lo=pi/2;
 *      then for x>0.98
 *              asin(x) = pi/2 - 2*(s+s*z*R(z))
 *                      = pio2_hi - (2*(s+s*z*R(z)) - pio2_lo)
 *      For x<=0.98, let pio4_hi = pio2_hi/2, then
 *              f = hi part of s;
 *              c = sqrt(z) - f = (z-f*f)/(s+f)         ...f+c=sqrt(z)
 *      and
 *              asin(x) = pi/2 - 2*(s+s*z*R(z))
 *                      = pio4_hi+(pio4-2s)-(2s*z*R(z)-pio2_lo)
 *                      = pio4_hi+(pio4-2f)-(2s*z*R(z)-(pio2_lo+2c))
 *
 * Special cases:
 *      if x is NaN, return x itself;
 *      if |x|>1, return NaN with invalid signal.
 *
 */

#include "softfloat/softfloat.h"
#include "softfloat/softfloat_types.h"
float64_t asin_soft(float64_t x)
{
	float64_t one =    SOFTFLOAT_INIT64( 1.00000000000000000000e+00); /* 0x3FF00000, 0x00000000 */
	float64_t huge =   SOFTFLOAT_INIT64( 1.000e+300);
	float64_t pio2_hi =SOFTFLOAT_INIT64( 1.57079632679489655800e+00); /* 0x3FF921FB, 0x54442D18 */
	float64_t pio2_lo =SOFTFLOAT_INIT64( 6.12323399573676603587e-17); /* 0x3C91A626, 0x33145C07 */
	float64_t pio4_hi =SOFTFLOAT_INIT64( 7.85398163397448278999e-01); /* 0x3FE921FB, 0x54442D18 */
	float64_t pS0 =    SOFTFLOAT_INIT64( 1.66666666666666657415e-01); /* 0x3FC55555, 0x55555555 */
	float64_t pS1 =    SOFTFLOAT_INIT64(-3.25565818622400915405e-01); /* 0xBFD4D612, 0x03EB6F7D */
	float64_t pS2 =    SOFTFLOAT_INIT64( 2.01212532134862925881e-01); /* 0x3FC9C155, 0x0E884455 */
	float64_t pS3 =    SOFTFLOAT_INIT64(-4.00555345006794114027e-02); /* 0xBFA48228, 0xB5688F3B */
	float64_t pS4 =    SOFTFLOAT_INIT64( 7.91534994289814532176e-04); /* 0x3F49EFE0, 0x7501B288 */
	float64_t pS5 =    SOFTFLOAT_INIT64( 3.47933107596021167570e-05); /* 0x3F023DE1, 0x0DFDF709 */
	float64_t qS1 =    SOFTFLOAT_INIT64(-2.40339491173441421878e+00); /* 0xC0033A27, 0x1C8A2D4B */
	float64_t qS2 =    SOFTFLOAT_INIT64( 2.02094576023350569471e+00); /* 0x40002AE5, 0x9C598AC8 */
	float64_t qS3 =    SOFTFLOAT_INIT64(-6.88283971605453293030e-01); /* 0xBFE6066C, 0x1B8D0159 */
	float64_t qS4 =    SOFTFLOAT_INIT64( 7.70381505559019352791e-02); /* 0x3FB3B8C5, 0xB12E9282 */
	float64_t t, w, p, q, c, r, s;
	int hx, ix;

	hx = __HI(x);
	ix = hx & 0x7fffffff;

	if (ix >= 0x3ff00000) {
		 if(((ix - 0x3ff00000) | __LO(x)) == 0) {
			 return f64_add(f64_mul(x, pio2_hi), f64_mul(x, pio2_lo));
		 }
		 return f64_div(f64_sub(x, x), f64_sub(x, x));
	} else if (ix < 0x3fe00000) {
		if (ix < 0x3e400000) {
			if (f64_lt(one, f64_add(huge, x))) {
				return x;
			}
		} else {
			t = f64_mul(x, x);
		}

		p = f64_mul(t, f64_add(pS0, f64_mul(t, f64_add(pS1, f64_mul(t, f64_add(pS2, f64_mul(t, f64_add(pS3, f64_mul(t, f64_add(pS4, f64_mul(t, pS5)))))))))));
		q = f64_add(one, f64_mul(t, f64_add(qS1, f64_mul(t, f64_add(qS2, f64_mul(t, f64_add(qS3, f64_mul(t, qS4))))))));
		w = f64_div(p, q);
		return f64_add(x, f64_mul(x, w));
	}

	w = f64_sub(one, soft_abs(x));
	t = f64_mul(w, SOFTFLOAT_INIT64(0.5));
	p = f64_mul(t, f64_add(pS0, f64_mul(t, f64_add(pS1, f64_mul(t, f64_add(pS2, f64_mul(t, f64_add(pS3, f64_mul(t, f64_add(pS4, f64_mul(pS5, t)))))))))));
	q = f64_add(one, f64_mul(t, f64_add(qS1,  f64_mul(t, f64_add(qS2, f64_mul(t, f64_add(qS3, f64_mul(t, qS4))))))));
	s = f64_sqrt(t);

	if (ix >= 0x3FEF3333) {
		w = f64_div(p, q);
		t = f64_sub(pio2_hi, f64_sub(f64_mul(SOFTFLOAT_INIT64(2.0), f64_add(s, f64_mul(s, w))), pio2_lo));
	} else {
		w = s;
		__LO(w) = 0;
		c = f64_div(f64_sub(t, f64_mul(w, w)), f64_add(s, w));
		r = f64_div(p, q);
		p = f64_sub(f64_mul(SOFTFLOAT_INIT64(2.0), f64_mul(s, r)), f64_sub(pio2_lo, f64_mul(SOFTFLOAT_INIT64(2.0), c)));
		q = f64_sub(pio4_hi, f64_mul(SOFTFLOAT_INIT64(2.0), w));
		t = f64_sub(pio4_hi, f64_sub(p ,q));
	}
	if (hx > 0) {
		return t;
	} else {
		return f64_sub(SOFTFLOAT_INIT64(0.0), t);
	}
}
