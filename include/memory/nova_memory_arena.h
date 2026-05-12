#ifndef NOVA_MEMORY_ARENA_H
#define NOVA_MEMORY_ARENA_H

#include "../ml/nova_tensor.h"

// Nova Memory Arena Planner
// Advanced memory management for ML workloads
// Prevents fragmentation and optimizes GPU memory

// NovaMemoryType is now unified as NovaBackendType
typedef NovaBackendType NovaMemoryType;

typedef struct NovaMemoryBlock {
  void *ptr;
  size_t size;
  size_t used;
  int is_free;
  struct NovaMemoryBlock *next;
  struct NovaMemoryBlock *prev;
  NovaMemoryType type;
  NovaLifetimeClass lifetime_class; // LIFETIME CLASS
  uint64_t last_used;                 // For LRU eviction
  char *debug_name;                   // For debugging
} NovaMemoryBlock;

typedef struct {
  NovaMemoryBlock *blocks;
  size_t total_allocated;
  size_t total_used;
  size_t block_count;
  size_t page_size; // Usually 4KB or 64KB
  NovaMemoryType type;

  // Separate arenas for different lifetimes - LIFETIME SEPARATION
  NovaMemoryBlock *persistent_blocks; // Model weights
  NovaMemoryBlock *temp_blocks;       // Graph temporaries
  NovaMemoryBlock *scratch_blocks;    // Scratch buffers
  NovaMemoryBlock *staging_blocks;    // GPU staging

  // Statistics
  uint64_t allocations;
  uint64_t deallocations;
  uint64_t peak_usage;
  double fragmentation_ratio;
  NovaLifetimeClass default_lifetime; // Default lifetime class

  // GPU-specific
  void *gpu_context; // Metal/CUDA/Vulkan context
  int supports_unified_memory;
} NovaMemoryArena;

// Arena lifecycle
NovaMemoryArena *nova_memory_arena_create(NovaMemoryType type,
                                              size_t initial_size);
void nova_memory_arena_destroy(NovaMemoryArena *arena);

// Memory allocation/deallocation
void *nova_arena_malloc(NovaMemoryArena *arena, size_t size,
                          const char *debug_name);
void nova_arena_free(NovaMemoryArena *arena, void *ptr);
void *nova_arena_realloc(NovaMemoryArena *arena, void *ptr,
                           size_t new_size);

// Bulk operations
void **nova_arena_malloc_batch(NovaMemoryArena *arena, size_t *sizes,
                                 int count, const char **debug_names);
void nova_arena_free_batch(NovaMemoryArena *arena, void **ptrs, int count);

// Memory planning for ML operations
typedef struct {
  char *operation_name;
  size_t input_sizes[8];  // Up to 8 inputs
  size_t output_sizes[4]; // Up to 4 outputs
  size_t temp_sizes[4];   // Temporary buffers
  int num_inputs;
  int num_outputs;
  int num_temps;
} NovaMemoryPlan;

typedef struct {
  NovaMemoryPlan *plans;
  int num_plans;
  size_t total_memory_needed;
  NovaMemoryArena *arena;
} NovaOperationPlanner;

// Operation planning
NovaOperationPlanner *
nova_operation_planner_create(NovaMemoryArena *arena);
void nova_operation_planner_destroy(NovaOperationPlanner *planner);

// Plan common ML operations
int nova_plan_matmul(NovaOperationPlanner *planner, int64_t m, int64_t n,
                       int64_t k);
int nova_plan_conv2d(NovaOperationPlanner *planner, int in_h, int in_w,
                       int out_h, int out_w, int in_c, int out_c, int k_h,
                       int k_w);
int nova_plan_attention(NovaOperationPlanner *planner, int seq_len,
                          int embed_dim, int num_heads);
int nova_plan_unet_block(NovaOperationPlanner *planner, int channels,
                           int height, int width);

// Execute planned operations
void *nova_execute_planned_operation(NovaOperationPlanner *planner,
                                       int plan_index);

// Memory optimization
void nova_memory_defragment(NovaMemoryArena *arena);
void nova_memory_compact(NovaMemoryArena *arena);
size_t nova_memory_suggest_size(NovaOperationPlanner *planner);

// GPU memory management
typedef struct {
  NovaMemoryArena *gpu_arena;
  NovaMemoryArena *cpu_pinned; // For fast transfers
  size_t max_gpu_memory;
  int supports_async_transfer;
} NovaGPUMemoryManager;

NovaGPUMemoryManager *
nova_gpu_memory_manager_create(size_t gpu_memory_limit);
void nova_gpu_memory_manager_destroy(NovaGPUMemoryManager *manager);

// GPU memory operations
void *nova_gpu_malloc(NovaGPUMemoryManager *manager, size_t size,
                        const char *debug_name);
void nova_gpu_free(NovaGPUMemoryManager *manager, void *ptr);
int nova_gpu_copy_to_device(NovaGPUMemoryManager *manager,
                              const void *host_data, void *device_data,
                              size_t size);
int nova_gpu_copy_from_device(NovaGPUMemoryManager *manager,
                                const void *device_data, void *host_data,
                                size_t size);
int nova_gpu_copy_device_to_device(NovaGPUMemoryManager *manager,
                                     const void *src, void *dst, size_t size);

// Async operations
typedef struct NovaAsyncTransfer NovaAsyncTransfer;
NovaAsyncTransfer *
nova_gpu_async_copy_to_device(NovaGPUMemoryManager *manager,
                                const void *host_data, void *device_data,
                                size_t size);
int nova_gpu_async_wait(NovaAsyncTransfer *transfer);

// Memory pooling for frequent allocations
typedef struct {
  void **free_blocks;
  size_t *block_sizes;
  int num_blocks;
  int capacity;
  NovaMemoryArena *arena;
} NovaMemoryPool;

NovaMemoryPool *nova_memory_pool_create(NovaMemoryArena *arena,
                                            size_t *common_sizes,
                                            int num_sizes);
void nova_memory_pool_destroy(NovaMemoryPool *pool);
void *nova_memory_pool_alloc(NovaMemoryPool *pool, size_t size);
void nova_memory_pool_free(NovaMemoryPool *pool, void *ptr);

// ML-specific memory patterns
void *nova_alloc_tensor(NovaMemoryArena *arena, int *shape, int ndim,
                          size_t element_size, const char *name);
void *nova_alloc_weights(NovaMemoryArena *arena, int in_dim, int out_dim,
                           const char *layer_name);
void *nova_alloc_gradients(NovaMemoryArena *arena, int param_count,
                             const char *param_name);

// Memory statistics and monitoring
typedef struct {
  size_t current_usage;
  size_t peak_usage;
  size_t total_allocations;
  size_t total_deallocations;
  double fragmentation_ratio;
  double allocation_efficiency;
  uint64_t average_allocation_time_ns;
  uint64_t average_deallocation_time_ns;
} NovaMemoryStats;

void nova_memory_get_stats(NovaMemoryArena *arena,
                             NovaMemoryStats *stats);
void nova_memory_reset_stats(NovaMemoryArena *arena);
void nova_memory_print_stats(NovaMemoryArena *arena);

// Debug and profiling
void nova_memory_enable_profiling(NovaMemoryArena *arena, int enable);
void nova_memory_dump_blocks(NovaMemoryArena *arena, const char *filename);
void nova_memory_check_leaks(NovaMemoryArena *arena);

// Memory advisors
size_t nova_memory_advisor_recommend_size(NovaOperationPlanner *planner);
NovaMemoryType nova_memory_advisor_recommend_type(void);
int nova_memory_advisor_should_use_unified(NovaMemoryArena *arena);

#endif // NOVA_MEMORY_ARENA_H
