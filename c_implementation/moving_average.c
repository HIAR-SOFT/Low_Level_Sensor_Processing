#include "sensor_processing.h"

/* out[i] = mean(x[i .. i+window-1]), i = 0 .. n-window.
 * First window is summed directly; afterwards the sum slides:
 *   sum += x[i+window-1] - x[i-1]
 * O(n) instead of O(n*window). Rounding drift is negligible for our data
 * and identical in the Assembly version, which uses the same operations. */
int sp_moving_average_c(const double *x, size_t n, size_t window, double *out)
{
    if (!x || !out) return SP_ERR_NULL;
    if (n == 0) return SP_ERR_EMPTY;
    if (window == 0 || window > n) return SP_ERR_PARAM;

    const double w = (double)window;
    double sum = 0.0;
    for (size_t j = 0; j < window; j++) sum += x[j];
    out[0] = sum / w;

    const size_t last = n - window;
    for (size_t i = 1; i <= last; i++) {
        sum += x[i + window - 1] - x[i - 1];
        out[i] = sum / w;
    }
    return SP_OK;
}