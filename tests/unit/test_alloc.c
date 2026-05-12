/**
 * @file test_alloc.c
 * @brief Unit tests for memory allocation and custom arena allocators in Nova.
 */

#include "std/alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_basic_alloc() {
    printf("--- Test: Basic Allocation ---\n");
    
    // Allocate
    int *ptr = (int *)nova_alloc(sizeof(int) * 10);
    assert(ptr != NULL);
    
    // Write and read
    for (int i = 0; i < 10; i++) {
        ptr[i] = i * 10;
    }
    for (int i = 0; i < 10; i++) {
        assert(ptr[i] == i * 10);
    }
    
    // Free
    nova_free(ptr);
    printf("✓ Basic allocation and free successful\n");
}

void test_calloc() {
    printf("--- Test: Calloc (Zeroed Memory) ---\n");
    
    size_t count = 20;
    int *ptr = (int *)nova_calloc(count, sizeof(int));
    assert(ptr != NULL);
    
    // Verify zero-initialization
    for (size_t i = 0; i < count; i++) {
        assert(ptr[i] == 0);
    }
    
    nova_free(ptr);
    printf("✓ Calloc zeroed memory successful\n");
}

void test_realloc() {
    printf("--- Test: Realloc ---\n");
    
    int *ptr = (int *)nova_alloc(sizeof(int) * 5);
    assert(ptr != NULL);
    for (int i = 0; i < 5; i++) {
        ptr[i] = i;
    }
    
    // Resize larger
    ptr = (int *)nova_realloc(ptr, sizeof(int) * 10);
    assert(ptr != NULL);
    
    // Verify old contents
    for (int i = 0; i < 5; i++) {
        assert(ptr[i] == i);
    }
    
    nova_free(ptr);
    printf("✓ Reallocation successful\n");
}

void test_arena() {
    printf("--- Test: Arena Allocator ---\n");
    
    // Create arena with default capacity
    nova_arena_t *arena = nova_arena_create(0);
    assert(arena != NULL);
    
    // Allocate blocks
    void *p1 = nova_arena_alloc(arena, 16);
    void *p2 = nova_arena_alloc(arena, 32);
    void *p3 = nova_arena_alloc(arena, 64);
    
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);
    
    // Reset arena
    nova_arena_reset(arena);
    
    // Arena can be reused
    void *p4 = nova_arena_alloc(arena, 128);
    assert(p4 != NULL);
    
    // Destroy arena
    nova_arena_destroy(arena);
    printf("✓ Arena allocation and life cycle successful\n");
}

int main(void) {
    printf("=== Running Allocation Unit Tests ===\n");
    test_basic_alloc();
    test_calloc();
    test_realloc();
    test_arena();
    printf("=== All Allocation Tests Passed! ===\n");
    return 0;
}
