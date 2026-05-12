/*
 * goodhart_demo.c — Nova Goodhart Guard + Stabilizer + Graph Entegrasyon
 *
 * Senaryo: Bir model optimizasyon döngüsünü simüle ederiz.
 *
 *   Faz 1 (adım   0–99):  Gerçek iyileşme — onaylanır
 *   Faz 2 (adım 100–199): Metric gaming — primary↑, holdout↓ — reddedilir
 *   Faz 3 (adım 200–299): Adversarial açık — edge-case başarısız — reddedilir
 *   Faz 4 (adım 300–399): Proxy düşüşü — gerçek dünya kötüleşiyor — reddedilir
 *   Faz 5 (adım 400–499): Toparlanma — sistem dengelenir
 *
 * Gösterilen:
 *  ✓ 6 koruma katmanının birbirini tamamlaması
 *  ✓ Soft rotation: primary metrik ağırlıklarının kademeli değişimi
 *  ✓ Divergence trend dedektörü gaming'i erken yakalaması
 *  ✓ Stabilizer + Goodhart'ın aynı döngüde çalışması
 *
 * Build:
 *   cc -O2 -std=c11 -o goodhart_demo goodhart_demo.c \
 *       zn_goodhart.c \
 *       performance_stabilizer.c variance_analyzer.c \
 *       pacing_controller.c thermal_detector.c \
 *       nova_graph.c nova_fusion.c nova_memory.c \
 *       nova_scheduler.c nova_backend.c nova_trace.c -lm
 */

#define _POSIX_C_SOURCE 200809L
#include "nova.h"
#include "performance_stabilizer.h"
#include "zn_goodhart.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Simülasyon durumu
 * ========================================================================= */

typedef struct {
  int phase;            /* 1–5 */
  float model_quality;  /* gerçek kalite (gizli — oracle) */
  float metric_bias;    /* gaming faz'ında enjekte edilen sapma */
  int adversarial_open; /* açık güvenlik açığı var mı */
  float proxy_health;   /* gerçek dünya sağlığı */
} SimState;

static SimState g_sim;

/* =========================================================================
 * Metrik implementasyonları
 * ========================================================================= */

/* ---- PRIMARY metrikler (optimizer görür) ---- */

/* Doğruluk simülasyonu */
float metric_accuracy(const void *ctx, const ZnGhChange *c,
                      const ZnGhState *s) {
  (void)ctx;
  (void)c;
  (void)s;
  float base = g_sim.model_quality;
  /* Gaming fazında: gerçek kaliteden bağımsız olarak yüksek görünür */
  return base + g_sim.metric_bias * 0.6f;
}

/* Hız / throughput simülasyonu */
float metric_throughput(const void *ctx, const ZnGhChange *c,
                        const ZnGhState *s) {
  (void)ctx;
  (void)c;
  (void)s;
  return g_sim.model_quality * 0.9f + g_sim.metric_bias * 0.4f;
}

/* ---- HOLDOUT metrikler (optimizer GÖRMEZ) ---- */

/* OOD generalization — gaming sırasında bozulur */
float metric_ood(const void *ctx, const ZnGhChange *c, const ZnGhState *s) {
  (void)ctx;
  (void)c;
  (void)s;
  /* Gaming fazında holdout düşer: bias primary'yi şişirir ama OOD bozulur */
  return g_sim.model_quality * 1.1f - g_sim.metric_bias * 0.8f;
}

/* Rare case performance */
float metric_rare(const void *ctx, const ZnGhChange *c, const ZnGhState *s) {
  (void)ctx;
  (void)c;
  (void)s;
  return g_sim.model_quality - g_sim.metric_bias * 0.5f;
}

/* Robustness score */
float metric_robustness(const void *ctx, const ZnGhChange *c,
                        const ZnGhState *s) {
  (void)ctx;
  (void)c;
  (void)s;
  return g_sim.model_quality * 0.95f - g_sim.metric_bias * 0.7f;
}

/* =========================================================================
 * Adversarial saldırılar
 * ========================================================================= */

static ZnGhAttackResult sim_attack(const void *ctx, ZnGhAttackStrategy strat,
                                   const ZnGhChange *chg) {
  (void)ctx;
  (void)chg;
  ZnGhAttackResult r;
  memset(&r, 0, sizeof(r));
  r.performance = g_sim.model_quality;

  switch (strat) {
  case ZN_ATTACK_EDGE_CASES:
  case ZN_ATTACK_WORST_CASE:
    if (g_sim.adversarial_open) {
      r.successful = true;
      r.performance = g_sim.model_quality * 0.3f;
      r.margin = 0.02f;
      r.vulnerability =
          (strat == ZN_ATTACK_EDGE_CASES)
              ? "Edge-case collapse: long-tail inputs fail"
              : "Worst-case: numerical instability on extreme values";
    } else {
      r.performance = g_sim.model_quality * 0.85f;
      r.margin = 0.15f;
    }
    break;

  case ZN_ATTACK_NUMERICAL_STRESS:
    if (g_sim.adversarial_open) {
      r.successful = true;
      r.performance = 0.1f;
      r.margin = 0.01f;
      r.vulnerability = "NaN propagation under extreme inputs";
    } else {
      r.performance = 0.7f;
    }
    break;

  default:
    r.performance = g_sim.model_quality;
    r.margin = 0.2f;
    break;
  }
  return r;
}

/* =========================================================================
 * Gerçek dünya proxy
 * ========================================================================= */

static float sim_proxy(const void *ctx, const ZnGhChange *chg) {
  (void)ctx;
  (void)chg;
  return g_sim.proxy_health;
}

/* =========================================================================
 * Simülasyon faz geçişi
 * ========================================================================= */

static void update_sim(int step) {
  if (step < 100) {
    /* Faz 1: gerçek iyileşme */
    g_sim.phase = 1;
    g_sim.model_quality = 0.3f + step * 0.003f; /* 0.3 → 0.6 */
    g_sim.metric_bias = 0.f;
    g_sim.adversarial_open = 0;
    g_sim.proxy_health = g_sim.model_quality;
  } else if (step < 200) {
    /* Faz 2: metric gaming başlıyor */
    g_sim.phase = 2;
    g_sim.model_quality = 0.55f;               /* gerçek kalite sabit */
    g_sim.metric_bias = (step - 100) * 0.005f; /* 0 → 0.5 */
    g_sim.adversarial_open = 0;
    g_sim.proxy_health = 0.5f;
  } else if (step < 300) {
    /* Faz 3: adversarial açık */
    g_sim.phase = 3;
    g_sim.model_quality = 0.5f;
    g_sim.metric_bias = 0.3f;
    g_sim.adversarial_open = 1;
    g_sim.proxy_health = 0.4f;
  } else if (step < 400) {
    /* Faz 4: proxy çöküşü */
    g_sim.phase = 4;
    g_sim.model_quality = 0.6f;
    g_sim.metric_bias = 0.2f;
    g_sim.adversarial_open = 0;
    g_sim.proxy_health = -0.3f; /* gerçek dünya kötüleşiyor */
  } else {
    /* Faz 5: toparlanma */
    g_sim.phase = 5;
    g_sim.model_quality = 0.6f + (step - 400) * 0.001f;
    g_sim.metric_bias = g_sim.metric_bias * 0.95f; /* yavaş azalıyor */
    g_sim.adversarial_open = 0;
    g_sim.proxy_health = 0.5f + (step - 400) * 0.001f;
  }
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void) {
  printf("\n");
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("║    Nova Goodhart Guard — 5-Faz Entegrasyon Demosu          ║\n");
  printf("║    Gaming / Adversarial / Proxy / Rotation Tespiti           ║\n");
  printf(
      "╚══════════════════════════════════════════════════════════════╝\n\n");

  /* ----------------------------------------------------------------
   * Metrik dizileri
   * ---------------------------------------------------------------- */
  ZnGhMetric primary[2] = {
      {.name = "accuracy", .weight = 0.6f, .eval = metric_accuracy},
      {.name = "throughput", .weight = 0.4f, .eval = metric_throughput},
  };
  ZnGhMetric holdout[3] = {
      {.name = "ood_gen", .weight = 0.4f, .eval = metric_ood},
      {.name = "rare_case", .weight = 0.3f, .eval = metric_rare},
      {.name = "robustness", .weight = 0.3f, .eval = metric_robustness},
  };

  /* Divergence history buffer */
  float div_buf[256];
  memset(div_buf, 0, sizeof(div_buf));

  /* ----------------------------------------------------------------
   * Goodhart Guard init
   * ---------------------------------------------------------------- */
  ZnGhConfig cfg = zn_gh_default_config();
  cfg.divergence_reject = 0.30f;
  cfg.divergence_warn = 0.15f;
  cfg.holdout_min = -0.02f; /* [FIX-2]: soft threshold */
  cfg.proxy_min = -0.10f;
  cfg.rotation_steps = 100u; /* sık rotation demo için */
  cfg.rotation_rate = 0.20f;
  cfg.rotation_anneal = 0.92f;
  cfg.run_all_attacks = true;
  cfg.trend_window = 20u;

  ZnGoodhart gh;
  zn_goodhart_init(&gh, primary, 2, holdout, 3, NULL, sim_attack, NULL,
                   sim_proxy, div_buf, 256, cfg);

  /* ----------------------------------------------------------------
   * Stabilizer (Nova runtime koruması)
   * ---------------------------------------------------------------- */
  ZnStabilizerConfig sc = zn_stabilizer_default_config(1.0);
  sc.verbose = false;
  ZnStabilizer *stab = zn_stabilizer_create(&sc);

  /* ----------------------------------------------------------------
   * Basit Nova graph (computation ölçümü için)
   * ---------------------------------------------------------------- */
  NovaGraph *graph = nova_graph_create("goodhart_compute", 8, 8);
  nova_graph_set_backend(graph, nova_backend_cpu());
  {
    int64_t sh[2] = {32, 32};
    uint32_t tA = nova_tensor_add(graph, NOVA_DTYPE_F32, 2, sh, true);
    uint32_t tB = nova_tensor_add(graph, NOVA_DTYPE_F32, 2, sh, true);
    uint32_t tC = nova_tensor_add(graph, NOVA_DTYPE_F32, 2, sh, false);
    uint32_t ins[2] = {tA, tB};
    uint32_t outs[1] = {tC};
    nova_node_add(graph, NOVA_OP_MATMUL, "mm", ins, 2, outs, 1);
    nova_graph_optimize(graph);
    nova_memory_allocate(graph);
    float *a = (float *)graph->tensors[tA].data;
    if (a)
      for (int i = 0; i < 32 * 32; i++)
        a[i] = 0.01f * (float)(i % 100);
    float *b = (float *)graph->tensors[tB].data;
    if (b)
      for (int i = 0; i < 32 * 32; i++)
        b[i] = 0.02f;
  }

  /* ----------------------------------------------------------------
   * Başlık
   * ---------------------------------------------------------------- */
  printf("  %-5s  %-6s  %-8s  %-8s  %-8s  %-6s  %-6s  %-8s  %s\n", "Adım",
         "Faz", "Primary", "Holdout", "Diverg.", "Onay", "Advers", "Proxy",
         "Uyarılar");
  printf("  %s\n", "─────  ──────  ────────  ────────  ────────  ──────  "
                   "──────  ────────  ────────────────────────────");

  int total_approved = 0;
  int total_rejected = 0;
  int last_phase_shown = 0;

  for (int step = 0; step < 500; step++) {

    /* Simülasyon güncelle */
    update_sim(step);

    /* Faz başlığı */
    static const char *faz_labels[] = {"",
                                       "Gerçek iyileşme",
                                       "Metric gaming",
                                       "Adversarial açık",
                                       "Proxy çöküşü",
                                       "Toparlanma"};
    if (g_sim.phase != last_phase_shown) {
      printf("\n  ── FAZ %d: %s ──\n", g_sim.phase, faz_labels[g_sim.phase]);
      last_phase_shown = g_sim.phase;
    }

    /* Stabilizer: compute döngüsü başlat */
    zn_stabilizer_begin(stab);

    for (uint32_t i = 0; i < graph->num_nodes; i++)
      graph->nodes[i].executed = false;
    nova_graph_execute(graph);

    zn_stabilizer_end(stab);

    /* Goodhart validasyonu */
    ZnGhChange chg = {
        .label = "optim_step",
        .data = NULL,
        .data_size = 0,
        .step = (uint64_t)step,
    };
    ZnGhState st = {
        .data = NULL,
        .data_size = 0,
        .timestamp_ns = zn_time_now_ns(),
    };

    ZnGhResult res = zn_goodhart_validate(&gh, &chg, &st);

    if (res.approved)
      total_approved++;
    else
      total_rejected++;

    /* Her 25 adımda bir log */
    if (step % 25 == 0) {
      char warn_summary[64] = "—";
      if (res.warnings_count > 0) {
        /* İlk uyarının kısa özetini al */
        strncpy(warn_summary, res.warnings[0], 40);
        warn_summary[40] = '\0';
        if (res.warnings_count > 1)
          strcat(warn_summary, "…");
      }

      printf("  %-5d  %-6d  %-8.3f  %-8.3f  %-8.4f  %-6s  %-6s  %-8.3f  %s\n",
             step, g_sim.phase, res.primary_score, res.holdout_score,
             res.divergence, res.approved ? "✓ ONAY" : "✗ RED",
             res.adversarial_passed ? "✓" : "✗ AÇIK", res.proxy_score,
             warn_summary);
    }
  }

  /* ----------------------------------------------------------------
   * Özet
   * ---------------------------------------------------------------- */
  printf("\n");
  printf("  ┌─────────────────────────────────────────────────┐\n");
  printf("  │  Toplam:  %3d onay  |  %3d red  |  oran: %.1f%%   │\n",
         total_approved, total_rejected,
         500 > 0 ? 100.f * total_approved / 500.f : 0.f);
  printf("  └─────────────────────────────────────────────────┘\n");

  /* Goodhart raporu */
  zn_goodhart_print_report(&gh, stderr);

  /* Stabilizer raporu */
  zn_stabilizer_print_report(stab);

  /* Temizlik */
  zn_stabilizer_destroy(stab);
  nova_graph_destroy(graph);

  printf("  Tüm katmanlar doğrulama tamamlandı.\n\n");
  return 0;
}
