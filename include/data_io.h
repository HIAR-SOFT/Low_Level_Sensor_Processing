#ifndef DATA_IO_H
#define DATA_IO_H

#include <stddef.h>

/* Loads little-endian float64 samples. max_samples == 0 means "all".
 * Returns a malloc'd buffer (caller frees) or NULL on error. */
double *sp_load_f64(const char *path, size_t max_samples, size_t *n_out);

/* Returns 0 on success, -1 on failure. */
int sp_save_f64(const char *path, const double *x, size_t n);

#endif