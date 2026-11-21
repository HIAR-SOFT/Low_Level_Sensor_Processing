#include <math.h>
#include "sensor_processing.h"

int sp_classify_rms(double rms, double warn, double crit, sp_health_t *out)
{
    if (!out) return SP_ERR_NULL;
    if (isnan(rms) || isnan(warn) || isnan(crit)) return SP_ERR_PARAM;
    if (rms < 0.0 || warn < 0.0 || crit < warn) return SP_ERR_PARAM;

    if (rms > crit)      *out = SP_HEALTH_CRITICAL;
    else if (rms > warn) *out = SP_HEALTH_WARNING;
    else                 *out = SP_HEALTH_NORMAL;
    return SP_OK;
}

const char *sp_health_name(sp_health_t h)
{
    switch (h) {
    case SP_HEALTH_NORMAL:   return "NORMAL";
    case SP_HEALTH_WARNING:  return "WARNING";
    case SP_HEALTH_CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}