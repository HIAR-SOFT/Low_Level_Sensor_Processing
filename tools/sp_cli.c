#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sp_dispatch.h"
#include "data_io.h"

static void usage(void)
{
    fprintf(stderr,
        "usage: sp_cli <c|asm> <command> <data.bin> [args]\n"
        "  rms    [warn crit]\n"
        "  stats\n"
        "  peaks  [threshold]\n"
        "  movavg <window> <out.bin>\n");
}

int main(int argc, char **argv)
{
    if (argc < 4) { usage(); return 1; }

    const sp_impl_t *impl = NULL;
    if (!strcmp(argv[1], "c")) impl = &SP_IMPL_C;
    else if (!strcmp(argv[1], "asm")) impl = &SP_IMPL_ASM;
    else { usage(); return 1; }

    const char *cmd = argv[2];
    size_t n = 0;
    double *x = sp_load_f64(argv[3], 0, &n);
    if (!x) { fprintf(stderr, "cannot read %s\n", argv[3]); return 1; }

    int rc = 0;
    if (!strcmp(cmd, "rms")) {
        double warn = argc > 4 ? atof(argv[4]) : SP_DEFAULT_RMS_WARN;
        double crit = argc > 5 ? atof(argv[5]) : SP_DEFAULT_RMS_CRIT;
        double r = 0.0;
        sp_health_t h;
        rc = impl->rms(x, n, &r);
        if (rc == SP_OK) rc = sp_classify_rms(r, warn, crit, &h);
        if (rc == SP_OK) printf("rms=%.17g\nstatus=%s\n", r, sp_health_name(h));
    } else if (!strcmp(cmd, "stats")) {
        sp_stats_t s;
        rc = impl->stats(x, n, &s);
        if (rc == SP_OK)
            printf("min=%.17g\nmax=%.17g\nmean=%.17g\nstddev=%.17g\n",
                   s.min, s.max, s.mean, s.stddev);
    } else if (!strcmp(cmd, "peaks")) {
        double thr = argc > 4 ? atof(argv[4]) : 2.5;
        size_t *idx = malloc(n ? n * sizeof(size_t) : sizeof(size_t));
        size_t count = 0;
        if (!idx) { free(x); return 1; }
        rc = impl->peak_detect(x, n, thr, idx, n, &count);
        if (rc == SP_OK) {
            printf("count=%zu\nindices=", count);
            for (size_t i = 0; i < count; i++) printf(i ? " %zu" : "%zu", idx[i]);
            printf("\n");
        }
        free(idx);
    } else if (!strcmp(cmd, "movavg")) {
        if (argc < 6) { usage(); free(x); return 1; }
        size_t w = (size_t)strtoull(argv[4], NULL, 10);
        double *out = malloc(n ? n * sizeof(double) : sizeof(double));
        if (!out) { free(x); return 1; }
        rc = impl->moving_average(x, n, w, out);
        if (rc == SP_OK) {
            if (sp_save_f64(argv[5], out, n - w + 1) != 0) rc = 1;
            else printf("samples=%zu\n", n - w + 1);
        }
        free(out);
    } else {
        usage();
        free(x);
        return 1;
    }

    free(x);
    if (rc != SP_OK) { fprintf(stderr, "error code %d\n", rc); return 2; }
    return 0;
}