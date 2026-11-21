#include <math.h>
#include "sensor_processing.h"

/* RMS = sqrt( (x1^2 + ... + xn^2) / n ), summed in index order. */
int sp_rms_c(const double *x, size_t n, double *out)
{
    if (!x || !out) return SP_ERR_NULL;
    if (n == 0) return SP_ERR_EMPTY;

    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += x[i] * x[i];

    *out = sqrt(sum / (double)n);
    return SP_OK;
}