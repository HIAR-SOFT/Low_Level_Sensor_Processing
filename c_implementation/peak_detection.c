#include <math.h>
#include "sensor_processing.h"

int sp_peak_detect_c(const double *x, size_t n, double threshold,
                     size_t *indices, size_t max_indices, size_t *count_out)
{
    if (!x || !count_out) return SP_ERR_NULL;
    if (n == 0) return SP_ERR_EMPTY;
    if (isnan(threshold) || threshold < 0.0) return SP_ERR_PARAM;
    if (!indices) max_indices = 0;

    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        if (fabs(x[i]) > threshold) { /* false for NaN samples */
            if (count < max_indices) indices[count] = i;
            count++;
        }
    }
    *count_out = count;
    return SP_OK;
}