/*
 * stabilizer_demo.c — Nova Stabilizer + Graph Entegrasyon Demosu
 *
 * Gösterir:
 *  1. Performance Stabilizer'ın Nova graph execute etrafına sarılması
 *  2. Execution profile'ın (threads, batch_size) runtime kısıtlanması
 *  3. Kasıtlı spike enjeksiyonu → stabilizer'ın müdahalesinin gözlemlenmesi
 *  4. Termal throttle simülasyonu
 *  5. Final rapor
 *
 * Build:
 *   cc -O2 -std=c11 -o stabilizer_demo stabilizer_demo.c \
 *       performance_stabilizer.c variance_analyzer.c \
 *       pacing_controller.c thermal_detector.c \
 *       nova_graph.c nova_fusion.c nova_memory.c \
 *       nova_scheduler.c nova_backend.c nova_trace.c -lm
 *
 * Çalıştır:
 *   ./stabilizer_demo
 *   ./stabilizer_demo --verbose      # adım adım log
 *   ./stabilizer_demo --spike-test   # spike enjeksiyonu açık
 */

#define _POSIX_C_SOURCE 200809L
#include "nova.h"
#include "performance_stabilizer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* =========================================================================
 * Simüle iş yükü
 *  - real_work: Nova graph execute
 *  - fake_noise: kasıtlı gecikme (spike simülasyonu)
 * ========================================================================= */

static void inject_spike_sleep_us(int us) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = (long)us * 1000L};
  nanosleep(&ts, NULL);
}

/* Basit Nova graph: matmul → bias → relu → softmax */
typedef struct {
  NovaGraph *graph;
  uint32_t t_input, t_weight, t_bias, t_out;
} DemoGraph;

static DemoGraph demo_graph_create(int M, int N, int K) {
  DemoGraph d;
  memset(&d, 0, sizeof(d));

  d.graph = nova_graph_create("demo_mlp", 32, 32);
  if (!d.graph) {
    fprintf(stderr, "graph_create fail\n");
    return d;
  }
  nova_graph_set_backend(d.graph, nova_backend_cpu());

  int64_t sh_in[2] = {M, K};
  int64_t sh_wt[2] = {K, N};
  int64_t sh_mm[2] = {M, N};
  int64_t sh_bias[1] = {N};
  int64_t sh_out[2] = {M, N};

  d.t_input = nova_tensor_add(d.graph, NOVA_DTYPE_F32, 2, sh_in, false);
  d.t_weight = nova_tensor_add(d.graph, NOVA_DTYPE_F32, 2, sh_wt, true);
  uint32_t t_mm = nova_tensor_add(d.graph, NOVA_DTYPE_F32, 2, sh_mm, false);
  d.t_bias = nova_tensor_add(d.graph, NOVA_DTYPE_F32, 1, sh_bias, true);
  uint32_t t_ba =
      nova_tensor_add(d.graph, NOVA_DTYPE_F32, 2, sh_out, false);
  uint32_t t_relu =
      nova_tensor_add(d.graph, NOVA_DTYPE_F32, 2, sh_out, false);
  d.t_out = nova_tensor_add(d.graph, NOVA_DTYPE_F32, 2, sh_out, false);

  uint32_t mm_in[2] = {d.t_input, d.t_weight};
  uint32_t mm_out[1] = {t_mm};
  nova_node_add(d.graph, NOVA_OP_MATMUL, "mm", mm_in, 2, mm_out, 1);

  uint32_t ba_in[2] = {t_mm, d.t_bias};
  uint32_t ba_out[1] = {t_ba};
  nova_node_add(d.graph, NOVA_OP_BIAS_ADD, "bias", ba_in, 2, ba_out, 1);

  uint32_t relu_in[1] = {t_ba};
  uint32_t relu_out[1] = {t_relu};
  nova_node_add(d.graph, NOVA_OP_RELU, "relu", relu_in, 1, relu_out, 1);

  uint32_t sm_in[1] = {t_relu};
  uint32_t sm_out[1] = {d.t_out};
  nova_node_add(d.graph, NOVA_OP_SOFTMAX, "softmax", sm_in, 1, sm_out, 1);

  nova_graph_optimize(d.graph);
  nova_memory_allocate(d.graph);

  /* Ağırlıkları ve girişi doldur */
  float *w = (float *)d.graph->tensors[d.t_weight].data;
  if (w)
    for (int i = 0; i < K * N; i++)
      w[i] = 0.01f * (float)(i % 100 - 50);
  float *b = (float *)d.graph->tensors[d.t_bias].data;
  if (b)
    for (int i = 0; i < N; i++)
      b[i] = 0.1f;

  return d;
}

static void demo_graph_run(DemoGraph *d, int M, int K) {
  /* Giriş tensörünü güncelle */
  float *inp = (float *)d->graph->tensors[d->t_input].data;
  if (inp) {
    for (int i = 0; i < M * K; i++)
      inp[i] = 0.01f * (float)((i + 1) % 200 - 100);
  }
  /* executed flag'lerini sıfırla */
  for (uint32_t i = 0; i < d->graph->num_nodes; i++)
    d->graph->nodes[i].executed = false;

  nova_graph_execute(d->graph);
}

static void demo_graph_destroy(DemoGraph *d) {
  if (d->graph)
    nova_graph_destroy(d->graph);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(int argc, char **argv) {
  bool verbose = false;
  bool spike_test = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0)
      verbose = true;
    if (strcmp(argv[i], "--spike-test") == 0)
      spike_test = true;
  }

  printf("\n");
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("║    Nova Deterministic Compute Controller — Demo            ║\n");
  printf("║    Stabilizer + Graph Entegrasyon Testi                      ║\n");
  printf(
      "╚══════════════════════════════════════════════════════════════╝\n\n");

  /* ----------------------------------------------------------------
   * Nova graph kur: 64x128 × 128x64 matmul zinciri
   * ---------------------------------------------------------------- */
  int M = 64, K = 128, N = 64;
  DemoGraph dg = demo_graph_create(M, N, K);
  if (!dg.graph)
    return 1;

  printf("[demo] Graph hazır: matmul(%dx%d) → bias → relu → softmax\n", M, K);
  printf("[demo] Fusions: %u, live nodes: %u\n\n", dg.graph->fusions_applied,
         dg.graph->topo_len);

  /* ----------------------------------------------------------------
   * Stabilizer konfigürasyonu
   * ---------------------------------------------------------------- */
  ZnStabilizerConfig cfg = zn_stabilizer_default_config(2.0 /* ms hedef */);
  cfg.envelope_pct = 0.05; /* ±5% */
  cfg.max_p99_ms = 5.0;
  cfg.window = 256;
  cfg.warmup_steps = 20;
  cfg.verbose = verbose;
  cfg.enable_thermal = true;
  cfg.enable_pacing = true;
  cfg.enable_autotune = true;

  cfg.bounds.min_threads = 1;
  cfg.bounds.max_threads = 8;
  cfg.bounds.min_batch = 1;
  cfg.bounds.max_batch = 64;
  cfg.bounds.min_tile = 16;
  cfg.bounds.max_tile = 256;
  cfg.bounds.min_fusion = 2;
  cfg.bounds.max_fusion = 8;

  ZnStabilizer *stab = zn_stabilizer_create(&cfg);
  if (!stab) {
    fprintf(stderr, "stabilizer_create fail\n");
    return 1;
  }

  printf("[demo] Stabilizer başlatıldı. Hedef: %.1f ms ±%.0f%%\n\n",
         cfg.target_ms, cfg.envelope_pct * 100.0);

  /* ----------------------------------------------------------------
   * Ana döngü: 400 adım
   *  - İlk 100: normal
   *  - 100-150: kasıtlı spike enjeksiyonu (spike_test modunda)
   *  - 150-250: termal yavaşlama simülasyonu (spike_test)
   *  - 250-400: toparlanma
   * ---------------------------------------------------------------- */
  int total_steps = 400;
  printf("  %-6s  %-8s  %-8s  %-8s  %-8s  %-6s  %-8s  %s\n", "Adım", "Lat(ms)",
         "Mean", "StdDev", "P99", "Thr", "Envelope", "Durum");
  printf("  %-6s  %-8s  %-8s  %-8s  %-8s  %-6s  %-8s  %s\n", "------",
         "--------", "--------", "--------", "--------", "------", "--------",
         "------");

  for (int i = 0; i < total_steps; i++) {

    /* Stabilizer ölçüm başlangıcı */
    zn_stabilizer_begin(stab);

    /* Nova graph çalıştır */
    demo_graph_run(&dg, M, K);

    /* Spike enjeksiyonu (test modu) */
    if (spike_test && i >= 100 && i < 150) {
      /* 50 adım boyunca 3ms ekstra gecikme */
      inject_spike_sleep_us(3000);
    }

    /* Termal yavaşlama simülasyonu */
    if (spike_test && i >= 150 && i < 250) {
      /* Daha fazla iş simüle et — artan iterasyon */
      volatile double acc = 0.0;
      for (int j = 0; j < 500000; j++)
        acc += sqrt((double)j + 1.0);
      (void)acc;
    }

    /* Stabilizer ölçüm sonu (kontrol döngüsü burada çalışır) */
    zn_stabilizer_end(stab);

    /* Rapor satırı — her 25 adımda */
    if (i > 0 && (i % 25) == 0) {
      ZnLatencyStats st = zn_stabilizer_stats(stab);
      ZnExecutionProfile ep = zn_stabilizer_profile(stab);
      int in_env = zn_stabilizer_in_envelope(stab);
      int inst = zn_stabilizer_instability(stab);

      char status[48] = "";
      if (inst & ZN_INSTABILITY_SPIKE)
        strcat(status, "⚡SPIKE ");
      if (inst & ZN_INSTABILITY_TAIL)
        strcat(status, "📊TAIL ");
      if (inst & ZN_INSTABILITY_DRIFT)
        strcat(status, "📈DRIFT ");
      if (inst & ZN_INSTABILITY_THERMAL)
        strcat(status, "🌡THERM ");
      if (inst & ZN_INSTABILITY_VARIANCE)
        strcat(status, "📉VAR ");
      if (inst == ZN_INSTABILITY_NONE)
        strcat(status, "✓ STABLE");

      printf("  %-6d  %-8.3f  %-8.3f  %-8.4f  %-8.3f  %-6d  %-8s  %s\n", i,
             st.ema_fast, /* son EMA: "anlık his" */
             st.mean, st.stddev, st.p99, ep.threads, in_env ? "✓ İÇ" : "✗ DIŞ",
             status);
    }
  }

  /* ----------------------------------------------------------------
   * Final rapor
   * ---------------------------------------------------------------- */
  printf("\n");
  zn_stabilizer_print_report(stab);

  /* Kontrol kalitesi özeti */
  ZnLatencyStats final_st = zn_stabilizer_stats(stab);
  double cov =
      (final_st.mean > 0.0) ? (final_st.stddev / final_st.mean * 100.0) : 0.0;
  printf("  Coefficient of Variation : %.2f%%  (hedef < 5%%)\n", cov);
  printf("  Spike oranı              : %.2f%%  (%llu / %llu)\n",
         final_st.n > 0 ? (100.0 * (double)final_st.spikes / (double)final_st.n)
                        : 0.0,
         (unsigned long long)final_st.spikes, (unsigned long long)final_st.n);
  printf("\n");

  zn_stabilizer_destroy(stab);
  demo_graph_destroy(&dg);
  return 0;
}
