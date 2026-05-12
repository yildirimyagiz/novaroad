/*
 * zn_goodhart.h — Nova Goodhart Guard
 *
 * "When a measure becomes a target, it ceases to be a good measure."
 *                                                    — Goodhart's Law
 *
 * Bu katman Nova'in meta-optimization sanity firewall'udur.
 * Bir optimizasyon döngüsü bir metriği "oynarken" (gaming) onu gerçek
 * iyileşme olarak kabul etmemek için tasarlanmıştır.
 *
 * Koruma mekanizmaları:
 *
 *   1. PRIMARY  / HOLDOUT ayrımı      → optimizer'ın görmediği metrikler
 *   2. DIVERGENCE dedektörü           → primary↑ + holdout↓ = gaming sinyali
 *   3. ADVERSARIAL validator          → worst-case / edge-case / OOD testleri
 *   4. REAL-WORLD PROXY               → iş metriği / enerji / kullanıcı etkisi
 *   5. SOFT METRIC ROTATION           → annealing ile holdout karışımı
 *   6. NUMERICALLY STABLE divergence  → eps-regularized, jitter-safe
 *
 * Mimari pozisyon (Nova katman yığını):
 *
 *   nova_graph + nova_fusion           ← compute graph
 *   performance_stabilizer                 ← latency variance controller
 *   zn_goodhart                            ← metric integrity guard  ← BU
 *
 * Entegrasyon:
 *
 *   ZnGoodhart *g = zn_goodhart_create(&cfg, primary, np, holdout, nh);
 *   zn_goodhart_set_attack_fn(g, my_attack_fn, ctx);
 *   zn_goodhart_set_proxy_fn(g, my_proxy_fn, ctx);
 *
 *   // her optimizasyon adımında:
 *   ZnGhResult res = zn_goodhart_validate(g, &change, &state);
 *   if (!res.approved) rollback();
 *
 *   zn_goodhart_destroy(g);
 *
 * Build:
 *   cc -O2 -std=c11 zn_goodhart.c -lm
 */

#ifndef ZN_GOODHART_H
#define ZN_GOODHART_H

#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Sürüm
 * ========================================================================= */
#define ZN_GH_VERSION_MAJOR 1
#define ZN_GH_VERSION_MINOR 1   /* orijinalden iyileştirildi */
#define ZN_GH_VERSION_PATCH 0

/* =========================================================================
 * İleri bildirimler
 * ========================================================================= */
typedef struct ZnGhChange ZnGhChange;  /* optimizasyon değişikliği */
typedef struct ZnGhState  ZnGhState;   /* sistem durumu           */

/* =========================================================================
 * Değişiklik ve Durum — kullanıcı tanımlar, Goodhart Guard geçirir
 * ========================================================================= */

/**
 * ZnGhChange: bir optimizasyon adımını temsil eder.
 * Ne değiştiğini metrik fonksiyonlarınız yorumlar.
 */
struct ZnGhChange {
    const char *label;      /* debug için isim (NULL olabilir)       */
    const void *data;       /* kullanıcı tanımlı yük                 */
    size_t      data_size;
    uint64_t    step;       /* global optimizasyon adımı             */
};

/**
 * ZnGhState: değişikliğin uygulandığı bağlam.
 */
struct ZnGhState {
    const void *data;
    size_t      data_size;
    uint64_t    timestamp_ns;
};

/* =========================================================================
 * Metrik sistemi
 * ========================================================================= */

/**
 * Metrik değerlendirme fonksiyonu.
 * Dönüş: herhangi bir float — yüksek = daha iyi (sistemin içinde normalize edilir).
 * NOT: bu fonksiyon saf (pure) olmalıdır — iç durum değiştirmemeli.
 */
typedef float (*ZnGhMetricEvalFn)(const void *ctx,
                                   const ZnGhChange *chg,
                                   const ZnGhState  *st);

typedef struct {
    const char      *name;    /* debug / raporlama                    */
    float            weight;  /* ağırlık (0 → otomatik normalize)     */
    const void      *ctx;     /* metrik fonksiyonuna özel bağlam      */
    ZnGhMetricEvalFn eval;    /* fonksiyon işaretçisi                 */

    /* Soft rotation state (dahili — kullanıcı dokunmaz) */
    float  _soft_weight;      /* annealing ile karıştırılan ağırlık   */
    uint32_t _rotation_age;   /* kaç kez döndürüldü                   */
} ZnGhMetric;

/* =========================================================================
 * Adversarial validator
 * ========================================================================= */

typedef enum {
    ZN_ATTACK_ADVERSARIAL_INPUTS = 0,  /* düşman girdiler              */
    ZN_ATTACK_DISTRIBUTION_SHIFT = 1,  /* dağılım kayması              */
    ZN_ATTACK_WORST_CASE         = 2,  /* en kötü durum                */
    ZN_ATTACK_EDGE_CASES         = 3,  /* kenar durumlar               */
    ZN_ATTACK_NUMERICAL_STRESS   = 4,  /* sayısal kararsızlık          */
    ZN_ATTACK_MEMORY_PRESSURE    = 5,  /* bellek baskısı               */
    ZN_ATTACK_COUNT              = 6,
} ZnGhAttackStrategy;

typedef struct {
    bool        successful;
    float       performance;     /* düşük = kötü, yüksek = iyi         */
    float       margin;          /* başarı/başarısızlık marjı           */
    const char *vulnerability;   /* NULL veya statik string             */
} ZnGhAttackResult;

/**
 * Saldırı fonksiyonu: verilen strateji ile sistemi dene.
 * Dönüş: ZnGhAttackResult — başarılı saldırı = sistemin zayıf noktası.
 */
typedef ZnGhAttackResult (*ZnGhAttackFn)(const void         *ctx,
                                          ZnGhAttackStrategy  strat,
                                          const ZnGhChange   *chg);

/* =========================================================================
 * Gerçek dünya proxy
 * ========================================================================= */

/**
 * Proxy değerlendirme: metrik dışı gerçek dünya ölçümü.
 * Örnekler: kullanıcı memnuniyeti, enerji tüketimi, iş KPI'ı.
 * Dönüş: -1.0 (tamamen kötü) … +1.0 (tamamen iyi).
 */
typedef float (*ZnGhProxyEvalFn)(const void      *ctx,
                                  const ZnGhChange *chg);

/* =========================================================================
 * Konfigürasyon
 * ========================================================================= */

typedef struct {
    /* Divergence eşikleri */
    float divergence_reject;  /* bu üstü → reddet (varsayılan: 0.30)  */
    float divergence_warn;    /* bu üstü → uyar  (varsayılan: 0.15)  */
    float divergence_eps;     /* sayısal stabilite (varsayılan: 1e-6) */

    /* Holdout toleransı: küçük negatif kabul edilebilir */
    float holdout_min;        /* varsayılan: -0.02 (< bunu reject)    */

    /* Proxy alt sınırı */
    float proxy_min;          /* varsayılan: -0.10                     */

    /* Soft metric rotation */
    uint64_t rotation_steps;  /* kaç adımda bir (varsayılan: 1000)     */
    float    rotation_rate;   /* karışım oranı 0..1 (varsayılan: 0.25) */
    float    rotation_anneal; /* her rotasyonda azalma (varsayılan:0.95)*/

    /* Adversarial */
    bool run_all_attacks;     /* tüm stratejileri çalıştır (varsayılan: true) */
    float attack_margin_min;  /* minimum marj gereksinimi (varsayılan: 0.05)  */

    /* Divergence trend penceresi */
    uint32_t trend_window;    /* varsayılan: 20                        */

    /* PRNG başlangıcı */
    uint64_t rng_seed;
} ZnGhConfig;

/** Varsayılan config — ayarlamak istediğiniz alanları override edin */
ZnGhConfig zn_gh_default_config(void);

/* =========================================================================
 * Validation sonucu
 * ========================================================================= */

#define ZN_GH_MAX_WARNINGS 12

typedef struct {
    /* Karar */
    bool  approved;

    /* Skorlar */
    float primary_score;
    float holdout_score;
    float proxy_score;
    float divergence;

    /* Adversarial */
    bool  adversarial_passed;   /* hiçbir saldırı başarılı olmadı  */
    float worst_attack_perf;    /* en düşük saldırı performansı    */

    /* Divergence trend */
    float divergence_trend;     /* pozitif = kötüleşiyor           */

    /* Uyarılar */
    uint32_t    warnings_count;
    const char *warnings[ZN_GH_MAX_WARNINGS];
} ZnGhResult;

/* =========================================================================
 * Ana struct
 * ========================================================================= */

typedef struct {
    /* Metrikler */
    ZnGhMetric  *primary;
    size_t       primary_len;
    ZnGhMetric  *holdout;
    size_t       holdout_len;

    /* Validatorlar */
    const void     *attack_ctx;
    ZnGhAttackFn    attack_fn;
    const void     *proxy_ctx;
    ZnGhProxyEvalFn proxy_fn;

    /* Divergence history (ring, kullanıcı sağlar) */
    float    *div_hist;
    uint32_t  div_cap;
    uint32_t  div_len;
    uint32_t  div_head;

    /* Config */
    ZnGhConfig cfg;

    /* Adım sayacı */
    uint64_t steps;
    uint64_t approvals;
    uint64_t rejections;

    /* İstatistik */
    float    div_ema;         /* divergence EMA */
    float    primary_ema;
    float    holdout_ema;
} ZnGoodhart;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * Başlat — primary/holdout metrik dizilerini sahiplenMEZ (kullanıcı yönetir).
 * div_hist_buf: dışarıdan sağlanan ring buffer (en az 64 float önerilir).
 */
void zn_goodhart_init(ZnGoodhart        *g,
                      ZnGhMetric        *primary,    size_t primary_len,
                      ZnGhMetric        *holdout,    size_t holdout_len,
                      const void        *attack_ctx, ZnGhAttackFn attack_fn,
                      const void        *proxy_ctx,  ZnGhProxyEvalFn proxy_fn,
                      float             *div_hist_buf, uint32_t div_hist_cap,
                      ZnGhConfig         cfg);

/** Validasyonu çalıştır — ana entegrasyon noktası */
ZnGhResult zn_goodhart_validate(ZnGoodhart      *g,
                                 const ZnGhChange *chg,
                                 const ZnGhState  *st);

/** Soft metric rotation — zn_goodhart_validate içinden otomatik çağrılır */
void zn_goodhart_rotate_metrics(ZnGoodhart *g);

/** Divergence trend: pozitif = kötüleşiyor, negatif = düzeliyor */
float zn_goodhart_divergence_trend(const ZnGoodhart *g);

/** Raporlama */
void zn_goodhart_print_report(const ZnGoodhart *g, FILE *out);

/** Opaklık: adım istatistikleri */
uint64_t zn_goodhart_steps(const ZnGoodhart *g);
uint64_t zn_goodhart_approvals(const ZnGoodhart *g);
uint64_t zn_goodhart_rejections(const ZnGoodhart *g);
float    zn_goodhart_approval_rate(const ZnGoodhart *g);

/* =========================================================================
 * Yardımcı: basit metrik builder'ları
 * ========================================================================= */

/** Sabit değer döndüren metrik (test / placeholder) */
typedef struct { float value; } ZnGhConstCtx;
float zn_gh_const_eval(const void *ctx, const ZnGhChange *c, const ZnGhState *s);

/** Latency tabanlı metrik (ZnLatencyStats'dan beslenir) */
typedef struct {
    double *mean_ptr;    /* ZnLatencyStats.mean'e işaret eder   */
    double  target_ms;
} ZnGhLatencyCtx;
float zn_gh_latency_eval(const void *ctx, const ZnGhChange *c, const ZnGhState *s);

#ifdef __cplusplus
}
#endif

#endif /* ZN_GOODHART_H */
