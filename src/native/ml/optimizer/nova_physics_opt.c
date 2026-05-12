#include "nova_physics_opt.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ============================================================================
   UTILITY FUNCTIONS
   ============================================================================ */

static float randf() {
    return (float)rand() / RAND_MAX;
}

static float randf_range(float min, float max) {
    return min + (max - min) * randf();
}

static float gaussian_sample(float mu, float sigma) {
    float u1 = randf();
    float u2 = randf();
    if (u1 < 1e-7) u1 = 1e-7;
    return mu + sigma * sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

/* ============================================================================
   SIMULATED ANNEALING
   ============================================================================ */

float nova_sa_accept_prob(float delta_e, float temperature) {
    if (temperature <= 0) return 0.0f;
    if (delta_e <= 0) return 1.0f;
    return expf(-delta_e / temperature);
}

float* nova_simulated_annealing(const NovaSAOpts *opts,
                                 NovaObjectiveFn objective_fn,
                                 void *ctx,
                                 const float *x0) {
    if (!opts || !objective_fn || !x0) return NULL;
    
    float *best_x = (float *)malloc(opts->n_dims * sizeof(float));
    float *current_x = (float *)malloc(opts->n_dims * sizeof(float));
    float *neighbor_x = (float *)malloc(opts->n_dims * sizeof(float));
    
    if (!best_x || !current_x || !neighbor_x) {
        free(best_x);
        free(current_x);
        free(neighbor_x);
        return NULL;
    }
    
    /* Initialize */
    memcpy(current_x, x0, opts->n_dims * sizeof(float));
    memcpy(best_x, x0, opts->n_dims * sizeof(float));
    
    float current_energy = objective_fn(current_x, opts->n_dims, ctx);
    float best_energy = current_energy;
    float temperature = opts->initial_temp;
    
    for (int iter = 0; iter < opts->max_iter; iter++) {
        /* Generate neighbor solution */
        for (int i = 0; i < opts->n_dims; i++) {
            neighbor_x[i] = current_x[i] + gaussian_sample(0.0f, temperature * 0.1f);
        }
        
        float neighbor_energy = objective_fn(neighbor_x, opts->n_dims, ctx);
        float delta_e = neighbor_energy - current_energy;
        
        /* Acceptance criterion (maximization) */
        if (delta_e > 0 || randf() < nova_sa_accept_prob(-delta_e, temperature)) {
            memcpy(current_x, neighbor_x, opts->n_dims * sizeof(float));
            current_energy = neighbor_energy;
            
            if (current_energy > best_energy) {
                memcpy(best_x, current_x, opts->n_dims * sizeof(float));
                best_energy = current_energy;
            }
        }
        
        /* Cool down */
        temperature *= opts->cooling_rate;
        if (temperature < opts->min_temp) {
            temperature = opts->min_temp;
        }
    }
    
    free(current_x);
    free(neighbor_x);
    return best_x;
}

/* ============================================================================
   PARTICLE SWARM OPTIMIZATION (PSO)
   ============================================================================ */

float* nova_pso_optimize(const NovaPSOOpts *opts,
                         NovaObjectiveFn objective_fn,
                         void *ctx) {
    if (!opts || !objective_fn) return NULL;
    
    /* Allocate particles */
    NovaParticle *particles = (NovaParticle *)malloc(opts->n_particles * sizeof(NovaParticle));
    if (!particles) return NULL;
    
    for (int i = 0; i < opts->n_particles; i++) {
        particles[i].position = (float *)malloc(opts->n_dims * sizeof(float));
        particles[i].velocity = (float *)malloc(opts->n_dims * sizeof(float));
        particles[i].personal_best_pos = (float *)malloc(opts->n_dims * sizeof(float));
        particles[i].n_dims = opts->n_dims;
        
        if (!particles[i].position || !particles[i].velocity || !particles[i].personal_best_pos) {
            for (int j = 0; j <= i; j++) {
                free(particles[j].position);
                free(particles[j].velocity);
                free(particles[j].personal_best_pos);
            }
            free(particles);
            return NULL;
        }
    }
    
    /* Initialize particles */
    float global_best_val = -INFINITY;
    float *global_best_pos = (float *)malloc(opts->n_dims * sizeof(float));
    if (!global_best_pos) {
        for (int i = 0; i < opts->n_particles; i++) {
            free(particles[i].position);
            free(particles[i].velocity);
            free(particles[i].personal_best_pos);
        }
        free(particles);
        return NULL;
    }
    
    for (int i = 0; i < opts->n_particles; i++) {
        for (int d = 0; d < opts->n_dims; d++) {
            particles[i].position[d] = randf_range(opts->x_min, opts->x_max);
            particles[i].velocity[d] = randf_range(-1.0f, 1.0f);
        }
        
        particles[i].personal_best_val = objective_fn(particles[i].position, 
                                                      opts->n_dims, ctx);
        memcpy(particles[i].personal_best_pos, particles[i].position, 
               opts->n_dims * sizeof(float));
        
        if (particles[i].personal_best_val > global_best_val) {
            global_best_val = particles[i].personal_best_val;
            memcpy(global_best_pos, particles[i].position, opts->n_dims * sizeof(float));
        }
    }
    
    /* Main PSO loop */
    for (int iter = 0; iter < opts->max_iter; iter++) {
        for (int i = 0; i < opts->n_particles; i++) {
            for (int d = 0; d < opts->n_dims; d++) {
                float r1 = randf();
                float r2 = randf();
                
                /* Update velocity */
                particles[i].velocity[d] = 
                    opts->w * particles[i].velocity[d] +
                    opts->c1 * r1 * (particles[i].personal_best_pos[d] - particles[i].position[d]) +
                    opts->c2 * r2 * (global_best_pos[d] - particles[i].position[d]);
                
                /* Update position */
                particles[i].position[d] += particles[i].velocity[d];
                
                /* Boundary handling */
                if (particles[i].position[d] < opts->x_min) {
                    particles[i].position[d] = opts->x_min;
                    particles[i].velocity[d] *= -0.5f;
                }
                if (particles[i].position[d] > opts->x_max) {
                    particles[i].position[d] = opts->x_max;
                    particles[i].velocity[d] *= -0.5f;
                }
            }
            
            /* Evaluate fitness */
            float fitness = objective_fn(particles[i].position, opts->n_dims, ctx);
            
            /* Update personal best */
            if (fitness > particles[i].personal_best_val) {
                particles[i].personal_best_val = fitness;
                memcpy(particles[i].personal_best_pos, particles[i].position,
                       opts->n_dims * sizeof(float));
                
                /* Update global best */
                if (fitness > global_best_val) {
                    global_best_val = fitness;
                    memcpy(global_best_pos, particles[i].position, 
                           opts->n_dims * sizeof(float));
                }
            }
        }
    }
    
    /* Cleanup particles */
    for (int i = 0; i < opts->n_particles; i++) {
        free(particles[i].position);
        free(particles[i].velocity);
        free(particles[i].personal_best_pos);
    }
    free(particles);
    
    return global_best_pos;
}

void nova_pso_free(float *result) {
    free(result);
}

/* ============================================================================
   GRAVITATIONAL SEARCH ALGORITHM (GSA)
   ============================================================================ */

float* nova_gsa_optimize(const NovaGSAOpts *opts,
                         NovaObjectiveFn objective_fn,
                         void *ctx) {
    if (!opts || !objective_fn) return NULL;
    
    /* Allocate agents */
    NovaGSAAgent *agents = (NovaGSAAgent *)malloc(opts->n_agents * sizeof(NovaGSAAgent));
    if (!agents) return NULL;
    
    for (int i = 0; i < opts->n_agents; i++) {
        agents[i].position = (float *)malloc(opts->n_dims * sizeof(float));
        agents[i].velocity = (float *)malloc(opts->n_dims * sizeof(float));
        agents[i].n_dims = opts->n_dims;
        
        if (!agents[i].position || !agents[i].velocity) {
            for (int j = 0; j <= i; j++) {
                free(agents[j].position);
                free(agents[j].velocity);
            }
            free(agents);
            return NULL;
        }
    }
    
    float *fitnesses = (float *)malloc(opts->n_agents * sizeof(float));
    float *best_pos = (float *)malloc(opts->n_dims * sizeof(float));
    
    if (!fitnesses || !best_pos) {
        for (int i = 0; i < opts->n_agents; i++) {
            free(agents[i].position);
            free(agents[i].velocity);
        }
        free(agents);
        free(fitnesses);
        free(best_pos);
        return NULL;
    }
    
    /* Initialize agents */
    float best_fitness = -INFINITY;
    int best_idx = 0;
    
    for (int i = 0; i < opts->n_agents; i++) {
        for (int d = 0; d < opts->n_dims; d++) {
            agents[i].position[d] = randf_range(-100.0f, 100.0f);
            agents[i].velocity[d] = 0.0f;
        }
        
        fitnesses[i] = objective_fn(agents[i].position, opts->n_dims, ctx);
        agents[i].mass = 1.0f / opts->n_agents;
        
        if (fitnesses[i] > best_fitness) {
            best_fitness = fitnesses[i];
            best_idx = i;
        }
    }
    memcpy(best_pos, agents[best_idx].position, opts->n_dims * sizeof(float));
    
    /* Main GSA loop */
    for (int iter = 0; iter < opts->max_iter; iter++) {
        float G = opts->G0 * expf(-opts->alpha * iter / (float)opts->max_iter);
        
        /* Update forces and accelerations */
        for (int i = 0; i < opts->n_agents; i++) {
            for (int d = 0; d < opts->n_dims; d++) {
                float force = 0.0f;
                
                /* Gravitational attraction from best agent */
                force += G * agents[best_idx].mass * 
                        (agents[best_idx].position[d] - agents[i].position[d]) / 
                        (1e-6f + fabsf(agents[best_idx].position[d] - agents[i].position[d]));
                
                float acceleration = force / agents[i].mass;
                agents[i].velocity[d] = randf() * agents[i].velocity[d] + acceleration;
                agents[i].position[d] += agents[i].velocity[d];
            }
            
            /* Evaluate fitness */
            fitnesses[i] = objective_fn(agents[i].position, opts->n_dims, ctx);
            
            if (fitnesses[i] > best_fitness) {
                best_fitness = fitnesses[i];
                best_idx = i;
                memcpy(best_pos, agents[i].position, opts->n_dims * sizeof(float));
            }
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < opts->n_agents; i++) {
        free(agents[i].position);
        free(agents[i].velocity);
    }
    free(agents);
    free(fitnesses);
    
    return best_pos;
}

void nova_gsa_free(float *result) {
    free(result);
}

/* ============================================================================
   HARMONY SEARCH
   ============================================================================ */

float* nova_harmony_search(const NovaHSOpts *opts,
                           NovaObjectiveFn objective_fn,
                           void *ctx) {
    if (!opts || !objective_fn) return NULL;
    
    /* Allocate harmony memory */
    float **harmony_memory = (float **)malloc(opts->harmony_memory_size * sizeof(float *));
    float *harmony_fitnesses = (float *)malloc(opts->harmony_memory_size * sizeof(float));
    
    if (!harmony_memory || !harmony_fitnesses) {
        free(harmony_memory);
        free(harmony_fitnesses);
        return NULL;
    }
    
    for (int i = 0; i < opts->harmony_memory_size; i++) {
        harmony_memory[i] = (float *)malloc(opts->n_dims * sizeof(float));
        if (!harmony_memory[i]) {
            for (int j = 0; j < i; j++) {
                free(harmony_memory[j]);
            }
            free(harmony_memory);
            free(harmony_fitnesses);
            return NULL;
        }
    }
    
    float *best_harmony = (float *)malloc(opts->n_dims * sizeof(float));
    if (!best_harmony) {
        for (int i = 0; i < opts->harmony_memory_size; i++) {
            free(harmony_memory[i]);
        }
        free(harmony_memory);
        free(harmony_fitnesses);
        return NULL;
    }
    
    /* Initialize harmony memory */
    float best_fitness = -INFINITY;
    
    for (int i = 0; i < opts->harmony_memory_size; i++) {
        for (int d = 0; d < opts->n_dims; d++) {
            harmony_memory[i][d] = randf_range(opts->x_min, opts->x_max);
        }
        harmony_fitnesses[i] = objective_fn(harmony_memory[i], opts->n_dims, ctx);
        
        if (harmony_fitnesses[i] > best_fitness) {
            best_fitness = harmony_fitnesses[i];
            memcpy(best_harmony, harmony_memory[i], opts->n_dims * sizeof(float));
        }
    }
    
    /* Main HS loop */
    for (int iter = 0; iter < opts->max_iter; iter++) {
        float *new_harmony = (float *)malloc(opts->n_dims * sizeof(float));
        if (!new_harmony) break;
        
        /* Generate new harmony */
        for (int d = 0; d < opts->n_dims; d++) {
            if (randf() < opts->hmcr) {
                /* Select from harmony memory */
                int idx = rand() % opts->harmony_memory_size;
                new_harmony[d] = harmony_memory[idx][d];
                
                /* Pitch adjustment */
                if (randf() < opts->par) {
                    new_harmony[d] += randf_range(-opts->bw, opts->bw);
                }
            } else {
                /* Random value */
                new_harmony[d] = randf_range(opts->x_min, opts->x_max);
            }
            
            /* Boundary handling */
            if (new_harmony[d] < opts->x_min) new_harmony[d] = opts->x_min;
            if (new_harmony[d] > opts->x_max) new_harmony[d] = opts->x_max;
        }
        
        /* Evaluate new harmony */
        float new_fitness = objective_fn(new_harmony, opts->n_dims, ctx);
        
        /* Update harmony memory (worst harmony replacement) */
        int worst_idx = 0;
        float worst_fitness = harmony_fitnesses[0];
        for (int i = 1; i < opts->harmony_memory_size; i++) {
            if (harmony_fitnesses[i] < worst_fitness) {
                worst_fitness = harmony_fitnesses[i];
                worst_idx = i;
            }
        }
        
        if (new_fitness > worst_fitness) {
            memcpy(harmony_memory[worst_idx], new_harmony, opts->n_dims * sizeof(float));
            harmony_fitnesses[worst_idx] = new_fitness;
            
            if (new_fitness > best_fitness) {
                best_fitness = new_fitness;
                memcpy(best_harmony, new_harmony, opts->n_dims * sizeof(float));
            }
        }
        
        free(new_harmony);
    }
    
    /* Cleanup */
    for (int i = 0; i < opts->harmony_memory_size; i++) {
        free(harmony_memory[i]);
    }
    free(harmony_memory);
    free(harmony_fitnesses);
    
    return best_harmony;
}

void nova_hs_free(float *result) {
    free(result);
}

/* ============================================================================
   SPRING-MASS SYSTEM OPTIMIZER
   ============================================================================ */

float* nova_spring_mass_optimize(int n_agents,
                                  int n_dims,
                                  int max_iter,
                                  float k,
                                  float damping,
                                  NovaObjectiveFn objective_fn,
                                  void *ctx) {
    if (n_agents <= 0 || n_dims <= 0 || !objective_fn) return NULL;
    
    /* Allocate agents */
    NovaSMSAgent *agents = (NovaSMSAgent *)malloc(n_agents * sizeof(NovaSMSAgent));
    if (!agents) return NULL;
    
    for (int i = 0; i < n_agents; i++) {
        agents[i].position = (float *)malloc(n_dims * sizeof(float));
        agents[i].velocity = (float *)malloc(n_dims * sizeof(float));
        agents[i].acceleration = (float *)malloc(n_dims * sizeof(float));
        agents[i].n_dims = n_dims;
        agents[i].mass = 1.0f / n_agents;
        
        if (!agents[i].position || !agents[i].velocity || !agents[i].acceleration) {
            for (int j = 0; j <= i; j++) {
                free(agents[j].position);
                free(agents[j].velocity);
                free(agents[j].acceleration);
            }
            free(agents);
            return NULL;
        }
    }
    
    float *fitnesses = (float *)malloc(n_agents * sizeof(float));
    float *best_pos = (float *)malloc(n_dims * sizeof(float));
    
    if (!fitnesses || !best_pos) {
        for (int i = 0; i < n_agents; i++) {
            free(agents[i].position);
            free(agents[i].velocity);
            free(agents[i].acceleration);
        }
        free(agents);
        free(fitnesses);
        free(best_pos);
        return NULL;
    }
    
    /* Initialize agents */
    float best_fitness = -INFINITY;
    int best_idx = 0;
    
    for (int i = 0; i < n_agents; i++) {
        for (int d = 0; d < n_dims; d++) {
            agents[i].position[d] = randf_range(-10.0f, 10.0f);
            agents[i].velocity[d] = 0.0f;
            agents[i].acceleration[d] = 0.0f;
        }
        
        fitnesses[i] = objective_fn(agents[i].position, n_dims, ctx);
        
        if (fitnesses[i] > best_fitness) {
            best_fitness = fitnesses[i];
            best_idx = i;
        }
    }
    memcpy(best_pos, agents[best_idx].position, n_dims * sizeof(float));
    
    /* Main SMS loop */
    for (int iter = 0; iter < max_iter; iter++) {
        /* Calculate forces on each agent */
        for (int i = 0; i < n_agents; i++) {
            for (int d = 0; d < n_dims; d++) {
                agents[i].acceleration[d] = 0.0f;
            }
            
            /* Spring force toward best position */
            for (int d = 0; d < n_dims; d++) {
                float displacement = best_pos[d] - agents[i].position[d];
                float force = k * displacement;
                
                /* Damping force */
                force -= damping * agents[i].velocity[d];
                
                agents[i].acceleration[d] = force / agents[i].mass;
            }
            
            /* Update velocity and position */
            for (int d = 0; d < n_dims; d++) {
                agents[i].velocity[d] += agents[i].acceleration[d];
                agents[i].position[d] += agents[i].velocity[d];
            }
            
            /* Evaluate fitness */
            fitnesses[i] = objective_fn(agents[i].position, n_dims, ctx);
            
            if (fitnesses[i] > best_fitness) {
                best_fitness = fitnesses[i];
                best_idx = i;
                memcpy(best_pos, agents[i].position, n_dims * sizeof(float));
            }
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < n_agents; i++) {
        free(agents[i].position);
        free(agents[i].velocity);
        free(agents[i].acceleration);
    }
    free(agents);
    free(fitnesses);
    
    return best_pos;
}

void nova_sms_free(float *result) {
    free(result);
}
