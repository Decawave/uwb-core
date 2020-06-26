#include <stdlib.h>
#include <string.h>
#include <softfloat/softfloat.h>
#include <softfloat/softfloat_types.h>

static long long int _strtoll(const char *s, char **endptr, unsigned int base)
{
#if __KERNEL__
    long long int v;
    int rc = kstrtoll(s, base, &v);
    if (rc){
        return 0;
    }
    if (endptr) {
        const char *p = s;
        while (*p == '-' || *p == ' ') p++;
        while (*p >= '0' && *p <= '9') p++;
        *endptr = (char*)p;
    }
    return v;
#else

    /* For testing the above kernel equivalent version only */
#if 0
    long long int v;
    v = strtoll(s, NULL, base);
    if (endptr) {
        const char *p = s;
        while (*p == '-' || *p == ' ') p++;
        while (*p >= '0' && *p <= '9') p++;
        *endptr = (char*)p;
    }
    return v;
#endif
    return strtoll(s, endptr, base);
#endif
}

float64_t strtod_soft( const char *nptr, char **endptr )
{
    int i, n;
    char *period_at;
    char *dash_at;
    char *exponent_at;
    char *ep = 0;
    int64_t ival, frac, div, exp;
    float64_t f, f_frac;

    period_at = strchr(nptr, '.');
    dash_at = strchr(nptr, '-');
    exponent_at = strchr(nptr, 'e');
    ival = _strtoll(nptr, &ep, 10);
    f = i64_to_f64(ival);

    if (period_at == 0 || *(period_at+1) == 0) {
        /* No period, or period at the very end, treat as integer */
        if (endptr) *endptr = ep;
        return f;
    }
    frac = _strtoll(period_at+1, &ep, 10);

    /* Fractional part */
    div = 1;
    if (endptr == 0) n = strlen(period_at + 1);
    else n = *endptr - period_at + 1;
    for (i = 0; i < n; i++) div *= 10;
    f_frac = f64_div(i64_to_f64(frac), i64_to_f64(div));

    if (ival < 0 || (dash_at != 0 && dash_at < period_at)) {
        f = f64_sub(f, f_frac);
    } else {
        f = f64_add(f, f_frac);
    }

    /* Exponent */
    if (endptr) *endptr = ep;
    if (exponent_at == 0 || *(exponent_at+1) == 0|| ep == 0 || exponent_at != ep) {
        return f;
    }

    n = _strtoll(exponent_at+1, &ep, 10);
    if (n < 0) {
        div = 1;
        for (i = 0; i < n; i++) div *= 10;
        f = f64_div(f, i64_to_f64(div));
    } else {
        exp = 1;
        for (i = 0; i < n; i++) exp *= 10;
        f = f64_mul(f, i64_to_f64(exp));
    }
    return f;
}
