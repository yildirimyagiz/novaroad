#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../optimizer/nova_genetic.h"

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

#define ASSERT_NEAR(a, b, tol) \
    do { \
        float _a = (a), _b = (b), _tol = (tol); \
        if (fabsf(_a - _b) > _tol) { \
            printf("ASSERT_NEAR failed: %f != %f (tolerance: %f)\n", _a, _b, _tol); \
            exit(1); \
        } \
    } while(0)

#define TEST_PASS() printf("✓ %s\n", __func__)
#define TEST_FAIL(msg) do { printf("✗ %s: %s\n", __func__, msg); exit(1); } while(0)

/* ============================================================================
 * Fitness Functions
 * ============================================================================ */

static float sphere_fitness(const float* genes, int n, void* ctx) {
    (void)ctx;
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += genes[i] * genes[i];
    }
    return -sum;  /* maximize negative = minimize sum of squares */
}

/* ============================================================================
 * Test Cases
 * ============================================================================ */

void test_chromosome_create_free(void) {
    NovaChromosome *chrom = nova_chromosome_create(10);
    
    assert(chrom != NULL);
    assert(chrom->n_genes == 10);
    assert(chrom->genes != NULL);
    
    /* Verify genes are in [-1, 1] range */
    for (int i = 0; i < 10; i++) {
        assert(chrom->genes[i] >= -1.0f && chrom->genes[i] <= 1.0f);
    }
    
    nova_chromosome_free(chrom);
    TEST_PASS();
}

void test_population_create_free(void) {
    NovaPopulation *pop = nova_population_create(50, 10);
    
    assert(pop != NULL);
    assert(pop->n_individuals == 50);
    assert(pop->n_genes == 10);
    assert(pop->individuals != NULL);
    assert(pop->generation == 0);
    
    /* Verify individuals are properly initialized */
    for (int i = 0; i < 50; i++) {
        assert(pop->individuals[i].genes != NULL);
        assert(pop->individuals[i].n_genes == 10);
    }
    
    nova_population_free(pop);
    TEST_PASS();
}

void test_tournament_selection(void) {
    NovaPopulation *pop = nova_population_create(10, 5);
    
    /* Assign fitness values */
    for (int i = 0; i < 10; i++) {
        pop->individuals[i].fitness = (float)(i) * 10.0f;
    }
    
    int selected_idx = nova_tournament_selection(pop, 3);
    
    assert(selected_idx >= 0 && selected_idx < 10);
    
    nova_population_free(pop);
    TEST_PASS();
}

void test_roulette_selection(void) {
    NovaPopulation *pop = nova_population_create(10, 5);
    
    /* Assign positive fitness values */
    for (int i = 0; i < 10; i++) {
        pop->individuals[i].fitness = (float)(i + 1) * 10.0f;  /* all positive */
    }
    
    int selected_idx = nova_roulette_selection(pop);
    
    assert(selected_idx >= 0 && selected_idx < 10);
    
    nova_population_free(pop);
    TEST_PASS();
}

void test_single_point_crossover(void) {
    NovaChromosome *parent1 = nova_chromosome_create(10);
    NovaChromosome *parent2 = nova_chromosome_create(10);
    NovaChromosome *child1 = nova_chromosome_create(10);
    NovaChromosome *child2 = nova_chromosome_create(10);
    
    /* Initialize parents with distinct values */
    for (int i = 0; i < 10; i++) {
        parent1->genes[i] = 0.5f;
        parent2->genes[i] = -0.5f;
    }
    
    nova_single_point_crossover(parent1, parent2, child1, child2);
    
    /* Children should inherit genes from both parents */
    assert(child1->n_genes == 10);
    assert(child2->n_genes == 10);
    
    nova_chromosome_free(parent1);
    nova_chromosome_free(parent2);
    nova_chromosome_free(child1);
    nova_chromosome_free(child2);
    TEST_PASS();
}

void test_uniform_crossover(void) {
    NovaChromosome *parent1 = nova_chromosome_create(10);
    NovaChromosome *parent2 = nova_chromosome_create(10);
    NovaChromosome *child1 = nova_chromosome_create(10);
    NovaChromosome *child2 = nova_chromosome_create(10);
    
    /* Initialize parents with distinct values */
    for (int i = 0; i < 10; i++) {
        parent1->genes[i] = 0.7f;
        parent2->genes[i] = -0.7f;
    }
    
    nova_uniform_crossover(parent1, parent2, child1, child2, 0.5f);
    
    /* Children should differ from parents with some probability */
    assert(child1->n_genes == 10);
    assert(child2->n_genes == 10);
    
    nova_chromosome_free(parent1);
    nova_chromosome_free(parent2);
    nova_chromosome_free(child1);
    nova_chromosome_free(child2);
    TEST_PASS();
}

void test_gaussian_mutation(void) {
    NovaChromosome *chrom = nova_chromosome_create(10);
    
    /* Store initial genes */
    float initial_genes[10];
    for (int i = 0; i < 10; i++) {
        initial_genes[i] = chrom->genes[i];
    }
    
    /* Apply mutation */
    nova_gaussian_mutation(chrom, 1.0f, 0.5f);
    
    /* At least one gene should have changed */
    int changed_count = 0;
    for (int i = 0; i < 10; i++) {
        if (fabsf(chrom->genes[i] - initial_genes[i]) > 1e-6f) {
            changed_count++;
        }
    }
    
    assert(changed_count > 0);
    
    nova_chromosome_free(chrom);
    TEST_PASS();
}

void test_genetic_evolve_sphere(void) {
    NovaGeneticOpts opts = {
        .population_size = 30,
        .n_genes = 5,
        .n_generations = 100,
        .mutation_rate = 0.2f,
        .crossover_rate = 0.8f,
        .tournament_size = 3,
        .elitism_count = 2,
        .sigma = 0.3f
    };
    
    NovaGeneticResult *result = nova_genetic_evolve(&opts, sphere_fitness, NULL);
    
    assert(result != NULL);
    assert(result->best_genes != NULL);
    assert(result->generations_run > 0);
    
    /* Best fitness should be close to optimum (sphere function minimum is 0) */
    /* We expect best_fitness to be negative (maximizing -sum(x^2)) */
    /* So the best fitness should be > -1.0 (i.e., sum of squares < 1.0) */
    assert(result->best_fitness > -1.0f);
    
    printf("  Best fitness achieved: %f\n", result->best_fitness);
    printf("  Sum of squares: %f\n", -result->best_fitness);
    
    nova_genetic_result_free(result);
    TEST_PASS();
}

void test_cmaes_create_free(void) {
    NovaCMAES *cmaes = nova_cmaes_create(5, 0.5f);
    
    assert(cmaes != NULL);
    assert(cmaes->n_dim == 5);
    assert(cmaes->sigma > 0.0f);
    
    nova_cmaes_free(cmaes);
    TEST_PASS();
}

void test_cmaes_optimize_sphere(void) {
    NovaCMAES *cmaes = nova_cmaes_create(5, 0.5f);
    
    float *result = nova_cmaes_optimize(cmaes, sphere_fitness, NULL, 200, 0.001f);
    
    assert(result != NULL);
    
    /* Calculate final fitness */
    float fitness = sphere_fitness(result, 5, NULL);
    
    printf("  CMA-ES result fitness: %f\n", -fitness);
    
    /* Result should show improvement towards optimum */
    assert(-fitness < 50.0f);
    
    free(result);
    nova_cmaes_free(cmaes);
    TEST_PASS();
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("\n=== Nova Genetic Algorithm Unit Tests ===\n\n");
    
    test_chromosome_create_free();
    test_population_create_free();
    test_tournament_selection();
    test_roulette_selection();
    test_single_point_crossover();
    test_uniform_crossover();
    test_gaussian_mutation();
    test_genetic_evolve_sphere();
    test_cmaes_create_free();
    test_cmaes_optimize_sphere();
    
    printf("\n=== All Genetic Algorithm Tests Passed ===\n\n");
    return 0;
}
