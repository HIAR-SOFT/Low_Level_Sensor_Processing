#ifndef SP_DISPATCH_H
#define SP_DISPATCH_H

#include "sensor_processing.h"
#include "sensor_assembly.h"

typedef struct {
    const char *name;
    int (*moving_average)(const double *, size_t, size_t, double *);
    int (*rms)(const double *, size_t, double *);
    int (*peak_detect)(const double *, size_t, double, size_t *, size_t, size_t *);
    int (*stats)(const double *, size_t, sp_stats_t *);
} sp_impl_t;

#define SP_UNUSED __attribute__((unused))

static const sp_impl_t SP_IMPL_C SP_UNUSED = {
    "c", sp_moving_average_c, sp_rms_c, sp_peak_detect_c, sp_stats_c};

static const sp_impl_t SP_IMPL_ASM SP_UNUSED = {
    "asm", sp_moving_average_asm, sp_rms_asm, sp_peak_detect_asm, sp_stats_asm};

#endif