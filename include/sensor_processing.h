#ifndef SENSOR_PROCESSING_H
#define SENSOR_PROCESSING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Status codes (shared by the C and Assembly implementations). */
enum { SP_OK = 0, SP_ERR_NULL = -1, SP_ERR_EMPTY = -2, SP_ERR_PARAM = -3 };

typedef enum { SP_HEALTH_NORMAL = 0, SP_HEALTH_WARNING = 1, SP_HEALTH_CRITICAL = 2 } sp_health_t;

/* Layout is relied upon by the Assembly code (offsets 0, 8, 16, 24). */
typedef struct {
    double min;
    double max;
    double mean;
    double stddev; /* population standard deviation */
} sp_stats_t;

_Static_assert(sizeof(sp_stats_t) == 32, "sp_stats_t must be 32 bytes");
_Static_assert(offsetof(sp_stats_t, stddev) == 24, "sp_stats_t layout changed");

/* Simulated thresholds in the same units as the data.
 * NOT validated industrial safety limits. */
#define SP_DEFAULT_RMS_WARN 1.0
#define SP_DEFAULT_RMS_CRIT 2.0

/* out has n - window + 1 elements. Requires 1 <= window <= n. */
int sp_moving_average_c(const double *x, size_t n, size_t window, double *out);

int sp_rms_c(const double *x, size_t n, double *out);

/* Counts samples with |x[i]| > threshold. Stores the first max_indices
 * indices; *count_out is the TOTAL number found (may exceed max_indices).
 * indices may be NULL (treated as max_indices = 0). threshold must be >= 0. */
int sp_peak_detect_c(const double *x, size_t n, double threshold,
                     size_t *indices, size_t max_indices, size_t *count_out);

int sp_stats_c(const double *x, size_t n, sp_stats_t *out);

/* Monitoring: rms > crit -> CRITICAL, rms > warn -> WARNING, else NORMAL. */
int sp_classify_rms(double rms, double warn, double crit, sp_health_t *out);
const char *sp_health_name(sp_health_t h);

#ifdef __cplusplus
}
#endif
#endif
