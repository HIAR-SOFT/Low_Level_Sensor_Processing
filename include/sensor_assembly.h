#ifndef SENSOR_ASSEMBLY_H
#define SENSOR_ASSEMBLY_H

#include <stddef.h>
#include "sensor_processing.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Implemented in the assembly directory (NASM, System V AMD64 ABI). Same contracts
 * as the *_c functions in sensor_processing.h. */
int sp_moving_average_asm(const double *x, size_t n, size_t window, double *out);
int sp_rms_asm(const double *x, size_t n, double *out);
int sp_peak_detect_asm(const double *x, size_t n, double threshold,
                       size_t *indices, size_t max_indices, size_t *count_out);
int sp_stats_asm(const double *x, size_t n, sp_stats_t *out);

#ifdef __cplusplus
}
#endif
#endif