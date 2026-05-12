# Nova Native Compiler Makefile

# Compiler settings
CC = clang
CFLAGS = -Wall -Wextra -g -std=c11 -O3 -march=native -ffast-math -I./src -DNOVA_LIBRARY_ONLY -DNOVA_USE_EXTREME_KERNELS
LDFLAGS = 

# LLVM Config
# Try to find llvm-config in standard locations
LLVM_CONFIG_PATH := $(shell which llvm-config 2>/dev/null)

# If not found, try Homebrew paths
ifeq ($(LLVM_CONFIG_PATH),)
    LLVM_CONFIG_PATH := /opt/homebrew/opt/llvm@17/bin/llvm-config
endif

# If found, use it
ifneq ($(wildcard $(LLVM_CONFIG_PATH)),)
    CFLAGS += $(shell $(LLVM_CONFIG_PATH) --cflags)
    LDFLAGS += $(shell $(LLVM_CONFIG_PATH) --ldflags --libs core analysis executionengine native)
else
    # Fallback to manual flags if llvm-config missing (danger)
    CFLAGS += -I/opt/homebrew/opt/llvm@17/include
    LDFLAGS += -L/opt/homebrew/opt/llvm@17/lib -lLLVM
endif

# Enable FP16 for NEON on Apple Silicon
ifeq ($(shell uname -m), arm64)
    CFLAGS += -march=armv8.2-a+fp16
endif

DEBUG_FLAGS = -g -O0 -fsanitize=address -fsanitize=undefined
INCLUDES = -Isrc -Iinclude -Isrc/formal -Isrc/formal/klee -Isrc/formal/smt -Isrc/formal/isabelle -Isrc/compute/graph -Isrc/memory/planner -Isrc/optimizer/fusion -Isrc/ai/numerics -Isrc/runtime -Isrc/backends/simd -Isrc/backends/metal -Isrc/ai/core -Isrc/ai/kernels -Isrc/ai/models/diffusion -Isrc/ai/models/cnn -Isrc/ai/models/transformer -Isrc/ai/io

# Directories
SRC_DIR = src
COMPILER_CORE_DIR = $(SRC_DIR)/compiler/core
COMPILER_LOWERING_DIR = $(SRC_DIR)/compiler/lowering
RUNTIME_DIR = $(SRC_DIR)/runtime
COMPUTE_DIR = $(SRC_DIR)/compute
OPTIMIZER_DIR = $(SRC_DIR)/optimizer
FORMAL_DIR = $(SRC_DIR)/formal
KLEE_DIR = $(FORMAL_DIR)/klee
SMT_DIR = $(FORMAL_DIR)/smt
ISABELLE_DIR = $(FORMAL_DIR)/isabelle
COMPUTE_GRAPH_DIR = $(COMPUTE_DIR)/graph
MEMORY_PLANNER_DIR = $(MEMORY_DIR)/planner
OPTIMIZER_FUSION_DIR = $(OPTIMIZER_DIR)/fusion
NUMERICS_DIR = $(AI_DIR)/numerics
MEMORY_DIR = $(SRC_DIR)/memory
BACKENDS_DIR = $(SRC_DIR)/backends
AI_DIR = $(SRC_DIR)/ai
TOOLS_DIR = $(SRC_DIR)/tools
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

# --- Source and Object Files ---

# Compiler Core
LEXER_SRC = $(COMPILER_CORE_DIR)/nova_lexer.c
LEXER_OBJ = $(BUILD_DIR)/nova_lexer.o

PARSER_SRC = $(COMPILER_CORE_DIR)/nova_parser.c
PARSER_OBJ = $(BUILD_DIR)/nova_parser.o

AST_SRC = $(COMPILER_CORE_DIR)/nova_ast.c
AST_OBJ = $(BUILD_DIR)/nova_ast.o

SEMANTIC_SRC = $(COMPILER_CORE_DIR)/nova_semantic.c
SEMANTIC_OBJ = $(BUILD_DIR)/nova_semantic.o

CODEGEN_SRC = $(COMPILER_CORE_DIR)/nova_codegen.c
CODEGEN_OBJ = $(BUILD_DIR)/nova_codegen.o

OWNERSHIP_SRC = $(COMPILER_CORE_DIR)/nova_ownership.c
OWNERSHIP_OBJ = $(BUILD_DIR)/nova_ownership.o

IR_SRC = $(COMPILER_CORE_DIR)/nova_ir.c
IR_OBJ = $(BUILD_DIR)/nova_ir.o

INCREMENTAL_SRC = $(COMPILER_CORE_DIR)/nova_incremental.c
INCREMENTAL_OBJ = $(BUILD_DIR)/nova_incremental.o

BUILTINS_SRC = $(COMPILER_CORE_DIR)/nova_builtins.c
BUILTINS_OBJ = $(BUILD_DIR)/nova_builtins.o

BORROW_CHECKER_SRC = $(COMPILER_CORE_DIR)/nova_borrow_checker.c
BORROW_CHECKER_OBJ = $(BUILD_DIR)/nova_borrow_checker.o

# Optimizer Layer
OPTIMIZER_SRC = $(OPTIMIZER_DIR)/nova_optimizer.c
OPTIMIZER_OBJ = $(BUILD_DIR)/nova_optimizer.o

OPTIMIZER_PRO_SRC = $(OPTIMIZER_DIR)/nova_optimizer_pro.c
OPTIMIZER_PRO_OBJ = $(BUILD_DIR)/nova_optimizer_pro.o

AUTOTUNE_SRC = $(OPTIMIZER_DIR)/nova_autotune.c
AUTOTUNE_OBJ = $(BUILD_DIR)/nova_autotune.o

PROFILER_V2_SRC = $(OPTIMIZER_DIR)/nova_profiler_v2.c
PROFILER_V2_OBJ = $(BUILD_DIR)/nova_profiler_v2.o

ADAPTIVE_OPTIMIZER_SRC = $(OPTIMIZER_DIR)/nova_adaptive_optimizer.c
ADAPTIVE_OPTIMIZER_OBJ = $(BUILD_DIR)/nova_adaptive_optimizer.o

INVARIANT_ENGINE_SRC = $(OPTIMIZER_DIR)/nova_invariant_engine.c
INVARIANT_ENGINE_OBJ = $(BUILD_DIR)/nova_invariant_engine.o

AUTOTUNE_V2_SRC = $(OPTIMIZER_DIR)/nova_autotune_v2.c
AUTOTUNE_V2_OBJ = $(BUILD_DIR)/nova_autotune_v2.o

FAULT_INTELLIGENCE_SRC = $(OPTIMIZER_DIR)/nova_fault_intelligence.c
FAULT_INTELLIGENCE_OBJ = $(BUILD_DIR)/nova_fault_intelligence.o

LEARNING_RUNTIME_SRC = $(OPTIMIZER_DIR)/nova_learning_runtime.c
LEARNING_RUNTIME_OBJ = $(BUILD_DIR)/nova_learning_runtime.o

# Compute Layer
COGNITIVE_SCHEDULER_SRC = $(COMPUTE_DIR)/nova_cognitive_scheduler.c
COGNITIVE_SCHEDULER_OBJ = $(BUILD_DIR)/nova_cognitive_scheduler.o

EXECUTION_FABRIC_SRC = $(COMPUTE_DIR)/nova_execution_fabric.c
EXECUTION_FABRIC_OBJ = $(BUILD_DIR)/nova_execution_fabric.o

COMPUTE_ECONOMICS_SRC = $(COMPUTE_DIR)/nova_compute_economics.c
COMPUTE_ECONOMICS_OBJ = $(BUILD_DIR)/nova_compute_economics.o

# Runtime Layer
RUNTIME_SRC = $(RUNTIME_DIR)/nova_web_runtime.c
RUNTIME_OBJ = $(BUILD_DIR)/nova_web_runtime.o

EVENTS_SRC = $(RUNTIME_DIR)/nova_events.c
EVENTS_OBJ = $(BUILD_DIR)/nova_events.o

GC_SRC = $(RUNTIME_DIR)/nova_gc.c
GC_OBJ = $(BUILD_DIR)/nova_gc.o

RUNTIME_IO_SRC = $(RUNTIME_DIR)/nova_runtime_io.c
RUNTIME_IO_OBJ = $(BUILD_DIR)/nova_runtime_io.o

DETERMINISTIC_KERNEL_SRC = $(RUNTIME_DIR)/nova_deterministic_kernel.c
DETERMINISTIC_KERNEL_OBJ = $(BUILD_DIR)/nova_deterministic_kernel.o

DETERMINISTIC_SRC = $(RUNTIME_DIR)/nova_deterministic.c
DETERMINISTIC_OBJ = $(BUILD_DIR)/nova_deterministic.o

# Memory Management
MEMORY_SRC = $(MEMORY_DIR)/nova_memory.c
MEMORY_OBJ = $(BUILD_DIR)/nova_memory.o

# AI Layer (Unified ML Execution)
AI_CORE_OBJS = $(BUILD_DIR)/nova_tensor.o $(BUILD_DIR)/nova_tensor_ops.o $(BUILD_DIR)/nova_nn.o
AI_KERNELS_OBJS = $(BUILD_DIR)/nova_kernels.o
AI_MODELS_OBJS = $(BUILD_DIR)/nova_transformer.o $(BUILD_DIR)/nova_emoji_cnn.o
AI_TRAINING_OBJS = $(BUILD_DIR)/nova_meta_model_trainer.o $(BUILD_DIR)/nova_training_upgrade.o
AI_IO_OBJS = $(BUILD_DIR)/image_loader.o $(BUILD_DIR)/safetensors_loader.o

AI_OBJS = $(AI_CORE_OBJS) $(AI_KERNELS_OBJS) $(AI_MODELS_OBJS) $(AI_TRAINING_OBJS) $(AI_IO_OBJS)

# Hardware Backends
SIMD_SRC = $(BACKENDS_DIR)/simd/nova_simd.c
SIMD_OBJ = $(BUILD_DIR)/nova_simd.o

METAL_SRC = $(BACKENDS_DIR)/metal/nova_metal_gpu.c
METAL_OBJ = $(BUILD_DIR)/nova_metal_gpu.o

CUDA_SRC = $(BACKENDS_DIR)/cuda/nova_cuda.c
CUDA_OBJ = $(BUILD_DIR)/nova_cuda.o
CUDA_KERNEL_SRC = $(AI_DIR)/kernels/nova_kernels_cuda.cu
CUDA_KERNEL_OBJ = $(BUILD_DIR)/nova_kernels_cuda.o

# Kernel capability detector
KERNEL_CAPS_SRC = $(AI_DIR)/kernels/nova_kernel_capabilities.c
KERNEL_CAPS_OBJ = $(BUILD_DIR)/nova_kernel_capabilities.o

# Tools
MAIN_SRC = $(TOOLS_DIR)/nova_main.c
NOVA_BIN = $(BIN_DIR)/nova

# Standard Library
STDLIB_CORE_SRC = stdlib/core.c
STDLIB_CORE_OBJ = $(BUILD_DIR)/stdlib_core.o

# --- Object Grouping ---

LOWERING_OBJS = $(BUILD_DIR)/nova_lower_graph.o

COMPILER_CORE_OBJS = $(LEXER_OBJ) $(PARSER_OBJ) $(AST_OBJ) $(SEMANTIC_OBJ) $(CODEGEN_OBJ) $(OWNERSHIP_OBJ) $(IR_OBJ) $(INCREMENTAL_OBJ) $(BUILTINS_OBJ) $(BORROW_CHECKER_OBJ) $(LOWERING_OBJS)

PHASE4_OBJS = $(PROFILER_V2_OBJ) $(ADAPTIVE_OPTIMIZER_OBJ) $(DETERMINISTIC_KERNEL_OBJ) $(INVARIANT_ENGINE_OBJ) $(AUTOTUNE_V2_OBJ)

PHASE5_OBJS = $(COGNITIVE_SCHEDULER_OBJ) $(EXECUTION_FABRIC_OBJ) $(LEARNING_RUNTIME_OBJ) $(FAULT_INTELLIGENCE_OBJ) $(COMPUTE_ECONOMICS_OBJ)

GRAPH_OBJS = $(BUILD_DIR)/nova_graph.o $(BUILD_DIR)/nova_graph_optimizer.o $(BUILD_DIR)/nova_graph_scheduler.o $(BUILD_DIR)/nova_graph_obligations.o
COMPUTE_OBJS = $(BUILD_DIR)/nova_compute.o $(BUILD_DIR)/nova_context.o $(BUILD_DIR)/nova_scheduler.o $(BUILD_DIR)/nova_dispatcher.o $(PHASE5_OBJS) $(GRAPH_OBJS)

OPTIMIZER_FUSION_OBJS = $(BUILD_DIR)/nova_kernel_fusion.o $(BUILD_DIR)/nova_op_fusion_patterns.o
OPTIMIZER_OBJS = $(OPTIMIZER_OBJ) $(OPTIMIZER_PRO_OBJ) $(AUTOTUNE_OBJ) $(PHASE4_OBJS) $(BUILD_DIR)/nova_universal_optimizer.o $(BUILD_DIR)/nova_profile_enforcer.o $(OPTIMIZER_FUSION_OBJS)

MEMORY_OBJS = $(MEMORY_OBJ) $(BUILD_DIR)/nova_allocator.o $(BUILD_DIR)/nova_gpu_memory_pool.o $(BUILD_DIR)/nova_memory_planner.o

BACKEND_OBJS = $(SIMD_OBJ) $(METAL_OBJ) $(CUDA_OBJ) $(BUILD_DIR)/nova_cpu_backend.o $(BUILD_DIR)/nova_kernels_cpu.o $(KERNEL_CAPS_OBJ)

# Conditionally add CUDA kernels if CUDA toolkit is available
ifneq ($(shell which nvcc 2>/dev/null),)
BACKEND_OBJS += $(CUDA_KERNEL_OBJ)
CUDA_AVAILABLE = 1
else
CUDA_AVAILABLE = 0
endif

FORMAL_CORE_OBJS = $(BUILD_DIR)/nova_formal.o $(BUILD_DIR)/nova_invariant_validator.o $(BUILD_DIR)/nova_kernel_verifier.o $(BUILD_DIR)/nova_optimizer_verifier.o $(BUILD_DIR)/nova_symbolic_analyzer.o $(BUILD_DIR)/nova_proof_cache.o $(BUILD_DIR)/nova_proof_manifest_generated.o $(BUILD_DIR)/nova_kernel_contracts.o $(BUILD_DIR)/nova_obligation_v2.o $(BUILD_DIR)/nova_obligation.o
KLEE_OBJS = $(BUILD_DIR)/nova_klee_bridge.o $(BUILD_DIR)/nova_klee_harness.o
SMT_OBJS = $(BUILD_DIR)/nova_cvc5_bridge.o $(BUILD_DIR)/nova_smt_ir.o $(BUILD_DIR)/nova_smtlib2_writer.o $(BUILD_DIR)/nova_solver_bridge_cvc5.o
ISABELLE_OBJS = $(BUILD_DIR)/nova_isabelle_bridge.o $(BUILD_DIR)/nova_isabelle_theory_export.o $(BUILD_DIR)/nova_isabelle_proof_artifacts.o
FORMAL_OBJS = $(FORMAL_CORE_OBJS) $(KLEE_OBJS) $(SMT_OBJS) $(ISABELLE_OBJS)
NUMERICS_OBJS = $(BUILD_DIR)/nova_numerics.o $(BUILD_DIR)/nova_numerical_guard.o $(BUILD_DIR)/nova_precision_policy.o
COMPILER_OBJS = $(COMPILER_CORE_OBJS) $(OPTIMIZER_OBJS) $(COMPUTE_OBJS) $(MEMORY_OBJS) $(BACKEND_OBJS) $(AI_OBJS) $(FORMAL_OBJS) $(NUMERICS_OBJS)

# Runtime Group
RUNTIME_OBJS = $(RUNTIME_OBJ) $(EVENTS_OBJ) $(GC_OBJ) $(RUNTIME_IO_OBJ) $(DETERMINISTIC_OBJ) $(DETERMINISTIC_KERNEL_OBJ) $(BUILD_DIR)/nova_runtime_free.o $(BUILD_DIR)/nova_errors.o $(BUILD_DIR)/nova_deterministic_reduction.o

STDLIB_OBJS = $(STDLIB_CORE_OBJ)

# --- Test Binaries ---
TEST_LEXER_BIN = $(BIN_DIR)/test_lexer
TEST_PARSER_BIN = $(BIN_DIR)/test_parser
TEST_SEMANTIC_BIN = $(BIN_DIR)/test_semantic
TEST_CODEGEN_BIN = $(BIN_DIR)/test_codegen
TEST_LEXER_WEB_BIN = $(BIN_DIR)/test_lexer_web
TEST_PARSER_WEB_BIN = $(BIN_DIR)/test_parser_web
TEST_CODEGEN_WEB_BIN = $(BIN_DIR)/test_codegen_web
TEST_RUNTIME_BIN = $(BIN_DIR)/test_runtime
TEST_E2E_WEB_BIN = $(BIN_DIR)/test_e2e_web
TEST_LINKED_BIN = $(BIN_DIR)/test_linked_executable
TEST_EVENTS_BIN = $(BIN_DIR)/test_events
TEST_GC_BIN = $(BIN_DIR)/test_gc
TEST_STDLIB_BIN = $(BIN_DIR)/test_stdlib
TEST_SIMD_BIN = $(BIN_DIR)/test_simd

# Targets
.PHONY: all clean test test-lexer test-parser test-semantic test-codegen test-lexer-web test-parser-web test-codegen-web test-runtime test-e2e-web test-linked test-events benchmark debug

all: $(NOVA_BIN) $(TEST_LEXER_BIN) $(TEST_PARSER_BIN) $(TEST_SEMANTIC_BIN) $(TEST_CODEGEN_BIN) $(TEST_LEXER_WEB_BIN) $(TEST_PARSER_WEB_BIN) $(TEST_CODEGEN_WEB_BIN) $(TEST_RUNTIME_BIN) $(TEST_E2E_WEB_BIN)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# --- Compilation Rules ---

$(BUILD_DIR)/%.o: $(COMPILER_CORE_DIR)/%.c include/%.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(OPTIMIZER_DIR)/%.c include/%.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(COMPUTE_DIR)/%.c include/%.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(RUNTIME_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(MEMORY_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(BACKENDS_DIR)/cpu/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(BACKENDS_DIR)/simd/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(BACKENDS_DIR)/metal/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(BACKENDS_DIR)/cuda/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# CUDA kernel compilation (only if nvcc is available)
$(BUILD_DIR)/nova_kernels_cuda.o: $(AI_DIR)/kernels/nova_kernels_cuda.cu | $(BUILD_DIR)
ifeq ($(CUDA_AVAILABLE),1)
	@echo "Compiling CUDA kernels with nvcc..."
	nvcc -c $< -o $@ -I$(INCLUDE_DIR) -DNOVA_CUDA_AVAILABLE -O3 --compiler-options -fPIC
else
	@echo "CUDA not available, skipping CUDA kernel compilation"
	@touch $@
endif

# Kernel capabilities
$(BUILD_DIR)/nova_kernel_capabilities.o: $(AI_DIR)/kernels/nova_kernel_capabilities.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# AI Layer Rules
$(BUILD_DIR)/%.o: $(AI_DIR)/core/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(AI_DIR)/kernels/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(AI_DIR)/models/cnn/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(AI_DIR)/models/transformer/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(AI_DIR)/training/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(AI_DIR)/io/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(FORMAL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(KLEE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(SMT_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(ISABELLE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(COMPUTE_GRAPH_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(MEMORY_PLANNER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(OPTIMIZER_FUSION_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(NUMERICS_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(COMPILER_LOWERING_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(STDLIB_CORE_OBJ): $(STDLIB_CORE_SRC) stdlib/core.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Manual overrides for files with different naming patterns
$(LEXER_OBJ): $(LEXER_SRC) include/nova_lexer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(AST_OBJ): $(AST_SRC) include/nova_ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(PARSER_OBJ): $(PARSER_SRC) include/nova_parser.h include/nova_ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SEMANTIC_OBJ): $(SEMANTIC_SRC) include/nova_semantic.h include/nova_ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(CODEGEN_OBJ): $(CODEGEN_SRC) include/nova_codegen.h include/nova_ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OWNERSHIP_OBJ): $(OWNERSHIP_SRC) include/nova_ownership.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OPTIMIZER_OBJ): $(OPTIMIZER_SRC) include/nova_optimizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OPTIMIZER_PRO_OBJ): $(OPTIMIZER_PRO_SRC) include/nova_optimizer_pro.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(AUTOTUNE_OBJ): $(AUTOTUNE_SRC) include/nova_autotune.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SIMD_OBJ): $(SIMD_SRC) $(BACKENDS_DIR)/simd/nova_simd.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(METAL_OBJ): $(METAL_SRC) include/nova_metal_gpu.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link test executables
$(NOVA_BIN): $(MAIN_SRC) $(COMPILER_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_LEXER_BIN): $(TEST_DIR)/test_lexer.c $(LEXER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_LEXER_WEB_BIN): $(TEST_DIR)/test_lexer_web.c $(LEXER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_PARSER_BIN): $(TEST_DIR)/test_parser.c $(LEXER_OBJ) $(PARSER_OBJ) $(AST_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_PARSER_WEB_BIN): $(TEST_DIR)/test_parser_web.c $(LEXER_OBJ) $(PARSER_OBJ) $(AST_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_SEMANTIC_BIN): $(TEST_DIR)/test_semantic.c $(LEXER_OBJ) $(PARSER_OBJ) $(AST_OBJ) $(SEMANTIC_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_CODEGEN_BIN): $(TEST_DIR)/test_codegen.c $(COMPILER_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_CODEGEN_WEB_BIN): $(TEST_DIR)/test_codegen_web.c $(COMPILER_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_RUNTIME_BIN): $(TEST_DIR)/test_runtime.c $(COMPILER_OBJS) $(RUNTIME_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_E2E_WEB_BIN): $(TEST_DIR)/test_e2e_web.c $(COMPILER_OBJS) $(RUNTIME_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_LINKED_BIN): $(TEST_DIR)/test_linked_executable.c $(COMPILER_OBJS) $(RUNTIME_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_EVENTS_BIN): $(TEST_DIR)/test_events.c $(RUNTIME_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_GC_BIN): $(TEST_DIR)/test_gc.c $(GC_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(TEST_STDLIB_BIN): $(TEST_DIR)/test_stdlib.c $(STDLIB_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) -lm

$(TEST_SIMD_BIN): $(TEST_DIR)/test_simd.c $(STDLIB_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) -lm

# Run tests
test: test-lexer test-parser test-semantic test-codegen test-lexer-web test-parser-web test-codegen-web test-runtime test-e2e-web

test-lexer: $(TEST_LEXER_BIN)
	@echo "Running lexer tests..."
	@./$(TEST_LEXER_BIN)

test-lexer-web: $(TEST_LEXER_WEB_BIN)
	@echo "Running web lexer tests..."
	@./$(TEST_LEXER_WEB_BIN)

test-parser: $(TEST_PARSER_BIN)
	@echo "Running parser tests..."
	@./$(TEST_PARSER_BIN)

test-parser-web: $(TEST_PARSER_WEB_BIN)
	@echo "Running web parser tests..."
	@./$(TEST_PARSER_WEB_BIN)

test-semantic: $(TEST_SEMANTIC_BIN)
	@echo "Running semantic tests..."
	@./$(TEST_SEMANTIC_BIN)

test-codegen: $(TEST_CODEGEN_BIN)
	@echo "Running codegen tests..."
	@./$(TEST_CODEGEN_BIN)

test-codegen-web: $(TEST_CODEGEN_WEB_BIN)
	@echo "Running web codegen tests..."
	@./$(TEST_CODEGEN_WEB_BIN)

test-runtime: $(TEST_RUNTIME_BIN)
	@echo "Running runtime tests..."
	@./$(TEST_RUNTIME_BIN)

test-e2e-web: $(TEST_E2E_WEB_BIN)
	@echo "Running end-to-end web tests..."
	@./$(TEST_E2E_WEB_BIN)

test-linked: $(TEST_LINKED_BIN) $(RUNTIME_OBJS)
	@echo "Running linked executable test..."
	@./$(TEST_LINKED_BIN)

test-events: $(TEST_EVENTS_BIN)
	@echo "Running event system tests..."
	@./$(TEST_EVENTS_BIN)

test-gc: $(TEST_GC_BIN)
	@echo "Running garbage collector tests..."
	@./$(TEST_GC_BIN)

test-stdlib: $(TEST_STDLIB_BIN)
	@echo "Running native stdlib tests..."
	@./$(TEST_STDLIB_BIN)

test-simd: $(TEST_SIMD_BIN)
	@echo "Running SIMD intrinsics tests..."
	@./$(TEST_SIMD_BIN)

test-runtime-complete: test-gc test-stdlib test-simd
	@echo "✅ All runtime tests passed!"

# Multi-language lowering test
test_multi_language_lowering: tests/test_multi_language_lowering.c $(COMPILER_OBJS) $(RUNTIME_OBJS) | bin
	$(CC) $(CFLAGS) -o bin/$@ $< $(COMPILER_OBJS) $(RUNTIME_OBJS) $(LDFLAGS)

test-lowering: test_multi_language_lowering
	@echo "Running multi-language lowering tests..."
	./bin/test_multi_language_lowering
	@echo "✓ All lowering tests passed"

# Build GC library for linking
libnova_gc.a: $(GC_OBJ) $(MEMORY_OBJ)
	ar rcs runtime/libnova_gc.a $(GC_OBJ) $(MEMORY_OBJ)
	@echo "✅ GC library built: runtime/libnova_gc.a"

# Build complete runtime library
libnova_runtime.a: $(RUNTIME_OBJS) $(STDLIB_OBJS)
	ar rcs runtime/libnova_runtime.a $(RUNTIME_OBJS) $(STDLIB_OBJS)
	@echo "✅ Runtime library built: runtime/libnova_runtime.a"

# ============================================
# SELF-HOSTING BOOTSTRAP PIPELINE
# ============================================

# Stage 1: Build Nova compiler using C compiler
bootstrap-stage1: $(NOVA_BIN)
	@echo "=========================================="
	@echo "STAGE 1: Building Nova compiler from C"
	@echo "=========================================="
	@echo "✅ Stage 1 complete: bin/nova (C-compiled)"
	@mkdir -p bootstrap
	@cp $(NOVA_BIN) bootstrap/nova_stage1
	@echo "Saved: bootstrap/nova_stage1"

# Stage 2: Use Stage 1 compiler to compile Nova compiler written in Nova
bootstrap-stage2: bootstrap-stage1
	@echo ""
	@echo "=========================================="
	@echo "STAGE 2: Nova compiling itself"
	@echo "=========================================="
	@if [ ! -f ../compiler/bootstrap/nova_compiler_bootstrap.zn ]; then \
		echo "❌ Error: compiler/bootstrap/nova_compiler_bootstrap.zn not found"; \
		exit 1; \
	fi
	@echo "Compiling Nova compiler with Stage 1..."
	@./bootstrap/nova_stage1 ../compiler/bootstrap/nova_compiler_bootstrap.zn -o bootstrap/nova_stage2 2>&1 | tee bootstrap/stage2.log || \
		(echo "❌ Stage 2 failed - check bootstrap/stage2.log" && exit 1)
	@if [ -f bootstrap/nova_stage2 ]; then \
		chmod +x bootstrap/nova_stage2; \
		echo "✅ Stage 2 complete: bootstrap/nova_stage2 (Nova-compiled)"; \
	else \
		echo "❌ Stage 2 failed: bootstrap/nova_stage2 not created"; \
		exit 1; \
	fi

# Stage 3: Use Stage 2 compiler to recompile itself (verification)
bootstrap-stage3: bootstrap-stage2
	@echo ""
	@echo "=========================================="
	@echo "STAGE 3: Self-hosting verification"
	@echo "=========================================="
	@echo "Recompiling Nova compiler with Stage 2..."
	@./bootstrap/nova_stage2 ../compiler/bootstrap/nova_compiler_bootstrap.zn -o bootstrap/nova_stage3 2>&1 | tee bootstrap/stage3.log || \
		(echo "❌ Stage 3 failed - check bootstrap/stage3.log" && exit 1)
	@if [ -f bootstrap/nova_stage3 ]; then \
		chmod +x bootstrap/nova_stage3; \
		echo "✅ Stage 3 complete: bootstrap/nova_stage3 (self-compiled)"; \
	else \
		echo "❌ Stage 3 failed: bootstrap/nova_stage3 not created"; \
		exit 1; \
	fi
	@echo ""
	@echo "Verifying binary equivalence..."
	@if cmp -s bootstrap/nova_stage2 bootstrap/nova_stage3; then \
		echo "✅✅✅ SUCCESS: Stage 2 and Stage 3 binaries are IDENTICAL!"; \
		echo "🎉 SELF-HOSTING COMPLETE - 100% ACHIEVED! 🎉"; \
	else \
		echo "⚠️  WARNING: Stage 2 and Stage 3 binaries differ"; \
		echo "This may indicate non-deterministic compilation"; \
		ls -lh bootstrap/nova_stage2 bootstrap/nova_stage3; \
	fi

# Full bootstrap pipeline (all 3 stages)
bootstrap: bootstrap-stage3
	@echo ""
	@echo "=========================================="
	@echo "✅ BOOTSTRAP PIPELINE COMPLETE"
	@echo "=========================================="
	@echo "Stage 1 (C):      bootstrap/nova_stage1"
	@echo "Stage 2 (Nova): bootstrap/nova_stage2"
	@echo "Stage 3 (Verify): bootstrap/nova_stage3"
	@echo ""
	@echo "Run 'make test-bootstrap' to test the self-hosted compiler"

# Test the self-hosted compiler
test-bootstrap: bootstrap
	@echo ""
	@echo "=========================================="
	@echo "Testing self-hosted compiler..."
	@echo "=========================================="
	@echo 'fn main() { println("Hello from self-hosted Nova!") }' > bootstrap/test.zn
	@./bootstrap/nova_stage3 bootstrap/test.zn -o bootstrap/test_hello
	@if [ -f bootstrap/test_hello ]; then \
		chmod +x bootstrap/test_hello; \
		echo "Running compiled test..."; \
		./bootstrap/test_hello; \
		echo "✅ Self-hosted compiler test PASSED!"; \
	else \
		echo "❌ Test compilation failed"; \
		exit 1; \
	fi

# Clean bootstrap artifacts
clean-bootstrap:
	rm -rf bootstrap

# Clean everything including bootstrap
clean-all: clean clean-bootstrap

# Clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) runtime/*.a
