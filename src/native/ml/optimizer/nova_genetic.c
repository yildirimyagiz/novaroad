#include "nova_genetic.h"
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
    /* Box-Muller transform */
    float u1 = randf();
    float u2 = randf();
    if (u1 < 1e-7) u1 = 1e-7;
    return mu + sigma * sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

/* ============================================================================
   INDIVIDUAL / CHROMOSOME
   ============================================================================ */

NovaChromosome* nova_chromosome_create(int n_genes) {
    NovaChromosome *chrom = (NovaChromosome *)malloc(sizeof(NovaChromosome));
    if (!chrom) return NULL;
    
    chrom->genes = (float *)malloc(n_genes * sizeof(float));
    if (!chrom->genes) {
        free(chrom);
        return NULL;
    }
    
    chrom->n_genes = n_genes;
    chrom->fitness = -INFINITY;
    
    /* Initialize with random values in [-1, 1] */
    for (int i = 0; i < n_genes; i++) {
        chrom->genes[i] = randf_range(-1.0f, 1.0f);
    }
    
    return chrom;
}

void nova_chromosome_free(NovaChromosome *chrom) {
    if (!chrom) return;
    free(chrom->genes);
    free(chrom);
}

void nova_chromosome_copy(NovaChromosome *dst, const NovaChromosome *src) {
    if (!dst || !src) return;
    memcpy(dst->genes, src->genes, src->n_genes * sizeof(float));
    dst->fitness = src->fitness;
}

/* ============================================================================
   POPULATION
   ============================================================================ */

NovaPopulation* nova_population_create(int n_individuals, int n_genes) {
    NovaPopulation *pop = (NovaPopulation *)malloc(sizeof(NovaPopulation));
    if (!pop) return NULL;
    
    pop->individuals = (NovaChromosome *)malloc(n_individuals * sizeof(NovaChromosome));
    if (!pop->individuals) {
        free(pop);
        return NULL;
    }
    
    pop->n_individuals = n_individuals;
    pop->n_genes = n_genes;
    pop->generation = 0;
    
    /* Initialize all individuals */
    for (int i = 0; i < n_individuals; i++) {
        pop->individuals[i].genes = (float *)malloc(n_genes * sizeof(float));
        if (!pop->individuals[i].genes) {
            /* Cleanup on allocation failure */
            for (int j = 0; j < i; j++) {
                free(pop->individuals[j].genes);
            }
            free(pop->individuals);
            free(pop);
            return NULL;
        }
        pop->individuals[i].n_genes = n_genes;
        pop->individuals[i].fitness = -INFINITY;
        
        for (int j = 0; j < n_genes; j++) {
            pop->individuals[i].genes[j] = randf_range(-1.0f, 1.0f);
        }
    }
    
    return pop;
}

void nova_population_free(NovaPopulation *pop) {
    if (!pop) return;
    for (int i = 0; i < pop->n_individuals; i++) {
        free(pop->individuals[i].genes);
    }
    free(pop->individuals);
    free(pop);
}

/* ============================================================================
   SELECTION OPERATORS
   ============================================================================ */

int nova_tournament_selection(const NovaPopulation *pop, int tournament_size) {
    if (!pop || tournament_size <= 0) return 0;
    
    int best_idx = rand() % pop->n_individuals;
    float best_fitness = pop->individuals[best_idx].fitness;
    
    for (int i = 1; i < tournament_size; i++) {
        int idx = rand() % pop->n_individuals;
        if (pop->individuals[idx].fitness > best_fitness) {
            best_fitness = pop->individuals[idx].fitness;
            best_idx = idx;
        }
    }
    
    return best_idx;
}

int nova_roulette_selection(const NovaPopulation *pop) {
    if (!pop || pop->n_individuals <= 0) return 0;
    
    /* Find min fitness (for shifting to make all positive) */
    float min_fitness = pop->individuals[0].fitness;
    for (int i = 1; i < pop->n_individuals; i++) {
        if (pop->individuals[i].fitness < min_fitness) {
            min_fitness = pop->individuals[i].fitness;
        }
    }
    
    /* Shift fitness values to be positive */
    float offset = (min_fitness < 0) ? -min_fitness + 1e-6f : 0;
    
    /* Calculate fitness sum */
    float total_fitness = 0.0f;
    for (int i = 0; i < pop->n_individuals; i++) {
        total_fitness += pop->individuals[i].fitness + offset;
    }
    
    if (total_fitness <= 0) return rand() % pop->n_individuals;
    
    /* Roulette wheel selection */
    float spin = randf() * total_fitness;
    float cumsum = 0.0f;
    
    for (int i = 0; i < pop->n_individuals; i++) {
        cumsum += pop->individuals[i].fitness + offset;
        if (cumsum >= spin) {
            return i;
        }
    }
    
    return pop->n_individuals - 1;
}

/* ============================================================================
   GENETIC OPERATORS
   ============================================================================ */

void nova_single_point_crossover(const NovaChromosome *parent1,
                                  const NovaChromosome *parent2,
                                  NovaChromosome *child1,
                                  NovaChromosome *child2) {
    if (!parent1 || !parent2 || !child1 || !child2) return;
    
    int n_genes = parent1->n_genes;
    int split_point = 1 + rand() % (n_genes - 1);
    
    /* Child 1: parent1 first half, parent2 second half */
    for (int i = 0; i < split_point; i++) {
        child1->genes[i] = parent1->genes[i];
    }
    for (int i = split_point; i < n_genes; i++) {
        child1->genes[i] = parent2->genes[i];
    }
    
    /* Child 2: parent2 first half, parent1 second half */
    for (int i = 0; i < split_point; i++) {
        child2->genes[i] = parent2->genes[i];
    }
    for (int i = split_point; i < n_genes; i++) {
        child2->genes[i] = parent1->genes[i];
    }
    
    child1->fitness = -INFINITY;
    child2->fitness = -INFINITY;
}

void nova_uniform_crossover(const NovaChromosome *parent1,
                             const NovaChromosome *parent2,
                             NovaChromosome *child1,
                             NovaChromosome *child2,
                             float prob) {
    if (!parent1 || !parent2 || !child1 || !child2) return;
    
    int n_genes = parent1->n_genes;
    
    for (int i = 0; i < n_genes; i++) {
        if (randf() < prob) {
            /* Swap genes */
            child1->genes[i] = parent2->genes[i];
            child2->genes[i] = parent1->genes[i];
        } else {
            /* Copy directly */
            child1->genes[i] = parent1->genes[i];
            child2->genes[i] = parent2->genes[i];
        }
    }
    
    child1->fitness = -INFINITY;
    child2->fitness = -INFINITY;
}

void nova_gaussian_mutation(NovaChromosome *chromosome,
                             float mutation_rate,
                             float sigma) {
    if (!chromosome) return;
    
    for (int i = 0; i < chromosome->n_genes; i++) {
        if (randf() < mutation_rate) {
            chromosome->genes[i] += gaussian_sample(0.0f, sigma);
        }
    }
    
    chromosome->fitness = -INFINITY;
}

void nova_uniform_mutation(NovaChromosome *chromosome,
                            float mutation_rate,
                            float range) {
    if (!chromosome) return;
    
    for (int i = 0; i < chromosome->n_genes; i++) {
        if (randf() < mutation_rate) {
            chromosome->genes[i] += randf_range(-range, range);
        }
    }
    
    chromosome->fitness = -INFINITY;
}

/* ============================================================================
   GENETIC ALGORITHM ENGINE
   ============================================================================ */

NovaGeneticResult* nova_genetic_evolve(const NovaGeneticOpts *opts,
                                        NovaFitnessFn fitness_fn,
                                        void *fitness_ctx) {
    if (!opts || !fitness_fn) return NULL;
    
    NovaPopulation *pop = nova_population_create(opts->population_size, opts->n_genes);
    if (!pop) return NULL;
    
    NovaGeneticResult *result = (NovaGeneticResult *)malloc(sizeof(NovaGeneticResult));
    if (!result) {
        nova_population_free(pop);
        return NULL;
    }
    
    result->fitness_history = (float *)malloc(opts->n_generations * sizeof(float));
    if (!result->fitness_history) {
        free(result);
        nova_population_free(pop);
        return NULL;
    }
    
    result->best_genes = (float *)malloc(opts->n_genes * sizeof(float));
    if (!result->best_genes) {
        free(result->fitness_history);
        free(result);
        nova_population_free(pop);
        return NULL;
    }
    
    /* Initialize fitness for population */
    for (int i = 0; i < pop->n_individuals; i++) {
        pop->individuals[i].fitness = fitness_fn(pop->individuals[i].genes, 
                                                   pop->individuals[i].n_genes, 
                                                   fitness_ctx);
    }
    
    /* Main evolution loop */
    for (int gen = 0; gen < opts->n_generations; gen++) {
        /* Find best individual */
        int best_idx = 0;
        float best_fitness = pop->individuals[0].fitness;
        for (int i = 1; i < pop->n_individuals; i++) {
            if (pop->individuals[i].fitness > best_fitness) {
                best_fitness = pop->individuals[i].fitness;
                best_idx = i;
            }
        }
        
        result->fitness_history[gen] = best_fitness;
        
        /* Create new population */
        NovaChromosome *new_pop = (NovaChromosome *)malloc(pop->n_individuals * sizeof(NovaChromosome));
        if (!new_pop) break;
        
        for (int i = 0; i < pop->n_individuals; i++) {
            new_pop[i].genes = (float *)malloc(opts->n_genes * sizeof(float));
            if (!new_pop[i].genes) {
                for (int j = 0; j < i; j++) {
                    free(new_pop[j].genes);
                }
                free(new_pop);
                break;
            }
            new_pop[i].n_genes = opts->n_genes;
            new_pop[i].fitness = -INFINITY;
        }
        
        /* Elitism: copy best individuals */
        for (int i = 0; i < opts->elitism_count && i < pop->n_individuals; i++) {
            nova_chromosome_copy(&new_pop[i], &pop->individuals[best_idx]);
        }
        
        /* Generate offspring */
        for (int i = opts->elitism_count; i < pop->n_individuals; i += 2) {
            int p1_idx = nova_tournament_selection(pop, opts->tournament_size);
            int p2_idx = nova_tournament_selection(pop, opts->tournament_size);
            
            if (randf() < opts->crossover_rate) {
                if (i + 1 < pop->n_individuals) {
                    nova_uniform_crossover(&pop->individuals[p1_idx],
                                           &pop->individuals[p2_idx],
                                           &new_pop[i],
                                           &new_pop[i + 1],
                                           0.5f);
                } else {
                    nova_chromosome_copy(&new_pop[i], &pop->individuals[p1_idx]);
                }
            } else {
                nova_chromosome_copy(&new_pop[i], &pop->individuals[p1_idx]);
                if (i + 1 < pop->n_individuals) {
                    nova_chromosome_copy(&new_pop[i + 1], &pop->individuals[p2_idx]);
                }
            }
        }
        
        /* Apply mutation */
        for (int i = opts->elitism_count; i < pop->n_individuals; i++) {
            nova_gaussian_mutation(&new_pop[i], opts->mutation_rate, opts->sigma);
        }
        
        /* Evaluate new population */
        for (int i = 0; i < pop->n_individuals; i++) {
            new_pop[i].fitness = fitness_fn(new_pop[i].genes, 
                                            new_pop[i].n_genes, 
                                            fitness_ctx);
        }
        
        /* Replace old population */
        for (int i = 0; i < pop->n_individuals; i++) {
            free(pop->individuals[i].genes);
        }
        free(pop->individuals);
        pop->individuals = new_pop;
        pop->generation++;
    }
    
    /* Extract best solution */
    int best_idx = 0;
    float best_fitness = pop->individuals[0].fitness;
    for (int i = 1; i < pop->n_individuals; i++) {
        if (pop->individuals[i].fitness > best_fitness) {
            best_fitness = pop->individuals[i].fitness;
            best_idx = i;
        }
    }
    
    memcpy(result->best_genes, pop->individuals[best_idx].genes, opts->n_genes * sizeof(float));
    result->best_fitness = best_fitness;
    result->generations_run = pop->generation;
    
    nova_population_free(pop);
    return result;
}

void nova_genetic_result_free(NovaGeneticResult *result) {
    if (!result) return;
    free(result->best_genes);
    free(result->fitness_history);
    free(result);
}

/* ============================================================================
   CMA-ES (Covariance Matrix Adaptation Evolution Strategy)
   ============================================================================ */

NovaCMAES* nova_cmaes_create(int n_dim, float sigma0) {
    if (n_dim <= 0) return NULL;
    
    NovaCMAES *cmaes = (NovaCMAES *)malloc(sizeof(NovaCMAES));
    if (!cmaes) return NULL;
    
    cmaes->n_dim = n_dim;
    cmaes->lambda = 4 + (int)(3 * logf(n_dim));
    cmaes->mu = cmaes->lambda / 2;
    cmaes->sigma = sigma0;
    
    /* Allocate mean vector */
    cmaes->mean = (float *)malloc(n_dim * sizeof(float));
    if (!cmaes->mean) {
        free(cmaes);
        return NULL;
    }
    
    /* Initialize mean to 0 */
    for (int i = 0; i < n_dim; i++) {
        cmaes->mean[i] = 0.0f;
    }
    
    /* Allocate covariance matrix */
    cmaes->C = (float **)malloc(n_dim * sizeof(float *));
    if (!cmaes->C) {
        free(cmaes->mean);
        free(cmaes);
        return NULL;
    }
    
    for (int i = 0; i < n_dim; i++) {
        cmaes->C[i] = (float *)malloc(n_dim * sizeof(float));
        if (!cmaes->C[i]) {
            for (int j = 0; j < i; j++) {
                free(cmaes->C[j]);
            }
            free(cmaes->C);
            free(cmaes->mean);
            free(cmaes);
            return NULL;
        }
    }
    
    /* Initialize covariance to identity */
    for (int i = 0; i < n_dim; i++) {
        for (int j = 0; j < n_dim; j++) {
            cmaes->C[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    
    /* Allocate samples */
    cmaes->samples = (float **)malloc(cmaes->lambda * sizeof(float *));
    if (!cmaes->samples) {
        for (int i = 0; i < n_dim; i++) {
            free(cmaes->C[i]);
        }
        free(cmaes->C);
        free(cmaes->mean);
        free(cmaes);
        return NULL;
    }
    
    for (int i = 0; i < cmaes->lambda; i++) {
        cmaes->samples[i] = (float *)malloc(n_dim * sizeof(float));
        if (!cmaes->samples[i]) {
            for (int j = 0; j < i; j++) {
                free(cmaes->samples[j]);
            }
            free(cmaes->samples);
            for (int j = 0; j < n_dim; j++) {
                free(cmaes->C[j]);
            }
            free(cmaes->C);
            free(cmaes->mean);
            free(cmaes);
            return NULL;
        }
    }
    
    /* Allocate and initialize weights */
    cmaes->weights = (float *)malloc(cmaes->mu * sizeof(float));
    if (!cmaes->weights) {
        for (int i = 0; i < cmaes->lambda; i++) {
            free(cmaes->samples[i]);
        }
        free(cmaes->samples);
        for (int i = 0; i < n_dim; i++) {
            free(cmaes->C[i]);
        }
        free(cmaes->C);
        free(cmaes->mean);
        free(cmaes);
        return NULL;
    }
    
    float sum_weights = 0.0f;
    for (int i = 0; i < cmaes->mu; i++) {
        cmaes->weights[i] = logf(cmaes->mu + 0.5f) - logf(i + 1.0f);
        sum_weights += cmaes->weights[i];
    }
    
    for (int i = 0; i < cmaes->mu; i++) {
        cmaes->weights[i] /= sum_weights;
    }
    
    return cmaes;
}

void nova_cmaes_free(NovaCMAES *cmaes) {
    if (!cmaes) return;
    
    free(cmaes->mean);
    
    for (int i = 0; i < cmaes->n_dim; i++) {
        free(cmaes->C[i]);
    }
    free(cmaes->C);
    
    for (int i = 0; i < cmaes->lambda; i++) {
        free(cmaes->samples[i]);
    }
    free(cmaes->samples);
    
    free(cmaes->weights);
    free(cmaes);
}

void nova_cmaes_ask(NovaCMAES *cmaes, float **samples) {
    if (!cmaes || !samples) return;
    
    /* Sample from multivariate Gaussian with mean and covariance C */
    for (int k = 0; k < cmaes->lambda; k++) {
        for (int i = 0; i < cmaes->n_dim; i++) {
            /* Sample standard Gaussian */
            float z = gaussian_sample(0.0f, 1.0f);
            
            /* Transform by covariance matrix (simplified: just scale by sigma) */
            float sample_val = cmaes->mean[i] + cmaes->sigma * z;
            
            samples[k][i] = sample_val;
        }
    }
}

void nova_cmaes_tell(NovaCMAES *cmaes,
                     float **samples,
                     const float *fitnesses) {
    if (!cmaes || !samples || !fitnesses) return;
    
    /* Sort samples by fitness */
    int *indices = (int *)malloc(cmaes->lambda * sizeof(int));
    if (!indices) return;
    
    for (int i = 0; i < cmaes->lambda; i++) {
        indices[i] = i;
    }
    
    /* Simple insertion sort */
    for (int i = 1; i < cmaes->lambda; i++) {
        for (int j = i; j > 0 && fitnesses[indices[j-1]] < fitnesses[indices[j]]; j--) {
            int tmp = indices[j];
            indices[j] = indices[j-1];
            indices[j-1] = tmp;
        }
    }
    
    /* Update mean with top mu solutions */
    for (int i = 0; i < cmaes->n_dim; i++) {
        cmaes->mean[i] = 0.0f;
    }
    
    for (int k = 0; k < cmaes->mu; k++) {
        int idx = indices[k];
        for (int i = 0; i < cmaes->n_dim; i++) {
            cmaes->mean[i] += cmaes->weights[k] * samples[idx][i];
        }
    }
    
    /* Update sigma (simplified adaptive update) */
    cmaes->sigma *= expf(0.01f);
    
    free(indices);
}

float* nova_cmaes_optimize(NovaCMAES *cmaes,
                           NovaFitnessFn fitness_fn,
                           void *ctx,
                           int max_iter,
                           float tol) {
    if (!cmaes || !fitness_fn) return NULL;
    
    float *fitnesses = (float *)malloc(cmaes->lambda * sizeof(float));
    if (!fitnesses) return NULL;
    
    float prev_best = -INFINITY;
    
    for (int iter = 0; iter < max_iter; iter++) {
        /* Ask for samples */
        nova_cmaes_ask(cmaes, cmaes->samples);
        
        /* Evaluate fitness */
        for (int k = 0; k < cmaes->lambda; k++) {
            fitnesses[k] = fitness_fn(cmaes->samples[k], cmaes->n_dim, ctx);
        }
        
        /* Tell the algorithm */
        nova_cmaes_tell(cmaes, cmaes->samples, fitnesses);
        
        /* Find best fitness */
        float best_fitness = fitnesses[0];
        for (int k = 1; k < cmaes->lambda; k++) {
            if (fitnesses[k] > best_fitness) {
                best_fitness = fitnesses[k];
            }
        }
        
        /* Check convergence */
        if (fabsf(best_fitness - prev_best) < tol) {
            break;
        }
        prev_best = best_fitness;
    }
    
    /* Return copy of mean as best solution */
    float *result = (float *)malloc(cmaes->n_dim * sizeof(float));
    if (result) {
        memcpy(result, cmaes->mean, cmaes->n_dim * sizeof(float));
    }
    
    free(fitnesses);
    return result;
}
