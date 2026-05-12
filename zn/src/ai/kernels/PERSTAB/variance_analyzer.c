/*
 * variance_analyzer.c — Varyans & Jitter Analizörü
 *
 * Sorumluluklar:
 *  - Welford algoritması: online mean + variance (numerically stable)
 *  - Ring buffer: son N örnek, percentile hesabı için
 *  - Quickselect: O(n) medyan / P95 / P99 (sort değil)
 *  - EMA çifti: hızlı drift + yavaş baseline
 *  - Kararsızlık dedektörü: spike / varyans / drift / tail bitmask
 *
 * Tasarım kararları:
 *  - Ring buffer kopyalanarak quickselect'e girer → orijinal sıra bozulmaz
 *  - Welford: n=1 tek örnek için bile doğru çalışır
 *  - Percentile hesabı warmup öncesi çağrılırsa 0.0 döner (güvenli)
 */

#define _POSIX_C_SOURCE 200809L
#include "performance_stabilizer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================================
 * İç yapı
 * ========================================================================= */

struct ZnVarianceAnalyzer {
    /* Ring buffer */
    double   *ring;
    uint32_t  cap;       /* buffer kapasitesi                 */
    uint32_t  len;       /* mevcut dolu slot sayısı           */
    uint32_t  head;      /* sonraki yazılacak pozisyon        */

    /* Welford state */
    double mean;
    double m2;           /* varyans hesabı için M2 akümülatörü */
    uint64_t n;

    /* EMA çifti */
    double ema_fast;
    double ema_slow;

    /* Extremes */
    double worst;
    double best;

    /* Spike sayacı */
    uint64_t spikes;

    /* Önceki percentile (son hesaplanan) */
    double p95;
    double p99;
};

/* =========================================================================
 * Quickselect — Hoare partition, in-place, O(n) ortalama
 * Girdi: a[left..right] üzerinde k. en küçük elemanı bul
 * ========================================================================= */

static double qs_select(double *a, int left, int right, int k)
{
    while (left < right) {
        /* Pivot: üç değerin medyanı (daha iyi worst-case) */
        int mid = left + (right - left) / 2;
        if (a[mid] < a[left])  { double t = a[mid];  a[mid]  = a[left];  a[left]  = t; }
        if (a[right] < a[left]){ double t = a[right]; a[right]= a[left];  a[left]  = t; }
        if (a[right] < a[mid]) { double t = a[right]; a[right]= a[mid];   a[mid]   = t; }
        double pivot = a[mid];

        /* Partition */
        int i = left, j = right;
        while (i <= j) {
            while (a[i] < pivot) i++;
            while (a[j] > pivot) j--;
            if (i <= j) {
                double t = a[i]; a[i] = a[j]; a[j] = t;
                i++; j--;
            }
        }
        if (k <= j)      right = j;
        else if (k >= i) left  = i;
        else             return a[k];
    }
    return a[k];
}

static double ring_percentile(const struct ZnVarianceAnalyzer *a, double pct)
{
    if (a->len == 0) return 0.0;

    uint32_t n = a->len;
    double *tmp = malloc(n * sizeof(double));
    if (!tmp) return 0.0;

    /* Ring'den kopyala — sıra önemli değil, percentile sadece değerlere bakar */
    for (uint32_t i = 0; i < n; i++)
        tmp[i] = a->ring[i];   /* ring dolu olmayabilir; dolu kısımdan kopyala */

    /* nearest-rank: k = floor(pct * (n-1) + 0.5) */
    int k = (int)floor(pct * (double)(n - 1) + 0.5);
    if (k < 0) k = 0;
    if ((uint32_t)k >= n) k = (int)n - 1;

    double v = qs_select(tmp, 0, (int)n - 1, k);
    free(tmp);
    return v;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

ZnVarianceAnalyzer *zna_create(uint32_t window)
{
    if (window < 8) window = 8;

    struct ZnVarianceAnalyzer *a = calloc(1, sizeof(*a));
    if (!a) return NULL;

    a->ring = calloc(window, sizeof(double));
    if (!a->ring) { free(a); return NULL; }

    a->cap  = window;
    a->best = 1e300;
    return a;
}

void zna_destroy(ZnVarianceAnalyzer *a)
{
    if (!a) return;
    free(a->ring);
    free(a);
}

void zna_push(ZnVarianceAnalyzer *a, double sample_ms)
{
    if (!a) return;

    /* Ring push */
    a->ring[a->head] = sample_ms;
    a->head = (a->head + 1u) % a->cap;
    if (a->len < a->cap) a->len++;

    /* Welford online update */
    a->n++;
    double delta  = sample_ms - a->mean;
    a->mean      += delta / (double)a->n;
    double delta2 = sample_ms - a->mean;
    a->m2        += delta * delta2;

    /* Extremes */
    if (sample_ms > a->worst) a->worst = sample_ms;
    if (sample_ms < a->best)  a->best  = sample_ms;

    /* EMA update */
    if (a->n == 1) {
        a->ema_fast = sample_ms;
        a->ema_slow = sample_ms;
    } else {
        a->ema_fast = ZN_EMA_ALPHA_FAST * sample_ms + (1.0 - ZN_EMA_ALPHA_FAST) * a->ema_fast;
        a->ema_slow = ZN_EMA_ALPHA_SLOW * sample_ms + (1.0 - ZN_EMA_ALPHA_SLOW) * a->ema_slow;
    }

    /* Percentile — her push'ta güncelle (ring doluyken anlamlı) */
    if (a->len >= 8) {
        a->p95 = ring_percentile(a, 0.95);
        a->p99 = ring_percentile(a, 0.99);
    }

    /* Spike sayacı: ani sıçrama tespiti */
    if (a->n > 1 && a->mean > 0.0) {
        double spike_ratio = sample_ms / a->mean;
        if (spike_ratio > ZN_SPIKE_RATIO_THRESHOLD)
            a->spikes++;
    }
}

ZnLatencyStats zna_stats(const ZnVarianceAnalyzer *a)
{
    ZnLatencyStats s;
    memset(&s, 0, sizeof(s));
    if (!a) return s;

    s.mean     = a->mean;
    s.variance = (a->n > 1) ? (a->m2 / (double)(a->n - 1)) : 0.0;
    s.stddev   = sqrt(s.variance);
    s.p95      = a->p95;
    s.p99      = a->p99;
    s.worst    = a->worst;
    s.best     = (a->best > 1e200) ? 0.0 : a->best;
    s.ema_fast = a->ema_fast;
    s.ema_slow = a->ema_slow;
    s.n        = a->n;
    s.spikes   = a->spikes;
    return s;
}

int zna_detect(const ZnVarianceAnalyzer *a,
               double target_ms,
               double max_p99_ms,
               double envelope_pct)
{
    if (!a || a->n < 4) return ZN_INSTABILITY_NONE;

    int mask = ZN_INSTABILITY_NONE;

    double mean = a->mean;
    double std  = (a->n > 1) ? sqrt(a->m2 / (double)(a->n - 1)) : 0.0;

    /* 1. Spike: son örnek mean'in SPIKE_RATIO katından büyük mü?
     *    (ring'deki son değer = head-1 ile alınır) */
    uint32_t last_idx = (a->head == 0) ? (a->cap - 1) : (a->head - 1);
    if (a->len > 0) {
        double last = a->ring[last_idx];
        if (mean > 0.0 && (last / mean) > ZN_SPIKE_RATIO_THRESHOLD)
            mask |= ZN_INSTABILITY_SPIKE;
    }

    /* 2. Varyans patlaması: stddev > K * target_ms */
    if (std > ZN_VARIANCE_EXPLOSION_K * target_ms)
        mask |= ZN_INSTABILITY_VARIANCE;

    /* 3. Drift: ema_fast ile ema_slow arası açılıyor mu?
     *    |fast - slow| > envelope_pct * target_ms */
    double drift_thr = envelope_pct * target_ms;
    if (fabs(a->ema_fast - a->ema_slow) > drift_thr)
        mask |= ZN_INSTABILITY_DRIFT;

    /* 4. Tail: p99 tavan üstünde mi? */
    if (a->p99 > max_p99_ms && a->len >= 8)
        mask |= ZN_INSTABILITY_TAIL;

    return mask;
}
