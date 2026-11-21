#include <math.h>
#include "sensor_processing.h"

/* Two-pass: pass 1 = sum/min/max, pass 2 = squared deviations.
 * min/max are written as (a < b ? a : b) so the compiled code matches the
 * semantics of the MINSD/MAXSD instructions used in the Assembly version. */
int sp_stats_c(const double *x, size_t n, sp_stats_t *out)
{
    if (!x || !out) return SP_ERR_NULL;
    if (n == 0) return SP_ERR_EMPTY;

    double sum = 0.0, mn = x[0], mx = x[0];
    for (size_t i = 0; i < n; i++) {
        const double v = x[i];
        sum += v;
        mn = (mn < v) ? mn : v;
        mx = (mx > v) ? mx : v;
    }
    const double dn = (double)n;
    const double mean = sum / dn;

    double ss = 0.0;
    for (size_t i = 0; i < n; i++) {
        const double d = x[i] - mean;
        ss += d * d;
    }

    out->min = mn;
    out->max = mx;
    out->mean = mean;
    out->stddev = sqrt(ss / dn);
    return SP_OK;
}