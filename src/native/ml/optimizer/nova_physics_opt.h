#ifndef NOVA_PHYSICS_OPT_H
#define NOVA_PHYSICS_OPT_H

#include <stdint.h>
#include <stdlib.h>

typedef float (*NovaObjectiveFn)(const float *x, int n_dims, void *ctx);

/* ============================================================================
   SIMULATED ANNEALING
   ============================================================================ */

typedef struct {
    float initial_temp;
    float cooling_rate;
    float min_temp;
    int max_iter;
    int n_dims;
} NovaSAOpts;

float* nova_simulated_annealing(const NovaSAOpts *opts,
                                 NovaObjectiveFn objective_fn,
                                 void *ctx,
                                 const float *x0);

float nova_sa_accept_prob(float delta_e, float temperature);

/* ============================================================================
   PARTICLE SWARM OPTIMIZATION (PSO)
   ============================================================================ */

typedef struct {
    float *position;
    float *velocity;
    float *personal_best_pos;
    float personal_best_val;
    int n_dims;
} NovaParticle;

typedef struct {
    int n_particles;
    int n_dims;
    int max_iter;
    float w;
    float c1;
    float c2;
    float x_min;
    float x_max;
} NovaPSOOpts;

float* nova_pso_optimize(const NovaPSOOpts *opts,
                         NovaObjectiveFn objective_fn,
                         void *ctx);

void nova_pso_free(float *result);

/* ============================================================================
   GRAVITATIONAL SEARCH ALGORITHM (GSA)
   ============================================================================ */

typedef struct {
    float *position;
    float *velocity;
    float mass;
    int n_dims;
} NovaGSAAgent;

typedef struct {
    int n_agents;
    int n_dims;
    int max_iter;
    float G0;
    float alpha;
} NovaGSAOpts;

float* nova_gsa_optimize(const NovaGSAOpts *opts,
                         NovaObjectiveFn objective_fn,
                         void *ctx);

void nova_gsa_free(float *result);

/* ============================================================================
   HARMONY SEARCH
   ============================================================================ */

typedef struct {
    int harmony_memory_size;
    int n_dims;
    int max_iter;
    float hmcr;
    float par;
    float bw;
    float x_min;
    float x_max;
} NovaHSOpts;

float* nova_harmony_search(const NovaHSOpts *opts,
                           NovaObjectiveFn objective_fn,
                           void *ctx);

void nova_hs_free(float *result);

/* ============================================================================
   SPRING-MASS SYSTEM OPTIMIZER
   ============================================================================ */

typedef struct {
    float *position;
    float *velocity;
    float *acceleration;
    float mass;
    int n_dims;
} NovaSMSAgent;

float* nova_spring_mass_optimize(int n_agents,
                                  int n_dims,
                                  int max_iter,
                                  float k,
                                  float damping,
                                  NovaObjectiveFn objective_fn,
                                  void *ctx);

void nova_sms_free(float *result);

#endif
