#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>
#include "sp_dispatch.h"
#include "data_io.h"

typedef struct {
    const double *x;
    size_t n, window;
    double threshold;
    double *out;      /* n doubles   */
    size_t *idx;      /* n indices   */
} ctx_t;

/* Written after every call so the compiler cannot discard the computation. */
static volatile double g_sink;

static void die(const char *msg) { fprintf(stderr, "benchmark error: %s\n", msg); exit(1); }

static void call_mavg(const sp_impl_t *I, ctx_t *c)
{
    if (I->moving_average(c->x, c->n, c->window, c->out) != SP_OK) die("moving_average");
    g_sink += c->out[c->n - c->window];
}
static void call_rms(const sp_impl_t *I, ctx_t *c)
{
    double r;
    if (I->rms(c->x, c->n, &r) != SP_OK) die("rms");
    g_sink += r;
}
static void call_peaks(const sp_impl_t *I, ctx_t *c)
{
    size_t cnt;
    if (I->peak_detect(c->x, c->n, c->threshold, c->idx, c->n, &cnt) != SP_OK) die("peaks");
    g_sink += (double)cnt;
}
static void call_stats(const sp_impl_t *I, ctx_t *c)
{
    sp_stats_t s;
    if (I->stats(c->x, c->n, &s) != SP_OK) die("stats");
    g_sink += s.stddev + s.min + s.max + s.mean;
}

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static inline uint64_t tsc_now(void)
{
    _mm_lfence();
    uint64_t t = __rdtsc();
    _mm_lfence();
    return t;
}

static long vmhwm_kb(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[256];
    long kb = -1;
    if (!f) return -1;
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "VmHWM: %ld kB", &kb) == 1) break;
    fclose(f);
    return kb;
}

int main(int argc, char **argv)
{
    const char *impl_name = "c", *label = "c", *data = NULL;
    size_t n = 0, window = 32;
    int runs = 30;
    double threshold = 2.5;

    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--impl")) impl_name = argv[i + 1];
        else if (!strcmp(argv[i], "--label")) label = argv[i + 1];
        else if (!strcmp(argv[i], "--data")) data = argv[i + 1];
        else if (!strcmp(argv[i], "--n")) n = (size_t)strtoull(argv[i + 1], NULL, 10);
        else if (!strcmp(argv[i], "--runs")) runs = atoi(argv[i + 1]);
        else if (!strcmp(argv[i], "--window")) window = (size_t)strtoull(argv[i + 1], NULL, 10);
        else if (!strcmp(argv[i], "--threshold")) threshold = atof(argv[i + 1]);
        else die("unknown option");
    }
    if (!data || n == 0) die("usage: --impl c|asm --label L --data file --n N [--runs R]");

    const sp_impl_t *I = !strcmp(impl_name, "asm") ? &SP_IMPL_ASM : &SP_IMPL_C;

    size_t got = 0;
    double *x = sp_load_f64(data, n, &got);      /* untimed: file I/O */
    if (!x || got != n) die("cannot load requested samples");
    ctx_t c = {x, n, window, threshold, malloc(n * sizeof(double)),
               malloc(n * sizeof(size_t))};
    if (!c.out || !c.idx) die("out of memory");

    struct { const char *name; void (*call)(const sp_impl_t *, ctx_t *); } algos[] = {
        {"moving_average", call_mavg}, {"rms", call_rms},
        {"peak_detection", call_peaks}, {"statistics", call_stats}};

    size_t iters = 2000000 / n;                  /* keep each timed sample >= ~ms */
    if (iters < 1) iters = 1;

    for (size_t a = 0; a < sizeof algos / sizeof algos[0]; a++) {
        for (int w = 0; w < 3; w++) algos[a].call(I, &c);       /* warm-up */
        for (int run = 0; run < runs; run++) {
            uint64_t t0 = now_ns(), c0 = tsc_now();
            for (size_t k = 0; k < iters; k++) algos[a].call(I, &c);
            uint64_t c1 = tsc_now(), t1 = now_ns();
            printf("%s,%s,%zu,%d,%.3f,%.1f,%zu\n", label, algos[a].name, n, run,
                   (double)(t1 - t0) / (double)iters,
                   (double)(c1 - c0) / (double)iters, iters);
        }
    }
    fprintf(stderr, "%s,%zu,%ld\n", label, n, vmhwm_kb());
    free(x); free(c.out); free(c.idx);
    return 0;
}