#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sp_dispatch.h"

static int g_fail = 0, g_checks = 0;
static const char *g_name = "";

#define CHECK(cond) do { g_checks++; if (!(cond)) { g_fail++; \
    fprintf(stderr, "FAIL %s:%d [%s] %s\n", __FILE__, __LINE__, g_name, #cond); } } while (0)

static int close_to(double a, double b, double rtol)
{
    return fabs(a - b) <= rtol * fmax(1.0, fabs(b));
}

static void test_known_values(const sp_impl_t *I)
{
    double r;
    double a[] = {3, -3, 3, -3};
    CHECK(I->rms(a, 4, &r) == SP_OK && close_to(r, 3.0, 1e-15));
    double ones[] = {1, 1, 1, 1};
    CHECK(I->rms(ones, 4, &r) == SP_OK && close_to(r, 1.0, 1e-15));

    double s[] = {1, 2, 3, 4, 5}, out[5];
    CHECK(I->moving_average(s, 5, 2, out) == SP_OK);
    CHECK(close_to(out[0], 1.5, 1e-12) && close_to(out[1], 2.5, 1e-12) &&
          close_to(out[2], 3.5, 1e-12) && close_to(out[3], 4.5, 1e-12));
    CHECK(I->moving_average(s, 5, 5, out) == SP_OK && close_to(out[0], 3.0, 1e-12));
    CHECK(I->moving_average(s, 5, 1, out) == SP_OK);
    for (int i = 0; i < 5; i++) CHECK(close_to(out[i], s[i], 1e-12));

    double p[] = {0, 5, -6, 1, 7, 0.5};
    size_t idx[8], cnt = 99;
    CHECK(I->peak_detect(p, 6, 4.9, idx, 8, &cnt) == SP_OK);
    CHECK(cnt == 3 && idx[0] == 1 && idx[1] == 2 && idx[2] == 4);
    CHECK(I->peak_detect(p, 6, 4.9, idx, 2, &cnt) == SP_OK);     /* truncated store */
    CHECK(cnt == 3 && idx[0] == 1 && idx[1] == 2);
    CHECK(I->peak_detect(p, 6, 4.9, NULL, 8, &cnt) == SP_OK && cnt == 3);
    CHECK(I->peak_detect(p, 6, 5.0, idx, 8, &cnt) == SP_OK && cnt == 2); /* strict > */
    double nanv[] = {NAN, 10};
    CHECK(I->peak_detect(nanv, 2, 5.0, idx, 8, &cnt) == SP_OK && cnt == 1 && idx[0] == 1);

    double d[] = {2, 4, 4, 4, 5, 5, 7, 9};
    sp_stats_t st;
    CHECK(I->stats(d, 8, &st) == SP_OK);
    CHECK(st.min == 2.0 && st.max == 9.0 && close_to(st.mean, 5.0, 1e-15) &&
          close_to(st.stddev, 2.0, 1e-15));
}

static void test_errors(const sp_impl_t *I)
{
    double x[4] = {1, 2, 3, 4}, out[4], r;
    size_t idx[4], cnt;
    sp_stats_t st;

    CHECK(I->rms(NULL, 4, &r) == SP_ERR_NULL);
    CHECK(I->rms(x, 4, NULL) == SP_ERR_NULL);
    CHECK(I->rms(x, 0, &r) == SP_ERR_EMPTY);
    CHECK(I->rms(NULL, 0, &r) == SP_ERR_NULL);               /* NULL checked first */

    CHECK(I->moving_average(NULL, 4, 2, out) == SP_ERR_NULL);
    CHECK(I->moving_average(x, 4, 2, NULL) == SP_ERR_NULL);
    CHECK(I->moving_average(x, 0, 2, out) == SP_ERR_EMPTY);
    CHECK(I->moving_average(x, 4, 0, out) == SP_ERR_PARAM);
    CHECK(I->moving_average(x, 4, 5, out) == SP_ERR_PARAM);

    CHECK(I->peak_detect(NULL, 4, 1.0, idx, 4, &cnt) == SP_ERR_NULL);
    CHECK(I->peak_detect(x, 4, 1.0, idx, 4, NULL) == SP_ERR_NULL);
    CHECK(I->peak_detect(x, 0, 1.0, idx, 4, &cnt) == SP_ERR_EMPTY);
    CHECK(I->peak_detect(x, 4, -1.0, idx, 4, &cnt) == SP_ERR_PARAM);
    CHECK(I->peak_detect(x, 4, NAN, idx, 4, &cnt) == SP_ERR_PARAM);

    CHECK(I->stats(NULL, 4, &st) == SP_ERR_NULL);
    CHECK(I->stats(x, 4, NULL) == SP_ERR_NULL);
    CHECK(I->stats(x, 0, &st) == SP_ERR_EMPTY);
}

static void test_boundaries(const sp_impl_t *I)
{
    double one[1] = {-4.0}, r, out[1];
    sp_stats_t st;
    CHECK(I->rms(one, 1, &r) == SP_OK && close_to(r, 4.0, 1e-15));
    CHECK(I->moving_average(one, 1, 1, out) == SP_OK && close_to(out[0], -4.0, 1e-15));
    CHECK(I->stats(one, 1, &st) == SP_OK && st.min == -4.0 && st.max == -4.0 &&
          st.stddev == 0.0);
    size_t cnt = 7;
    CHECK(I->peak_detect(one, 1, 0.0, NULL, 0, &cnt) == SP_OK && cnt == 1);
}

/* ---- large-data equivalence between the C and Assembly implementations ---- */
static double *make_data(size_t n)
{
    double *x = malloc(n * sizeof(double));
    if (!x) return NULL;
    uint64_t s = 0x9E3779B97F4A7C15ull;
    for (size_t i = 0; i < n; i++) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;                 /* xorshift64 */
        x[i] = ((double)(s >> 11) / 9007199254740992.0) * 6.0 - 3.0;  /* [-3, 3) */
    }
    return x;
}

static void test_equivalence_large(void)
{
    const size_t n = 2000003, w = 64;
    g_name = "c-vs-asm";
    double *x = make_data(n);
    double *oc = malloc(n * sizeof(double)), *oa = malloc(n * sizeof(double));
    size_t *ic = malloc(n * sizeof(size_t)), *ia = malloc(n * sizeof(size_t));
    CHECK(x && oc && oa && ic && ia);
    if (!(x && oc && oa && ic && ia)) goto done;

    double rc_, ra;
    CHECK(sp_rms_c(x, n, &rc_) == SP_OK && sp_rms_asm(x, n, &ra) == SP_OK);
    CHECK(close_to(rc_, ra, 1e-12));

    sp_stats_t sc, sa;
    CHECK(sp_stats_c(x, n, &sc) == SP_OK && sp_stats_asm(x, n, &sa) == SP_OK);
    CHECK(sc.min == sa.min && sc.max == sa.max);
    CHECK(close_to(sc.mean, sa.mean, 1e-12) && close_to(sc.stddev, sa.stddev, 1e-12));

    CHECK(sp_moving_average_c(x, n, w, oc) == SP_OK);
    CHECK(sp_moving_average_asm(x, n, w, oa) == SP_OK);
    size_t bad = 0;
    for (size_t i = 0; i < n - w + 1; i++) if (!close_to(oc[i], oa[i], 1e-12)) bad++;
    CHECK(bad == 0);

    size_t cc, ca;
    CHECK(sp_peak_detect_c(x, n, 2.9, ic, n, &cc) == SP_OK);
    CHECK(sp_peak_detect_asm(x, n, 2.9, ia, n, &ca) == SP_OK);
    CHECK(cc == ca);
    bad = 0;
    for (size_t i = 0; i < cc && i < ca; i++) if (ic[i] != ia[i]) bad++;
    CHECK(bad == 0);
done:
    free(x); free(oc); free(oa); free(ic); free(ia);
}

int main(void)
{
    const sp_impl_t *impls[] = {&SP_IMPL_C, &SP_IMPL_ASM};
    for (int k = 0; k < 2; k++) {
        g_name = impls[k]->name;
        test_known_values(impls[k]);
        test_errors(impls[k]);
        test_boundaries(impls[k]);
    }
    test_equivalence_large();

    if (g_fail) { fprintf(stderr, "%d of %d checks FAILED\n", g_fail, g_checks); return 1; }
    printf("ALL TESTS PASSED (%d checks)\n", g_checks);
    return 0;
}