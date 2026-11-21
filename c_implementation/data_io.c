#include <stdio.h>
#include <stdlib.h>
#include "data_io.h"

double *sp_load_f64(const char *path, size_t max_samples, size_t *n_out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long bytes = ftell(f);
    if (bytes < 0) { fclose(f); return NULL; }
    rewind(f);

    size_t avail = (size_t)bytes / sizeof(double);
    size_t n = (max_samples != 0 && max_samples < avail) ? max_samples : avail;

    double *buf = malloc(n ? n * sizeof(double) : sizeof(double));
    if (!buf) { fclose(f); return NULL; }
    size_t got = n ? fread(buf, sizeof(double), n, f) : 0;
    fclose(f);
    if (got != n) { free(buf); return NULL; }

    *n_out = n;
    return buf;
}

int sp_save_f64(const char *path, const double *x, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t put = n ? fwrite(x, sizeof(double), n, f) : 0;
    int rc = (fclose(f) == 0 && put == n) ? 0 : -1;
    return rc;
}