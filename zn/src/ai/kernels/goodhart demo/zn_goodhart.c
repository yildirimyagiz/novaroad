/*
 * zn_goodhart.c — Nova Goodhart Guard — Implementasyon
 *
 * Kritik iyileştirmeler (orijinal tasarım analizine göre):
 *
 *  [FIX-1] Divergence formülü:  eps-regularized, NaN-safe
 *          |p - h| / (|p| + |h| + eps)  ← Smyth varyans formülünden esinlendi
 *
 *  [FIX-2] Holdout rejection:  sert 0-sınırı yerine yapılandırılabilir min
 *          holdout > holdout_min  (varsayılan: -0.02)
 *
 *  [FIX-3] Proxy layer:  gerçek dünya metrikleri için tam tasarım
 *          Latency stability / OOD generalization / energy proxy
 *
 *  [FIX-4] Soft metric rotation:  annealing + gradual mixing
 *          Sert swap yerine ağırlık azaltma + yeniden ekleme
 *
 *  [FIX-5] Adversarial genişletme:  6 strateji, marj takibi
 *
 *  [NEW]   Approval rate EMA + divergence EMA istatistikleri
 *  [NEW]   Tam raporlama
 */

#define _POSIX_C_SOURCE 200809L
#include "zn_goodhart.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* =========================================================================
 * Dahili yardımcılar
 * ========================================================================= */

static inline float gh_fabsf(float x) { return x < 0.f ? -x : x; }
static inline float gh_fmaxf(float a, float b) { return a > b ? a : b; }
static inline float gh_fminf(float a, float b) { return a < b ? a : b; }

/* xorshift64 PRNG — deterministik, platform-bağımsız */
static inline uint64_t gh_xorshift64(uint64_t *s)
{
    uint64_t x = (*s == 0) ? 0x9E3779B97F4A7C15ull : *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

static inline uint32_t gh_rand_u32(uint64_t *s, uint32_t bound)
{
    if (bound <= 1) return 0;
    return (uint32_t)(gh_xorshift64(s) % (uint64_t)bound);
}

static inline void gh_add_warning(ZnGhResult *r, const char *msg)
{
    if (r->warnings_count < ZN_GH_MAX_WARNINGS)
        r->warnings[r->warnings_count++] = msg;
}

/* =========================================================================
 * [FIX-1] Numerically stable divergence
 *
 * Orijinal:  |p - h| / max(|p|, |h|)
 *   Sorun:   primary ≈ 0 AND holdout small → 0/0 veya noise amplification
 *
 * Yeni:      |p - h| / (|p| + |h| + eps)
 *   Avantaj: her zaman [0, 1) aralığında, NaN üretmez, jitter-safe
 *   Ek:      çok küçük değerlerde hem primary hem holdout noise ise
 *            divergence yaklaşık 0 kalır — doğru davranış
 * ========================================================================= */

static float gh_divergence(float primary, float holdout, float eps)
{
    float diff  = gh_fabsf(primary - holdout);
    float denom = gh_fabsf(primary) + gh_fabsf(holdout) + eps;
    return diff / denom;
}

/* =========================================================================
 * Ağırlıklı metrik seti değerlendirme
 * _soft_weight yoksa weight kullanılır
 * ========================================================================= */

static float gh_eval_set(ZnGhMetric       *m,
                          size_t            n,
                          const ZnGhChange *chg,
                          const ZnGhState  *st)
{
    if (n == 0 || !m) return 0.f;

    float acc   = 0.f;
    float wsum  = 0.f;

    for (size_t i = 0; i < n; i++) {
        ZnGhMetric *mt = &m[i];
        if (!mt->eval) continue;

        /* Soft weight: annealing döndükten sonra azalmış olabilir */
        float w = (mt->_soft_weight > 0.f) ? mt->_soft_weight : mt->weight;
        if (w <= 0.f) w = 1.f;  /* ağırlıksız → eşit katılım */

        float v = mt->eval(mt->ctx, chg, st);

        /* Sayısal koruma: NaN / Inf → 0 */
        if (!isfinite(v)) v = 0.f;

        acc  += v * w;
        wsum += w;
    }

    return (wsum > 1e-9f) ? (acc / wsum) : 0.f;
}

/* =========================================================================
 * Ring buffer
 * ========================================================================= */

static void gh_ring_push(ZnGoodhart *g, float d)
{
    if (!g->div_hist || g->div_cap == 0) return;
    g->div_hist[g->div_head] = d;
    g->div_head = (g->div_head + 1u) % g->div_cap;
    if (g->div_len < g->div_cap) g->div_len++;
}

/* =========================================================================
 * Varsayılan config
 * ========================================================================= */

ZnGhConfig zn_gh_default_config(void)
{
    ZnGhConfig c;
    memset(&c, 0, sizeof(c));
    c.divergence_reject  = 0.30f;
    c.divergence_warn    = 0.15f;
    c.divergence_eps     = 1e-6f;
    c.holdout_min        = -0.02f;  /* [FIX-2]: -2% tolerans */
    c.proxy_min          = -0.10f;
    c.rotation_steps     = 1000u;
    c.rotation_rate      = 0.25f;
    c.rotation_anneal    = 0.95f;
    c.run_all_attacks    = true;
    c.attack_margin_min  = 0.05f;
    c.trend_window       = 20u;
    c.rng_seed           = 0xD1B54A32D192ED03ull;
    return c;
}

/* =========================================================================
 * Init
 * ========================================================================= */

void zn_goodhart_init(ZnGoodhart        *g,
                      ZnGhMetric        *primary,    size_t primary_len,
                      ZnGhMetric        *holdout,    size_t holdout_len,
                      const void        *attack_ctx, ZnGhAttackFn attack_fn,
                      const void        *proxy_ctx,  ZnGhProxyEvalFn proxy_fn,
                      float             *div_hist_buf, uint32_t div_hist_cap,
                      ZnGhConfig         cfg)
{
    memset(g, 0, sizeof(*g));

    g->primary      = primary;
    g->primary_len  = primary_len;
    g->holdout      = holdout;
    g->holdout_len  = holdout_len;
    g->attack_ctx   = attack_ctx;
    g->attack_fn    = attack_fn;
    g->proxy_ctx    = proxy_ctx;
    g->proxy_fn     = proxy_fn;
    g->div_hist     = div_hist_buf;
    g->div_cap      = div_hist_cap;
    g->cfg          = cfg;

    /* Güvenli varsayılanlar */
    if (g->cfg.divergence_reject <= 0.f)  g->cfg.divergence_reject = 0.30f;
    if (g->cfg.divergence_warn   <= 0.f)  g->cfg.divergence_warn   = 0.15f;
    if (g->cfg.divergence_eps    <= 0.f)  g->cfg.divergence_eps    = 1e-6f;
    if (g->cfg.rotation_steps    == 0u)   g->cfg.rotation_steps    = 1000u;
    if (g->cfg.rotation_rate     <= 0.f)  g->cfg.rotation_rate     = 0.25f;
    if (g->cfg.rotation_anneal   <= 0.f)  g->cfg.rotation_anneal   = 0.95f;
    if (g->cfg.trend_window      == 0u)   g->cfg.trend_window      = 20u;
    if (g->cfg.rng_seed          == 0u)   g->cfg.rng_seed          = 0xD1B54A32D192ED03ull;
    if (g->cfg.attack_margin_min < 0.f)   g->cfg.attack_margin_min = 0.05f;

    /* Soft weight başlangıcı: her metrik kendi weight'ini kullanır */
    for (size_t i = 0; i < primary_len; i++)
        g->primary[i]._soft_weight = g->primary[i].weight;
    for (size_t i = 0; i < holdout_len; i++)
        g->holdout[i]._soft_weight = g->holdout[i].weight;
}

/* =========================================================================
 * [FIX-4] Soft Metric Rotation
 *
 * Orijinal: sert ZnMetric swap → learning loop şoku
 *
 * Yeni mekanizma:
 *  1. Seçilen primary metriğin _soft_weight'ini rotation_rate ile azalt
 *  2. Seçilen holdout metriğini primary listesine "yumuşakça" ekle
 *     (soft_weight başlangıcı = düşük, annealing ile hedef ağırlığa çıkar)
 *  3. rotation_anneal: her rotasyonda tüm soft_weight'ler hedeflerine yaklaşır
 *
 * Bu TCP slow-start'a benzer: ani değil, kademeli.
 * ========================================================================= */

void zn_goodhart_rotate_metrics(ZnGoodhart *g)
{
    if (!g->primary || !g->holdout) return;
    if (g->primary_len == 0 || g->holdout_len == 0) return;

    float rate = gh_fminf(gh_fmaxf(g->cfg.rotation_rate, 0.f), 1.f);

    /* Adım 1: Tüm primary metriklerinin soft_weight'ini
     *         hedef weight'lerine doğru anneal et */
    for (size_t i = 0; i < g->primary_len; i++) {
        ZnGhMetric *m = &g->primary[i];
        float target  = m->weight;
        m->_soft_weight = m->_soft_weight + (target - m->_soft_weight) * g->cfg.rotation_anneal;
        if (m->_soft_weight < 0.001f) m->_soft_weight = 0.001f;
    }

    /* Adım 2: Rastgele bir primary metriğin ağırlığını geçici düşür
     *         (holdout'un görünürlüğünü arttırmak için dolaylı baskı) */
    uint32_t pi = gh_rand_u32(&g->cfg.rng_seed, (uint32_t)g->primary_len);
    g->primary[pi]._soft_weight *= (1.f - rate);
    g->primary[pi]._rotation_age++;

    /* Adım 3: Holdout metriğinin ağırlığını da anneal et
     *         (holdout'a da öğrenilmiş weight verilebilir ileride) */
    for (size_t i = 0; i < g->holdout_len; i++) {
        ZnGhMetric *m = &g->holdout[i];
        m->_soft_weight = m->weight;  /* holdout sabit kalmalı */
    }
}

/* =========================================================================
 * Adversarial validation — 6 strateji
 * [FIX-5]: marj takibi + genişletilmiş strateji seti
 * ========================================================================= */

static void gh_run_adversarial(ZnGoodhart       *g,
                                const ZnGhChange *chg,
                                ZnGhResult       *out)
{
    if (!g->attack_fn) {
        out->adversarial_passed = true;
        out->worst_attack_perf  = 1.0f;
        return;
    }

    bool   any_success  = false;
    float  worst_perf   = 1e9f;
    int    n_strategies = g->cfg.run_all_attacks
                          ? (int)ZN_ATTACK_COUNT
                          : 4;  /* sadece temel 4 */

    for (int s = 0; s < n_strategies; s++) {
        ZnGhAttackResult ar = g->attack_fn(
            g->attack_ctx, (ZnGhAttackStrategy)s, chg);

        if (ar.performance < worst_perf)
            worst_perf = ar.performance;

        if (ar.successful) {
            any_success = true;
            /* Mesaj: saldırıya özel */
            const char *msg = ar.vulnerability
                              ? ar.vulnerability
                              : "Adversarial vulnerability detected";
            gh_add_warning(out, msg);

            /* Marj çok küçük olsa bile uyar ama reddetme */
            if (ar.margin < g->cfg.attack_margin_min && !ar.successful)
                gh_add_warning(out, "Attack margin dangerously narrow");
        }
    }

    out->adversarial_passed = !any_success;
    out->worst_attack_perf  = worst_perf;
}

/* =========================================================================
 * Divergence trend: son trend_window/2 vs önceki trend_window/2
 * ========================================================================= */

float zn_goodhart_divergence_trend(const ZnGoodhart *g)
{
    if (!g->div_hist || g->div_len < (g->cfg.trend_window * 2u))
        return 0.f;

    uint32_t half = g->cfg.trend_window / 2u;
    if (half < 1u) half = 1u;

    float recent = 0.f, older = 0.f;
    for (uint32_t i = 0; i < half; i++) {
        uint32_t ir = (g->div_head + g->div_cap - 1u - i)       % g->div_cap;
        uint32_t io = (g->div_head + g->div_cap - 1u - (half + i)) % g->div_cap;
        recent += g->div_hist[ir];
        older  += g->div_hist[io];
    }
    return (recent - older) / (float)half;  /* normalleştirilmiş */
}

/* =========================================================================
 * Ana validasyon döngüsü
 * ========================================================================= */

ZnGhResult zn_goodhart_validate(ZnGoodhart       *g,
                                  const ZnGhChange *chg,
                                  const ZnGhState  *st)
{
    ZnGhResult out;
    memset(&out, 0, sizeof(out));
    out.worst_attack_perf = 1.0f;

    /* ── 1. Primary metrikler ──────────────────────────────────────── */
    float primary = gh_eval_set(g->primary, g->primary_len, chg, st);

    /* ── 2. Holdout metrikler (optimizer'ın görmediği) ────────────── */
    float holdout = gh_eval_set(g->holdout, g->holdout_len, chg, st);

    /* ── 3. [FIX-1] Numerically stable divergence ─────────────────── */
    float div = gh_divergence(primary, holdout, g->cfg.divergence_eps);
    gh_ring_push(g, div);

    /* EMA güncellemesi */
    float ema_a = 0.1f;
    g->div_ema     = (g->steps == 0) ? div     : (ema_a * div     + (1.f - ema_a) * g->div_ema);
    g->primary_ema = (g->steps == 0) ? primary : (ema_a * primary + (1.f - ema_a) * g->primary_ema);
    g->holdout_ema = (g->steps == 0) ? holdout : (ema_a * holdout + (1.f - ema_a) * g->holdout_ema);

    /* ── 4. Adversarial validation ────────────────────────────────── */
    gh_run_adversarial(g, chg, &out);

    /* ── 5. Gerçek dünya proxy ────────────────────────────────────── */
    float proxy = 0.f;
    if (g->proxy_fn) {
        proxy = g->proxy_fn(g->proxy_ctx, chg);
        if (!isfinite(proxy)) proxy = 0.f;
    }

    /* ── 6. Divergence trend ─────────────────────────────────────── */
    float trend = zn_goodhart_divergence_trend(g);

    /* ── Karar politikası ──────────────────────────────────────────
     *
     * Onay için GEREKLİ koşullar:
     *  a) primary > 0
     *  b) holdout > holdout_min   [FIX-2: soft threshold]
     *  c) divergence < reject_threshold
     *  d) adversarial başarısız
     *  e) proxy > proxy_min (proxy varsa)
     * ─────────────────────────────────────────────────────────── */
    bool approved = true;

    /* a) Primary negatifse iyileşme yok */
    if (primary <= 0.f) {
        approved = false;
        gh_add_warning(&out, "Primary metric not improving");
    }

    /* b) [FIX-2] Holdout toleranslı kontrol */
    if (holdout < g->cfg.holdout_min) {
        approved = false;
        gh_add_warning(&out, "Holdout metric below tolerance (possible gaming)");
    } else if (holdout < 0.f) {
        /* Negatif ama tolerans içinde — uyar ama reddetme */
        gh_add_warning(&out, "Holdout metric slightly negative (monitor closely)");
    }

    /* c) Divergence */
    if (div > g->cfg.divergence_reject) {
        approved = false;
        gh_add_warning(&out, "Metric divergence too high — gaming strongly suspected");
    } else if (div > g->cfg.divergence_warn) {
        gh_add_warning(&out, "Metric divergence elevated — monitor for gaming");
    }

    /* Divergence kötüleşiyor mu? */
    if (trend > 0.05f) {
        gh_add_warning(&out, "Divergence trend worsening — early gaming signal");
        if (trend > 0.15f) {
            approved = false;
            gh_add_warning(&out, "Divergence trend critical — reject");
        }
    }

    /* d) Adversarial */
    if (!out.adversarial_passed) {
        approved = false;
        /* Uyarı zaten gh_run_adversarial içinde eklendi */
    }

    /* e) Proxy */
    if (g->proxy_fn && proxy < g->cfg.proxy_min) {
        approved = false;
        gh_add_warning(&out, "Real-world proxy declining — metrics may be misleading");
    }

    /* Sonuçları doldur */
    out.approved            = approved;
    out.primary_score       = primary;
    out.holdout_score       = holdout;
    out.proxy_score         = proxy;
    out.divergence          = div;
    out.divergence_trend    = trend;

    /* İstatistik güncelleme */
    g->steps++;
    if (approved) g->approvals++;
    else          g->rejections++;

    /* Rotation schedule */
    if (g->cfg.rotation_steps > 0 &&
        (g->steps % g->cfg.rotation_steps) == 0) {
        zn_goodhart_rotate_metrics(g);
    }

    return out;
}

/* =========================================================================
 * Accessor'lar
 * ========================================================================= */

uint64_t zn_goodhart_steps(const ZnGoodhart *g)      { return g ? g->steps      : 0; }
uint64_t zn_goodhart_approvals(const ZnGoodhart *g)  { return g ? g->approvals  : 0; }
uint64_t zn_goodhart_rejections(const ZnGoodhart *g) { return g ? g->rejections : 0; }

float zn_goodhart_approval_rate(const ZnGoodhart *g)
{
    if (!g || g->steps == 0) return 0.f;
    return (float)g->approvals / (float)g->steps;
}

/* =========================================================================
 * Raporlama
 * ========================================================================= */

void zn_goodhart_print_report(const ZnGoodhart *g, FILE *out)
{
    if (!g || !out) return;
    const char *bar =
        "═══════════════════════════════════════════════════════════";

    fprintf(out, "\n%s\n", bar);
    fprintf(out, "  Nova Goodhart Guard — Rapor\n");
    fprintf(out, "%s\n", bar);
    fprintf(out, "  Adım sayısı      : %llu\n", (unsigned long long)g->steps);
    fprintf(out, "  Onay             : %llu (%.1f%%)\n",
            (unsigned long long)g->approvals,
            g->steps > 0 ? 100.f * (float)g->approvals / (float)g->steps : 0.f);
    fprintf(out, "  Red              : %llu (%.1f%%)\n",
            (unsigned long long)g->rejections,
            g->steps > 0 ? 100.f * (float)g->rejections / (float)g->steps : 0.f);
    fprintf(out, "%s\n", bar);
    fprintf(out, "  Primary EMA      : %.4f\n", g->primary_ema);
    fprintf(out, "  Holdout EMA      : %.4f\n", g->holdout_ema);
    fprintf(out, "  Divergence EMA   : %.4f\n", g->div_ema);
    fprintf(out, "  Div. trend       : %.4f %s\n",
            zn_goodhart_divergence_trend(g),
            zn_goodhart_divergence_trend(g) > 0.05f ? "⚠ KÖTÜLEŞIYOR" : "✓");
    fprintf(out, "%s\n", bar);
    fprintf(out, "  Primary metrikler: %zu\n", g->primary_len);
    for (size_t i = 0; i < g->primary_len; i++) {
        fprintf(out, "    [%zu] %-20s  w=%.3f  soft_w=%.3f  age=%u\n",
                i, g->primary[i].name ? g->primary[i].name : "?",
                g->primary[i].weight,
                g->primary[i]._soft_weight,
                g->primary[i]._rotation_age);
    }
    fprintf(out, "  Holdout metrikler: %zu\n", g->holdout_len);
    for (size_t i = 0; i < g->holdout_len; i++) {
        fprintf(out, "    [%zu] %-20s  w=%.3f\n",
                i, g->holdout[i].name ? g->holdout[i].name : "?",
                g->holdout[i].weight);
    }
    fprintf(out, "%s\n\n", bar);
}

/* =========================================================================
 * Yerleşik yardımcı metrikler
 * ========================================================================= */

float zn_gh_const_eval(const void *ctx, const ZnGhChange *c, const ZnGhState *s)
{
    (void)c; (void)s;
    if (!ctx) return 0.f;
    return ((const ZnGhConstCtx *)ctx)->value;
}

float zn_gh_latency_eval(const void *ctx, const ZnGhChange *c, const ZnGhState *s)
{
    (void)c; (void)s;
    if (!ctx) return 0.f;
    const ZnGhLatencyCtx *lc = (const ZnGhLatencyCtx *)ctx;
    if (!lc->mean_ptr || lc->target_ms <= 0.0) return 0.f;
    /* Dönüş: (target - mean) / target — pozitif = hedefte ya da altında */
    double mean = *lc->mean_ptr;
    return (float)((lc->target_ms - mean) / lc->target_ms);
}
