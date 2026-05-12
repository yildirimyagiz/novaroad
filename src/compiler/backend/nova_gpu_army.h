/**
 * 🦅 nova_gpu_army.h - 4-Layer Unified Army (4LUA) Backend
 *
 * Specialized backend that orchestrates compute across four tiered systems:
 * L1: Silicon-Cell (Ultra-Low Latency Reflex)
 * L2: Kernel-OS (Stable Background Daemon)
 * L3: Web-Nexus (Massive Global Scale / Gemini)
 * L4: Mesh-Global (Resilient P2P Collective)
 */

#ifndef NOVA_GPU_ARMY_H
#define NOVA_GPU_ARMY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ARMY_TIER_L1_SILICON,
    ARMY_TIER_L2_KERNEL,
    ARMY_TIER_L3_WEB,
    ARMY_TIER_L4_MESH
} NovaArmyTier;

/**
 * Initialize the GPU Army orchestration system.
 */
int64_t nova_gpu_army_init(void);

/**
 * Shutdown and cleanup GPU Army resources.
 */
void nova_gpu_army_cleanup(void);

/**
 * Matmul specialized for the Army's tiered routing.
 */
int64_t nova_gpu_army_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n,
                             int64_t k);

/**
 * Flash Attention specialized for zero-latency reflex (L1).
 */
int64_t nova_gpu_army_flash_attention(const float *Q, const float *K, const float *V, float *Out,
                                      int L, int D);

/**
 * Universal Vector operations for the Army Mesh.
 */
int64_t nova_gpu_army_add(const float *a, const float *b, float *c, int64_t count);
int64_t nova_gpu_army_mul(const float *a, const float *b, float *c, int64_t count);

#endif // NOVA_GPU_ARMY_H
