#ifndef NOVA_GENETIC_H
#define NOVA_GENETIC_H

#include <stdint.h>
#include <stdlib.h>

/* ============================================================================
   INDIVIDUAL / CHROMOSOME
   ============================================================================ */

typedef struct {
    float *genes;
    int n_genes;
    float fitness;
} NovaChromosome;

NovaChromosome* nova_chromosome_create(int n_genes);
void nova_chromosome_free(NovaChromosome *chrom);
void nova_chromosome_copy(NovaChromosome *dst, const NovaChromosome *src);

/* ============================================================================
   POPULATION
   ============================================================================ */

typedef struct {
    NovaChromosome *individuals;
    int n_individuals;
    int n_genes;
    int generation;
} NovaPopulation;

NovaPopulation* nova_population_create(int n_individuals, int n_genes);
void nova_population_free(NovaPopulation *pop);

/* ============================================================================
   SELECTION OPERATORS
   ============================================================================ */

int nova_tournament_selection(const NovaPopulation *pop, int tournament_size);
int nova_roulette_selection(const NovaPopulation *pop);

/* ============================================================================
   GENETIC OPERATORS
   ============================================================================ */

void nova_single_point_crossover(const NovaChromosome *parent1,
                                  const NovaChromosome *parent2,
                                  NovaChromosome *child1,
                                  NovaChromosome *child2);

void nova_uniform_crossover(const NovaChromosome *parent1,
                             const NovaChromosome *parent2,
                             NovaChromosome *child1,
                             NovaChromosome *child2,
                             float prob);

void nova_gaussian_mutation(NovaChromosome *chromosome,
                             float mutation_rate,
                             float sigma);

void nova_uniform_mutation(NovaChromosome *chromosome,
                            float mutation_rate,
                            float range);

/* ============================================================================
   GENETIC ALGORITHM ENGINE
   ============================================================================ */

typedef struct {
    int population_size;
    int n_genes;
    int n_generations;
    float mutation_rate;
    float crossover_rate;
    int tournament_size;
    int elitism_count;
    float sigma;
} NovaGeneticOpts;

typedef struct {
    float *best_genes;
    float best_fitness;
    int generations_run;
    float *fitness_history;
} NovaGeneticResult;

typedef float (*NovaFitnessFn)(const float *genes, int n_genes, void *ctx);

NovaGeneticResult* nova_genetic_evolve(const NovaGeneticOpts *opts,
                                        NovaFitnessFn fitness_fn,
                                        void *fitness_ctx);

void nova_genetic_result_free(NovaGeneticResult *result);

/* ============================================================================
   CMA-ES (Covariance Matrix Adaptation Evolution Strategy)
   ============================================================================ */

typedef struct {
    float *mean;
    float sigma;
    float **C;
    float **samples;
    float *weights;
    int n_dim;
    int lambda;
    int mu;
} NovaCMAES;

NovaCMAES* nova_cmaes_create(int n_dim, float sigma0);
void nova_cmaes_free(NovaCMAES *cmaes);
void nova_cmaes_ask(NovaCMAES *cmaes, float **samples);
void nova_cmaes_tell(NovaCMAES *cmaes, float **samples, const float *fitnesses);
float* nova_cmaes_optimize(NovaCMAES *cmaes, NovaFitnessFn fitness_fn, void *ctx, int max_iter, float tol);

#endif
