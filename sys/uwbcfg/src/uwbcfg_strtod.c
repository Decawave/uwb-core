#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifndef __KERNEL__

double strtod_soft( const char *nptr, char **endptr )
{
    int i, n;
    char *period_at;
    char *dash_at;
    char *exponent_at;
    char *ep = 0;
    int64_t ival, frac, div, exp;
    double f, f_frac;

    period_at = strchr(nptr, '.');
    dash_at = strchr(nptr, '-');
    exponent_at = strchr(nptr, 'e');
    ival = strtoll(nptr, &ep, 10);
    f = (double)(ival);

    if (period_at == 0 || *(period_at+1) == 0) {
        /* No period, or period at the very end, treat as integer */
        if (endptr) *endptr = ep;
        return f;
    }
    frac = strtoll(period_at+1, &ep, 10);

    /* Fractional part */
    div = 1;
    if (endptr == 0) n = strlen(period_at + 1);
    else n = *endptr - period_at + 1;
    for (i = 0; i < n; i++) div *= 10;
    f_frac = (double)frac / (double)div;

    if (ival < 0 || (dash_at != 0 && dash_at < period_at)) {
        f -= f_frac;
    } else {
        f += f_frac;
    }

    /* Exponent */
    if (endptr) *endptr = ep;
    if (exponent_at == 0 || *(exponent_at+1) == 0|| ep == 0 || exponent_at != ep) {
        return f;
    }

    n = strtoll(exponent_at+1, &ep, 10);
    if (n < 0) {
        div = 1;
        for (i = 0; i < n; i++) div *= 10;
        f /= (double)div;
    } else {
        exp = 1;
        for (i = 0; i < n; i++) exp *= 10;
        f *= (double)exp;
    }
    return f;
}
#endif
