/*
 * pacing_controller.c — Execution Pacing Controller
 *
 * TCP'nin congestion window'u gibi: instability arttığında daha fazla
 * uyur, kararlı durumda yavaşça gevşer.
 *
 * Mekanizmalar:
 *  1. Akümülatör: her instability sinyalinde artar
 *  2. Damping: kararlı durumda üstel azalma (exponential decay)
 *  3. Hard cap: ZN_PACE_MAX_NS aşılamaz
 *  4. Termal uyumu: throttle varsa ekstra uyku
 *  5. EINTR güvenli nanosleep: sinyal kesintisinde devam eder
 *
 * Tasarım notu:
 *  - Uyku ölçüm DIŞINDA uygulanır (zn_stabilizer_end sonrası)
 *  - Bu sayede measured latency pacing'den kirlenmez
 *  - Gerçek throughput kontrolü: iş başı frekansı düşürülür
 */

#define _POSIX_C_SOURCE 200809L
#include "performance_stabilizer.h"

#include <math.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/* =========================================================================
 * Platform uyku
 * ========================================================================= */

void zn_pacing_sleep_ns(uint64_t ns)
{
    if (ns == 0) return;
    struct timespec req = {
        .tv_sec  = (time_t)(ns / 1000000000ull),
        .tv_nsec = (long)  (ns % 1000000000ull)
    };
    /* EINTR'den sonra kalan süreyi tekrar dene */
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {}
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void zn_pacing_init(ZnPacingController *p)
{
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->damp = ZN_PACE_DAMP;
}

void zn_pacing_update(ZnPacingController *p,
                      double observed_ms,
                      double target_ms,
                      int instability_mask)
{
    if (!p) return;

    /* --- Saldırgan artış: instability sinyaline göre --- */

    if (instability_mask & ZN_INSTABILITY_SPIKE) {
        /* Spike: hızlı, büyük artış */
        p->accum_ns = fmin(p->accum_ns + 200000.0, (double)ZN_PACE_MAX_NS);
    }

    if (instability_mask & ZN_INSTABILITY_TAIL) {
        /* p99 tavan: orta artış */
        p->accum_ns = fmin(p->accum_ns + 100000.0, (double)ZN_PACE_MAX_NS);
    }

    if (instability_mask & ZN_INSTABILITY_VARIANCE) {
        /* Varyans patlaması: büyük artış */
        p->accum_ns = fmin(p->accum_ns + 300000.0, (double)ZN_PACE_MAX_NS);
    }

    if (instability_mask & ZN_INSTABILITY_THERMAL) {
        /* Termal: maksimuma yakın — CPU'ya nefes aldır */
        p->accum_ns = fmin(p->accum_ns + 500000.0, (double)ZN_PACE_MAX_NS);
    }

    if (instability_mask & ZN_INSTABILITY_DRIFT) {
        /* Drift: latency yükseliyorsa pacing ekle */
        if (observed_ms > target_ms) {
            double err_ms  = observed_ms - target_ms;
            double add_ns  = err_ms * 1e6 * 0.5;  /* hata'nın yarısı kadar uyku */
            p->accum_ns = fmin(p->accum_ns + add_ns, (double)ZN_PACE_MAX_NS);
        }
    }

    /* --- Ortalama yüksekse ek pacing (envelope kontrolü için) --- */
    if (instability_mask == ZN_INSTABILITY_NONE && observed_ms > target_ms * 1.05) {
        double err_ns = (observed_ms - target_ms * 1.05) * 1e6 * 0.3;
        p->accum_ns = fmin(p->accum_ns + err_ns, (double)ZN_PACE_MAX_NS);
    }
}

void zn_pacing_relax(ZnPacingController *p)
{
    if (!p) return;
    /* Kararlı durumda üstel azalma */
    p->accum_ns *= p->damp;
    if (p->accum_ns < 500.0) p->accum_ns = 0.0;  /* sıfıra round et */
}

void zn_pacing_apply(ZnPacingController *p)
{
    if (!p || p->accum_ns <= 0.0) return;

    uint64_t sleep_ns = (uint64_t)p->accum_ns;
    if (sleep_ns > ZN_PACE_MAX_NS) sleep_ns = ZN_PACE_MAX_NS;

    zn_pacing_sleep_ns(sleep_ns);

    p->total_slept_ns += sleep_ns;
    p->sleep_count++;
}
