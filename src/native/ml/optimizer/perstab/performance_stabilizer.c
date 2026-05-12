/*
 * performance_stabilizer.c — Nova Deterministic Compute Controller
 *
 * Ana kontrol döngüsü:
 *
 *   measure → analyze → detect → adapt → clamp → pace → repeat
 *
 * Katman mimarisi:
 *
 *   ZnStabilizer
 *       ├── ZnVarianceAnalyzer   (variance_analyzer.c)
 *       ├── ZnThermalState       (thermal_detector.c)
 *       ├── ZnPacingController   (pacing_controller.c)
 *       ├── ZnExecutionProfile   (profil + bounds)
 *       └── ZnEnvelope           (deterministik bant)
 *
 * TCP/congestion analojisi:
 *   - ema_fast/slow çifti → RTT smooth
 *   - pacing_controller  → congestion window
 *   - profile.threads    → MSS / parallelism
 *   - envelope           → SLA guarantee
 */

#define _POSIX_C_SOURCE 200809L
#include "performance_stabilizer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#if defined(__APPLE__)
#  include <mach/mach_time.h>
#endif

/* =========================================================================
 * Platform timing
 * ========================================================================= */

uint64_t zn_time_now_ns(void)
{
#if defined(__APPLE__)
    static mach_timebase_info_data_t tb;
    static int init = 0;
    if (!init) { mach_timebase_info(&tb); init = 1; }
    uint64_t t = mach_absolute_time();
    return (t * (uint64_t)tb.numer) / (uint64_t)tb.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

double zn_time_elapsed_ms(uint64_t start_ns, uint64_t end_ns)
{
    if (end_ns < start_ns) return 0.0;
    return (double)(end_ns - start_ns) / 1e6;
}

/* =========================================================================
 * Envelope
 * ========================================================================= */

void zn_envelope_init(ZnEnvelope *e,
                      double target_ms,
                      double envelope_pct,
                      double max_p99_ms)
{
    if (!e) return;
    e->target_ms    = target_ms;
    e->envelope_pct = envelope_pct;
    e->max_p99_ms   = max_p99_ms;
    e->min_ms       = target_ms * (1.0 - envelope_pct);
    e->max_ms       = target_ms * (1.0 + envelope_pct);
    if (e->min_ms < 0.0) e->min_ms = 0.0;
}

int zn_within_envelope(const ZnEnvelope *e, const ZnLatencyStats *s)
{
    if (!e || !s) return 1;
    if (s->mean < e->min_ms || s->mean > e->max_ms) return 0;
    if (s->p99 > e->max_p99_ms && s->n >= 8)        return 0;
    return 1;
}

/* =========================================================================
 * Execution Profile
 * ========================================================================= */

void zn_profile_init(ZnExecutionProfile *p, const ZnProfileBounds *b)
{
    if (!p || !b) return;
    /* Başlangıç: maksimumda başla, stabilizer kısaltır */
    p->threads          = b->max_threads;
    p->batch_size       = b->max_batch;
    p->tile_size        = (b->min_tile + b->max_tile) / 2;
    p->fusion_threshold = b->min_fusion;
}

void zn_profile_autotune(ZnExecutionProfile *p,
                         const ZnProfileBounds *b,
                         const ZnLatencyStats *s,
                         double target_ms,
                         int instability_mask)
{
    if (!p || !b || !s) return;

    bool unstable = (instability_mask != ZN_INSTABILITY_NONE);
    bool drifting = !!(instability_mask & ZN_INSTABILITY_DRIFT);
    bool thermal  = !!(instability_mask & ZN_INSTABILITY_THERMAL);

    /* --- Kararsız: parallelism kıs --- */
    if (unstable) {
        if (p->threads > b->min_threads) p->threads--;
        if (p->batch_size > b->min_batch) {
            p->batch_size = (p->batch_size * 3) / 4;  /* %25 küçült */
            if (p->batch_size < b->min_batch) p->batch_size = b->min_batch;
        }
    }

    /* --- Termal: tile boyutunu düşür (cache pressure azalt) --- */
    if (thermal && p->tile_size > b->min_tile) {
        p->tile_size = (p->tile_size * 3) / 4;
        if (p->tile_size < b->min_tile) p->tile_size = b->min_tile;
    }

    /* --- Drift ve yüksek latency: fusion threshold düşür
     *    (daha fazla fusion = daha az kernel launch = daha düşük jitter) --- */
    if (drifting && s->mean > target_ms * 1.05) {
        if (p->fusion_threshold > b->min_fusion) p->fusion_threshold--;
    }

    /* --- Kararlı: yavaşça geri artır --- */
    if (!unstable && s->mean < target_ms * 0.95) {
        if (p->threads < b->max_threads) p->threads++;
        if (p->batch_size < b->max_batch) {
            p->batch_size = (int)(p->batch_size * 1.1) + 1;
            if (p->batch_size > b->max_batch) p->batch_size = b->max_batch;
        }
    }

    if (!unstable && !thermal && p->tile_size < b->max_tile)
        p->tile_size++;
}

void zn_enforce_stability(const ZnEnvelope *e,
                          ZnExecutionProfile *p,
                          const ZnProfileBounds *b,
                          ZnPacingController *pc,
                          const ZnLatencyStats *s,
                          int instability_mask)
{
    if (!e || !p || !b || !pc || !s) return;
    if (zn_within_envelope(e, s)) return;

    /* Zarfın dışındayız — agresif düzeltme */
    double excess = s->mean - e->max_ms;
    if (excess > 0.0) {
        /* Ortalama üstte: thread sayısını minimuma çek, pacing arttır */
        p->threads    = b->min_threads;
        p->batch_size = b->min_batch;
        /* Pacing: hata ile orantılı */
        double add_ns = excess * 1e6 * 0.8;
        if (add_ns > (double)ZN_PACE_MAX_NS) add_ns = (double)ZN_PACE_MAX_NS;
        pc->accum_ns = fmin(pc->accum_ns + add_ns, (double)ZN_PACE_MAX_NS);
    }

    /* p99 tavan ihlali */
    if (s->p99 > e->max_p99_ms && s->n >= 8) {
        if (p->threads > b->min_threads) p->threads--;
        pc->accum_ns = fmin(pc->accum_ns + 150000.0, (double)ZN_PACE_MAX_NS);
    }

    (void)instability_mask;
}

/* =========================================================================
 * Opak Stabilizer yapısı
 * ========================================================================= */

struct ZnStabilizer {
    ZnStabilizerConfig  cfg;
    ZnVarianceAnalyzer *analyzer;
    ZnThermalState      thermal;
    ZnPacingController  pacing;
    ZnExecutionProfile  profile;
    ZnEnvelope          envelope;

    uint64_t            t_begin_ns;    /* son begin() zamanı       */
    uint64_t            step;          /* toplam çağrı sayısı      */
    int                 last_instability;
    bool                in_envelope;
};

/* =========================================================================
 * Default config
 * ========================================================================= */

ZnStabilizerConfig zn_stabilizer_default_config(double target_ms)
{
    ZnStabilizerConfig c;
    memset(&c, 0, sizeof(c));
    c.target_ms      = (target_ms > 0.0) ? target_ms : 1.0;
    c.envelope_pct   = 0.05;                  /* ±5% */
    c.max_p99_ms     = target_ms * 2.0;       /* p99 tavanı 2x */
    c.window         = ZN_RING_DEFAULT_WINDOW;
    c.warmup_steps   = ZN_WARMUP_DEFAULT;
    c.bounds.min_threads  = 1;
    c.bounds.max_threads  = 8;
    c.bounds.min_batch    = 1;
    c.bounds.max_batch    = 64;
    c.bounds.min_tile     = 16;
    c.bounds.max_tile     = 256;
    c.bounds.min_fusion   = 2;
    c.bounds.max_fusion   = 8;
    c.enable_thermal = true;
    c.enable_pacing  = true;
    c.enable_autotune = true;
    c.verbose        = false;
    return c;
}

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

ZnStabilizer *zn_stabilizer_create(const ZnStabilizerConfig *cfg)
{
    if (!cfg) return NULL;

    ZnStabilizer *s = calloc(1, sizeof(*s));
    if (!s) return NULL;

    s->cfg      = *cfg;

    s->analyzer = zna_create(cfg->window > 0 ? cfg->window : ZN_RING_DEFAULT_WINDOW);
    if (!s->analyzer) { free(s); return NULL; }

    zn_pacing_init(&s->pacing);
    zn_profile_init(&s->profile, &cfg->bounds);
    zn_envelope_init(&s->envelope,
                     cfg->target_ms,
                     cfg->envelope_pct,
                     cfg->max_p99_ms);

    if (cfg->enable_thermal)
        zn_thermal_init(&s->thermal);

    s->in_envelope = true;
    return s;
}

void zn_stabilizer_destroy(ZnStabilizer *s)
{
    if (!s) return;
    zna_destroy(s->analyzer);
    free(s);
}

/* =========================================================================
 * Ana döngü: begin / end
 * ========================================================================= */

void zn_stabilizer_begin(ZnStabilizer *s)
{
    if (!s) return;
    s->t_begin_ns = zn_time_now_ns();
}

void zn_stabilizer_end(ZnStabilizer *s)
{
    if (!s) return;

    uint64_t t_end = zn_time_now_ns();
    double elapsed_ms = zn_time_elapsed_ms(s->t_begin_ns, t_end);

    s->step++;

    /* 1. Ölçümü kaydet */
    zna_push(s->analyzer, elapsed_ms);
    ZnLatencyStats stats = zna_stats(s->analyzer);

    /* 2. Warmup'ta control yok — sadece ölç */
    if (s->step < s->cfg.warmup_steps) {
        if (s->cfg.verbose) {
            fprintf(stderr, "[zn] warmup %4llu: %.3fms\n",
                    (unsigned long long)s->step, elapsed_ms);
        }
        return;
    }

    /* 3. Termal probe (periyodik — her adımda değil) */
    int thermal_flag = 0;
    if (s->cfg.enable_thermal) {
        int throttled = zn_detect_throttling(&s->thermal);
        if (throttled) thermal_flag = ZN_INSTABILITY_THERMAL;
    }

    /* 4. Kararsızlık tespiti */
    int instability = zna_detect(s->analyzer,
                                 s->cfg.target_ms,
                                 s->cfg.max_p99_ms,
                                 s->cfg.envelope_pct);
    instability |= thermal_flag;
    s->last_instability = instability;

    /* 5. Envelope kontrolü */
    s->in_envelope = zn_within_envelope(&s->envelope, &stats);

    /* 6. Pacing güncelle */
    if (s->cfg.enable_pacing) {
        if (instability == ZN_INSTABILITY_NONE && s->in_envelope) {
            zn_pacing_relax(&s->pacing);
        } else {
            zn_pacing_update(&s->pacing, elapsed_ms,
                             s->cfg.target_ms, instability);
        }
    }

    /* 7. Profil autotune */
    if (s->cfg.enable_autotune) {
        zn_profile_autotune(&s->profile, &s->cfg.bounds,
                            &stats, s->cfg.target_ms, instability);
    }

    /* 8. Envelope dışındaysa zorla düzeltme */
    if (!s->in_envelope) {
        zn_enforce_stability(&s->envelope, &s->profile,
                             &s->cfg.bounds, &s->pacing,
                             &stats, instability);
    }

    /* 9. Verbose log */
    if (s->cfg.verbose) {
        char flags[64] = "";
        if (instability & ZN_INSTABILITY_SPIKE)   strcat(flags, "SPIKE ");
        if (instability & ZN_INSTABILITY_VARIANCE) strcat(flags, "VAR ");
        if (instability & ZN_INSTABILITY_DRIFT)    strcat(flags, "DRIFT ");
        if (instability & ZN_INSTABILITY_TAIL)     strcat(flags, "TAIL ");
        if (instability & ZN_INSTABILITY_THERMAL)  strcat(flags, "THERM ");
        fprintf(stderr,
            "[zn] step=%-5llu lat=%.3fms mean=%.3fms std=%.3f "
            "p99=%.3f thr=%d env=%s %s\n",
            (unsigned long long)s->step,
            elapsed_ms, stats.mean, stats.stddev,
            stats.p99, s->profile.threads,
            s->in_envelope ? "IN" : "OUT",
            flags);
    }

    /* 10. Pacing uygula (ölçüm DIŞINDA) */
    if (s->cfg.enable_pacing) {
        zn_pacing_apply(&s->pacing);
    }
}

/* =========================================================================
 * Accessors
 * ========================================================================= */

ZnLatencyStats zn_stabilizer_stats(const ZnStabilizer *s)
{
    if (!s || !s->analyzer) {
        ZnLatencyStats z; memset(&z, 0, sizeof(z)); return z;
    }
    return zna_stats(s->analyzer);
}

ZnExecutionProfile zn_stabilizer_profile(const ZnStabilizer *s)
{
    ZnExecutionProfile z;
    memset(&z, 0, sizeof(z));
    if (!s) return z;
    return s->profile;
}

ZnThermalState zn_stabilizer_thermal(const ZnStabilizer *s)
{
    ZnThermalState z;
    memset(&z, 0, sizeof(z));
    if (!s) return z;
    return s->thermal;
}

int zn_stabilizer_instability(const ZnStabilizer *s)
{
    return s ? s->last_instability : 0;
}

int zn_stabilizer_in_envelope(const ZnStabilizer *s)
{
    return s ? (s->in_envelope ? 1 : 0) : 0;
}

void zn_stabilizer_reset_pacing(ZnStabilizer *s)
{
    if (s) zn_pacing_init(&s->pacing);
}

/* =========================================================================
 * Rapor
 * ========================================================================= */

void zn_stabilizer_print_report(const ZnStabilizer *s)
{
    if (!s) return;
    ZnLatencyStats st = zna_stats(s->analyzer);

    const char *bar = "═══════════════════════════════════════════════════════════";

    fprintf(stderr, "\n%s\n", bar);
    fprintf(stderr, "  Nova Deterministic Compute Controller — Rapor\n");
    fprintf(stderr, "%s\n", bar);
    fprintf(stderr, "  Hedef latency  : %.3f ms\n", s->cfg.target_ms);
    fprintf(stderr, "  Envelope       : ±%.1f%%  (%.3f – %.3f ms)\n",
            s->cfg.envelope_pct * 100.0,
            s->envelope.min_ms, s->envelope.max_ms);
    fprintf(stderr, "  p99 tavanı     : %.3f ms\n", s->cfg.max_p99_ms);
    fprintf(stderr, "  Toplam adım    : %llu\n",  (unsigned long long)st.n);
    fprintf(stderr, "  Spike sayısı   : %llu\n",  (unsigned long long)st.spikes);
    fprintf(stderr, "%s\n", bar);
    fprintf(stderr, "  Ölçülen latency istatistikleri (ms):\n");
    fprintf(stderr, "    Mean         : %.4f\n", st.mean);
    fprintf(stderr, "    StdDev       : %.4f\n", st.stddev);
    fprintf(stderr, "    Best         : %.4f\n", st.best);
    fprintf(stderr, "    Worst        : %.4f\n", st.worst);
    fprintf(stderr, "    P95          : %.4f\n", st.p95);
    fprintf(stderr, "    P99          : %.4f\n", st.p99);
    fprintf(stderr, "    EMA (fast)   : %.4f\n", st.ema_fast);
    fprintf(stderr, "    EMA (slow)   : %.4f\n", st.ema_slow);
    fprintf(stderr, "    CoV          : %.4f\n",
            st.mean > 0.0 ? st.stddev / st.mean : 0.0);
    fprintf(stderr, "%s\n", bar);
    fprintf(stderr, "  Son execution profil:\n");
    fprintf(stderr, "    Threads      : %d\n", s->profile.threads);
    fprintf(stderr, "    Batch size   : %d\n", s->profile.batch_size);
    fprintf(stderr, "    Tile size    : %d\n", s->profile.tile_size);
    fprintf(stderr, "    Fusion thr.  : %d\n", s->profile.fusion_threshold);
    fprintf(stderr, "%s\n", bar);
    fprintf(stderr, "  Pacing istatistikleri:\n");
    fprintf(stderr, "    Toplam uyku  : %.3f ms\n",
            (double)s->pacing.total_slept_ns / 1e6);
    fprintf(stderr, "    Uyku adımı   : %u\n", s->pacing.sleep_count);
    if (s->cfg.enable_thermal) {
        fprintf(stderr, "  Termal durum:\n");
        fprintf(stderr, "    Frekans oran.: %.3f\n",  s->thermal.freq_ratio);
        fprintf(stderr, "    Throttled    : %s\n",
                s->thermal.throttled ? "EVET ⚠" : "Hayır");
        fprintf(stderr, "    Turbo osc.   : %s\n",
                s->thermal.turbo_oscillating ? "EVET" : "Hayır");
    }
    fprintf(stderr, "  Envelope durumu: %s\n",
            s->in_envelope ? "✓ İÇİNDE" : "✗ DIŞINDA");
    fprintf(stderr, "%s\n\n", bar);
}
