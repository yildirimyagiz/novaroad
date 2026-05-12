/*
 * thermal_detector.c — Termal / Frekans Drift Dedektörü
 *
 * CPU frekansını doğrudan /proc/cpuinfo veya platform API'si olmadan ölçer:
 * Sabit iterasyon sayısında döngü çalıştırır, geçen süreyi ölçer,
 * iterasyon/ns oranından frekans proxy değeri türetir.
 *
 * Strateji:
 *  1. İlk çağrıda kalibrasyon: N iterasyon / T ns = nominal_freq_proxy
 *  2. Sonraki çağrılarda: aynı döngü, oran değişimi = frekans değişimi
 *  3. Oran < THROTTLE_RATIO → throttling var demek
 *  4. Turbo dalgalanması: son K ölçümde yüksek varyans
 *
 * Not: Bu bir spektrometre değil, relative change detector'dür.
 * Mutlak GHz vermez — göreceli sapma yeterlidir kontrol için.
 *
 * Linux'ta isteğe bağlı: /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
 * okunur (root gerektirmez, sadece cpufreq driver gerektirir).
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
 * Platform: nanosaniye zaman damgası
 * ========================================================================= */

static uint64_t thermal_now_ns(void)
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

/* =========================================================================
 * Linux: /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
 * (kHz döndürür, mevcut değilse -1.0 döner)
 * ========================================================================= */

#if defined(__linux__)
static double read_sysfs_freq_ghz(void)
{
    static const char *paths[] = {
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        FILE *f = fopen(paths[i], "r");
        if (!f) continue;
        unsigned long khz = 0;
        int ok = fscanf(f, "%lu", &khz);
        fclose(f);
        if (ok == 1 && khz > 0)
            return (double)khz / 1e6;  /* kHz → GHz */
    }
    return -1.0;  /* mevcut değil */
}
#else
static double read_sysfs_freq_ghz(void) { return -1.0; }
#endif

/* =========================================================================
 * Kalibrasyon döngüsü: deterministik, optimize-edilemez volatile trick
 * Döngü sayısı / geçen süre = "freq proxy" (ns^-1 cinsinden)
 * ========================================================================= */

#define CALIB_ITERS 2000000u

static double measure_freq_proxy(void)
{
    volatile double acc = 1.0;
    uint64_t t0 = thermal_now_ns();
    for (uint32_t i = 0; i < CALIB_ITERS; i++)
        acc += (double)(i & 0xFFu) * 0.000001;
    uint64_t t1 = thermal_now_ns();
    (void)acc;

    uint64_t dt = (t1 > t0) ? (t1 - t0) : 1;
    return (double)CALIB_ITERS / (double)dt;  /* iterasyon/ns */
}

/* =========================================================================
 * Turbo dalgalanması: son K ölçümde coefficent of variation yüksek mi?
 * ========================================================================= */

#define TURBO_HIST_LEN 8

typedef struct {
    double  hist[TURBO_HIST_LEN];
    int     idx;
    int     filled;
} TurboHistory;

static void turbo_push(TurboHistory *h, double v)
{
    h->hist[h->idx] = v;
    h->idx = (h->idx + 1) % TURBO_HIST_LEN;
    if (!h->filled && h->idx == 0) h->filled = 1;
}

static double turbo_cv(const TurboHistory *h)
{
    /* Coefficient of Variation = stddev / mean */
    int n = h->filled ? TURBO_HIST_LEN : h->idx;
    if (n < 2) return 0.0;
    double mean = 0.0;
    for (int i = 0; i < n; i++) mean += h->hist[i];
    mean /= n;
    if (mean <= 0.0) return 0.0;
    double var = 0.0;
    for (int i = 0; i < n; i++) {
        double d = h->hist[i] - mean;
        var += d * d;
    }
    var /= (double)(n - 1);
    return sqrt(var) / mean;
}

/* =========================================================================
 * Kalıcı state (per-detector handle içinde saklanır)
 * ========================================================================= */

typedef struct {
    double       nominal_proxy;   /* kalibrasyon değeri          */
    TurboHistory turbo;
    int          calibrated;
    double       last_probe_ns;
} ThermalInternal;

static ThermalInternal g_thermal_internal = {0};

/* =========================================================================
 * Public API
 * ========================================================================= */

void zn_thermal_init(ZnThermalState *t)
{
    memset(t, 0, sizeof(*t));
    memset(&g_thermal_internal, 0, sizeof(g_thermal_internal));

    /* Kalibrasyon: 3 ölçüm al, medyanı al */
    double samples[3];
    for (int i = 0; i < 3; i++)
        samples[i] = measure_freq_proxy();
    /* basit 3-sort */
    for (int i = 0; i < 2; i++)
        for (int j = i+1; j < 3; j++)
            if (samples[j] < samples[i]) {
                double tmp = samples[i]; samples[i] = samples[j]; samples[j] = tmp;
            }
    g_thermal_internal.nominal_proxy = samples[1];  /* medyan */
    g_thermal_internal.calibrated = 1;
    g_thermal_internal.last_probe_ns = (double)thermal_now_ns();

    /* sysfs varsa gerçek GHz'i dene */
    double sysfs = read_sysfs_freq_ghz();
    t->nominal_freq_ghz  = (sysfs > 0.0) ? sysfs : 1.0;  /* 1.0 = proxy normalized */
    t->current_freq_ghz  = t->nominal_freq_ghz;
    t->freq_ratio        = 1.0;
    t->throttled         = false;
    t->turbo_oscillating = false;
    t->last_probe_ns     = g_thermal_internal.last_probe_ns;
}

double zn_detect_cpu_freq(ZnThermalState *t)
{
    if (!g_thermal_internal.calibrated) {
        zn_thermal_init(t);
    }

    double now_ns = (double)thermal_now_ns();
    double elapsed_ms = (now_ns - t->last_probe_ns) / 1e6;

    /* Çok sık probe etme — ZN_FREQ_PROBE_INTERVAL_MS periyodu koru */
    if (elapsed_ms < ZN_FREQ_PROBE_INTERVAL_MS && t->last_probe_ns > 0.0)
        return t->current_freq_ghz;

    /* sysfs varsa tercih et (daha doğru) */
    double sysfs = read_sysfs_freq_ghz();
    if (sysfs > 0.0) {
        t->current_freq_ghz = sysfs;
        t->freq_ratio = t->nominal_freq_ghz > 0.0
                        ? (sysfs / t->nominal_freq_ghz) : 1.0;
    } else {
        /* Proxy ölçümü */
        double cur_proxy = measure_freq_proxy();
        t->freq_ratio    = (g_thermal_internal.nominal_proxy > 0.0)
                           ? (cur_proxy / g_thermal_internal.nominal_proxy) : 1.0;
        t->current_freq_ghz = t->nominal_freq_ghz * t->freq_ratio;
    }

    /* Turbo dalgalanması dedektörü */
    turbo_push(&g_thermal_internal.turbo, t->freq_ratio);
    double cv = turbo_cv(&g_thermal_internal.turbo);
    t->turbo_oscillating = (cv > 0.04);  /* %4 CV üstü = dalgalanıyor */

    /* Throttle bayrağı */
    t->throttled = (t->freq_ratio < ZN_THROTTLE_RATIO);
    t->last_probe_ns = now_ns;

    return t->current_freq_ghz;
}

int zn_detect_throttling(ZnThermalState *t)
{
    if (!g_thermal_internal.calibrated) zn_thermal_init(t);
    zn_detect_cpu_freq(t);  /* state'i güncelle */
    return t->throttled ? 1 : 0;
}
