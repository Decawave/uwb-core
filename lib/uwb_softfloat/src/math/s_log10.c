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

#include "softfloat/softfloat.h"
#include "softfloat/softfloat_types.h"

float64_t log_soft(float64_t x)
{
	float64_t ln2_hi = SOFTFLOAT_INIT64(6.93147180369123816490e-01);	/* 3fe62e42 fee00000 */
	float64_t ln2_lo = SOFTFLOAT_INIT64(1.90821492927058770002e-10);	/* 3dea39ef 35793c76 */
	float64_t two54  = SOFTFLOAT_INIT64(1.80143985094819840000e+16);  /* 43500000 00000000 */
	float64_t Lg1 = SOFTFLOAT_INIT64(6.666666666666735130e-01);  /* 3FE55555 55555593 */
	float64_t Lg2 = SOFTFLOAT_INIT64(3.999999999940941908e-01);  /* 3FD99999 9997FA04 */
	float64_t Lg3 = SOFTFLOAT_INIT64(2.857142874366239149e-01);  /* 3FD24924 94229359 */
	float64_t Lg4 = SOFTFLOAT_INIT64(2.222219843214978396e-01);  /* 3FCC71C5 1D8E78AF */
	float64_t Lg5 = SOFTFLOAT_INIT64(1.818357216161805012e-01);  /* 3FC74664 96CB03DE */
	float64_t Lg6 = SOFTFLOAT_INIT64(1.531383769920937332e-01);  /* 3FC39A09 D078C69F */
	float64_t Lg7 = SOFTFLOAT_INIT64(1.479819860511658591e-01);  /* 3FC2F112 DF3E5244 */
	float64_t hfsq, f, s, z, R, w, t1, t2, dk;
	float64_t zero = i32_to_f64(0);

	int k, hx, i, j;
	unsigned lx;

	hx = __HI(x);		/* high word of x */
	lx = __LO(x);		/* low  word of x */

	k = 0;
	if (hx < 0x00100000) {
	    if (((hx & 0x7fffffff) | lx) == 0)
	    	return f64_div(f64_sub(SOFTFLOAT_INIT64(0), two54), zero);
	    if (hx < 0)
	    	return f64_div(f64_sub(x, x), zero);
	k -= 54;
	x = f64_mul(x, two54);
	hx = __HI(x);
	}

	if (hx >= 0x7ff00000)
		return f64_add(x, x);

	k += (hx >> 20) - 1023;
	hx &= 0x000fffff;
	i = (hx + 0x95f64) & 0x100000;
	__HI(x) = hx | (i ^ 0x3ff00000);
	k += (i >> 20);
	f = f64_sub(x, SOFTFLOAT_INIT64(1.0));

	if ((0x000fffff & (2 + hx)) < 3) {
		if (f64_eq(f, zero)) {
			if (k == 0) {
				return zero;
			}  else {
				dk = i32_to_f64(k);
				return f64_add(f64_mul(dk, ln2_hi), f64_mul(dk, ln2_lo));
			}
		}

		R = f64_mul(f64_mul(f, f), f64_sub(SOFTFLOAT_INIT64(0.5), f64_mul(f, SOFTFLOAT_INIT64(0.33333333333333333))));
		if (k == 0) {
			return f64_sub(f, R);
		} else {
			dk = i32_to_f64(k);
			return f64_sub(f64_mul(dk, ln2_hi), f64_sub(f64_sub(R, f64_mul(dk, ln2_lo)), f));
		}
	}

	s = f64_div(f, f64_add(SOFTFLOAT_INIT64(2.0), f));
	dk = i32_to_f64(k);
	z = f64_mul(s, s);
	i = hx - 0x6147a;
	w = f64_mul(z, z);
	j = 0x6b851 - hx;

	t1 = f64_mul(w, f64_add(Lg2, f64_mul(w, f64_add(Lg4, f64_mul(w, Lg6)))));
	t2 = f64_mul(z, f64_add(Lg1, f64_mul(w, f64_add(Lg3, f64_mul(w, f64_add(Lg5, f64_mul(w, Lg7)))))));

	i |= j;
	R = f64_add(t2, t1);

	if (i > 0) {
		hfsq = f64_mul(f64_mul(SOFTFLOAT_INIT64(0.5), f), f);
		if(k == 0)
			return f64_sub(f, f64_sub(hfsq, f64_mul(s, f64_add(hfsq, R))));
		else
			return f64_sub(f64_mul(dk, ln2_hi), f64_sub(f64_sub(hfsq, f64_add(f64_mul(s, f64_add(hfsq, R)), f64_mul(dk, ln2_lo))), f));
	} else {
		if(k == 0)
			return f64_sub(f, f64_mul(s, f64_sub(f, R)));
		else
			return f64_sub(f64_mul(dk, ln2_hi), f64_sub(f64_sub(f64_mul(s, f64_sub(f, R)), f64_mul(dk, ln2_lo)), f));
	}
}

float64_t log10_soft(float64_t x)
{
	float64_t two54      = SOFTFLOAT_INIT64(1.80143985094819840000e+16); /* 0x43500000, 0x00000000 */
	float64_t ivln10     = SOFTFLOAT_INIT64(4.34294481903251816668e-01); /* 0x3FDBCB7B, 0x1526E50E */
	float64_t log10_2hi  = SOFTFLOAT_INIT64(3.01029995663611771306e-01); /* 0x3FD34413, 0x509F6000 */
	float64_t log10_2lo  = SOFTFLOAT_INIT64(3.69423907715893078616e-13); /* 0x3D59FEF3, 0x11F12B36 */
	float64_t y, z;
	float64_t zero = i32_to_f64(0);
	int i, k, hx;
	unsigned lx;

	hx = __HI(x);
	lx = __LO(x);

    k = 0;

	if (hx < 0x00100000) {
		if (((hx & 0x7fffffff) | lx) == 0)
			return f64_div(f64_sub(SOFTFLOAT_INIT64(0), two54), zero);
		if (hx<0)
			return f64_div(f64_sub(x, x), zero);
		k -= 54;
		x = f64_mul(x, two54);
		hx = __HI(x);
	}

	if (hx >= 0x7ff00000)
		return f64_add(x, x);

	k += (hx >> 20) - 1023;
	i  = ((unsigned)k & 0x80000000) >> 31;
    hx = (hx & 0x000fffff) | ((0x3ff - i) << 20);
    y  = i32_to_f64(k + i);
    __HI(x) = hx;

	z = f64_add(f64_mul(y, log10_2lo), f64_mul(ivln10, log_soft(x)));

	return f64_add(z, f64_mul(y, log10_2hi));
}
