/*
 * performance_stabilizer.h — Nova Deterministic Compute Controller
 *
 * Katman rolü: Execution Efficiency Layer'ın üzerinde oturur.
 * Her graph execution döngüsünü sarmalar, varyansı kısar,
 * termal drifti tespit eder ve deterministik bir performans zarfı
 * içinde kalmayı garanti eder.
 *
 * Felsefe: TCP congestion control + CPU governor + RT scheduler
 *   ✗  Peak performance chase
 *   ✓  Stable, predictable, low-variance execution
 *
 * Kullanım:
 *   ZnStabilizer *s = zn_stabilizer_create(&cfg);
 *   while (work) {
 *       zn_stabilizer_begin(s);
 *       nova_graph_execute(graph);       // mevcut Nova API
 *       zn_stabilizer_end(s);
 *   }
 *   zn_stabilizer_print_report(s);
 *   zn_stabilizer_destroy(s);
 *
 * Build: cc -O2 -std=c11 ... performance_stabilizer.c variance_analyzer.c
 *                              pacing_controller.c thermal_detector.c -lm
 */

#ifndef ZN_PERFORMANCE_STABILIZER_H
#define ZN_PERFORMANCE_STABILIZER_H

#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Version
 * ========================================================================= */
#define ZN_STABILIZER_VERSION_MAJOR 1
#define ZN_STABILIZER_VERSION_MINOR 0
#define ZN_STABILIZER_VERSION_PATCH 0

/* =========================================================================
 * Constants
 * ========================================================================= */
#define ZN_RING_DEFAULT_WINDOW    512u   /* percentile hesap penceresi        */
#define ZN_WARMUP_DEFAULT         30u    /* ısınma adımları (kontrol yok)     */
#define ZN_SPIKE_RATIO_THRESHOLD  2.0    /* worst/mean > 2x → spike           */
#define ZN_VARIANCE_EXPLOSION_K   4.0    /* stddev > K * target_ms → patlama  */
#define ZN_FREQ_PROBE_INTERVAL_MS 500.0  /* CPU frekans ölçüm periyodu        */
#define ZN_THROTTLE_RATIO         0.85   /* frekans < %85 nominal → throttle  */
#define ZN_PACE_MAX_NS            5000000ull   /* max mikro-uyku: 5ms          */
#define ZN_PACE_DAMP              0.85         /* pacing decay multiplier       */
#define ZN_EMA_ALPHA_FAST         0.20         /* hızlı drift tespiti           */
#define ZN_EMA_ALPHA_SLOW         0.05         /* yavaş baseline               */

/* =========================================================================
 * 1. Latency Stats  (variance_analyzer.h)
 * ========================================================================= */

/** Tam istatistik paketi — her zn_stabilizer_end() sonrası güncellenir. */
typedef struct {
    double mean;        /* Welford online mean (ms)          */
    double variance;    /* Welford online sample variance     */
    double stddev;      /* sqrt(variance)                     */
    double p95;         /* 95. yüzdelik (pencereden)          */
    double p99;         /* 99. yüzdelik (pencereden)          */
    double worst;       /* tüm zamanlardaki maksimum          */
    double best;        /* tüm zamanlardaki minimum           */
    double ema_fast;    /* hızlı EMA — drift dedektörü        */
    double ema_slow;    /* yavaş EMA — baseline               */
    uint64_t n;         /* toplam örnek sayısı                */
    uint64_t spikes;    /* spike sayısı (threshold aşımı)     */
} ZnLatencyStats;

/** Varyans analizörü — sliding ring üzerinde çalışır. */
typedef struct ZnVarianceAnalyzer ZnVarianceAnalyzer;

ZnVarianceAnalyzer *zna_create(uint32_t window);
void                zna_destroy(ZnVarianceAnalyzer *a);
void                zna_push(ZnVarianceAnalyzer *a, double sample_ms);
ZnLatencyStats      zna_stats(const ZnVarianceAnalyzer *a);

/** Kararsızlık tespiti — bitmask döndürür */
#define ZN_INSTABILITY_NONE       0x00
#define ZN_INSTABILITY_SPIKE      0x01   /* anlık spike */
#define ZN_INSTABILITY_VARIANCE   0x02   /* varyans patlaması */
#define ZN_INSTABILITY_DRIFT      0x04   /* ortalama sürükleniyor */
#define ZN_INSTABILITY_TAIL       0x08   /* p99 tavana çarptı */
#define ZN_INSTABILITY_THERMAL    0x10   /* termal throttle */

int zna_detect(const ZnVarianceAnalyzer *a,
               double target_ms,
               double max_p99_ms,
               double envelope_pct);

/* =========================================================================
 * 2. Thermal / Frequency Drift Detector  (thermal_detector.h)
 * ========================================================================= */

typedef struct {
    double nominal_freq_ghz;   /* ilk ölçüm baseline               */
    double current_freq_ghz;   /* son ölçüm                         */
    double freq_ratio;         /* current / nominal                  */
    bool   throttled;          /* ratio < ZN_THROTTLE_RATIO          */
    bool   turbo_oscillating;  /* hızlı frekans dalgalanması         */
    double last_probe_ns;      /* son ölçüm zamanı (ns)             */
} ZnThermalState;

/** Termal dedektörü başlat — kalibrasyon döngüsü çalıştırır */
void   zn_thermal_init(ZnThermalState *t);

/** Frekans probe — freq_ghz döndürür, state günceller */
double zn_detect_cpu_freq(ZnThermalState *t);

/** Throttle var mı? 1 = evet */
int    zn_detect_throttling(ZnThermalState *t);

/* =========================================================================
 * 3. Execution Pacing Controller  (pacing_controller.h)
 * ========================================================================= */

typedef struct {
    double   accum_ns;       /* birikmiş uyku miktarı (ns)         */
    double   damp;           /* her adımda akümülatör decay         */
    uint64_t total_slept_ns; /* toplam uyku (istatistik)            */
    uint32_t sleep_count;    /* uyku kaç kez uygulandı              */
} ZnPacingController;

void zn_pacing_init(ZnPacingController *p);

/** ns kadar monotonic uyku — EINTR güvenli */
void zn_pacing_sleep_ns(uint64_t ns);

/** Gözlemlenen latency'e göre pacing miktarını güncelle */
void zn_pacing_update(ZnPacingController *p,
                      double observed_ms,
                      double target_ms,
                      int instability_mask);

/** Birikmiş uyku uygula (varsa) */
void zn_pacing_apply(ZnPacingController *p);

/** Kararlı durumda pacing'i yavaş azalt */
void zn_pacing_relax(ZnPacingController *p);

/* =========================================================================
 * 4. Adaptive Workload Shaper (Execution Profile)
 * ========================================================================= */

/** Execution parametreleri — Stabilizer tarafından dinamik ayarlanır */
typedef struct {
    int threads;          /* paralel thread sayısı                  */
    int batch_size;       /* işlem batch büyüklüğü                  */
    int tile_size;        /* tiling parametresi                     */
    int fusion_threshold; /* fusion minimum op sayısı               */
} ZnExecutionProfile;

/** Profil sınırları */
typedef struct {
    int min_threads;      int max_threads;
    int min_batch;        int max_batch;
    int min_tile;         int max_tile;
    int min_fusion;       int max_fusion;
} ZnProfileBounds;

void zn_profile_init(ZnExecutionProfile *p, const ZnProfileBounds *b);

/** İstatistiklere göre profili otomatik ayarla */
void zn_profile_autotune(ZnExecutionProfile *p,
                         const ZnProfileBounds *b,
                         const ZnLatencyStats *s,
                         double target_ms,
                         int instability_mask);

/* =========================================================================
 * 5. Deterministic Envelope Clamp
 * ========================================================================= */

typedef struct {
    double target_ms;       /* hedef ortalama latency              */
    double envelope_pct;    /* izin verilen sapma, örn. 0.05 = ±5% */
    double max_p99_ms;      /* p99 tavanı (hard limit)             */
    double min_ms;          /* izin verilen minimum                */
    double max_ms;          /* izin verilen maksimum               */
} ZnEnvelope;

/** Envelope hesapla (target ve pct'den) */
void zn_envelope_init(ZnEnvelope *e,
                      double target_ms,
                      double envelope_pct,
                      double max_p99_ms);

/** Mevcut stats envelope içinde mi? */
int  zn_within_envelope(const ZnEnvelope *e, const ZnLatencyStats *s);

/** Dışarıdaysa execution profile'ı zorla kısalt */
void zn_enforce_stability(const ZnEnvelope *e,
                          ZnExecutionProfile *p,
                          const ZnProfileBounds *b,
                          ZnPacingController *pc,
                          const ZnLatencyStats *s,
                          int instability_mask);

/* =========================================================================
 * 6. Top-Level Stabilizer  (performance_stabilizer.h ana API)
 * ========================================================================= */

/** Stabilizer konfigürasyonu — zn_stabilizer_create'e iletilir */
typedef struct {
    /* Performans hedefi */
    double   target_ms;        /* ideal ortalama latency            */
    double   envelope_pct;     /* tolerans: 0.05 = ±5%             */
    double   max_p99_ms;       /* p99 hard ceiling                  */

    /* Analiz penceresi */
    uint32_t window;           /* ring buffer boyutu                */
    uint32_t warmup_steps;     /* ısınma adımı (kontrol yok)       */

    /* Execution profil sınırları */
    ZnProfileBounds bounds;

    /* Feature flags */
    bool enable_thermal;       /* termal dedektörü aktif            */
    bool enable_pacing;        /* pacing controller aktif           */
    bool enable_autotune;      /* profil otomatik ayar              */
    bool verbose;              /* her adımda stderr log             */
} ZnStabilizerConfig;

/** Stabilizer opak handle */
typedef struct ZnStabilizer ZnStabilizer;

/** Varsayılan config üret */
ZnStabilizerConfig zn_stabilizer_default_config(double target_ms);

/** Stabilizer oluştur */
ZnStabilizer *zn_stabilizer_create(const ZnStabilizerConfig *cfg);

/** Belleği serbest bırak */
void zn_stabilizer_destroy(ZnStabilizer *s);

/** Her execution döngüsünün BAŞINDA çağır */
void zn_stabilizer_begin(ZnStabilizer *s);

/** Her execution döngüsünün SONUNDA çağır */
void zn_stabilizer_end(ZnStabilizer *s);

/** Mevcut istatistikleri döndür */
ZnLatencyStats zn_stabilizer_stats(const ZnStabilizer *s);

/** Mevcut execution profilini döndür */
ZnExecutionProfile zn_stabilizer_profile(const ZnStabilizer *s);

/** Mevcut termal durumu döndür */
ZnThermalState zn_stabilizer_thermal(const ZnStabilizer *s);

/** Kararsızlık bitmask'ını döndür (son adım) */
int zn_stabilizer_instability(const ZnStabilizer *s);

/** Envelope içinde mi? */
int zn_stabilizer_in_envelope(const ZnStabilizer *s);

/** Detaylı raporu stderr'e bas */
void zn_stabilizer_print_report(const ZnStabilizer *s);

/** Sıfırla (stats hariç profil) — yeni iş yükü geçişi için */
void zn_stabilizer_reset_pacing(ZnStabilizer *s);

/* =========================================================================
 * 7. Platform Timing  (internal — ama dışarıya da açık)
 * ========================================================================= */

uint64_t zn_time_now_ns(void);
double   zn_time_elapsed_ms(uint64_t start_ns, uint64_t end_ns);

#ifdef __cplusplus
}
#endif

#endif /* ZN_PERFORMANCE_STABILIZER_H */
