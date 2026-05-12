# Nova Project Makefile
# Alternative build system using plain Make

CC := clang
CXX := clang++
AS := nasm
AR := ar
LD := ld

CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
ASFLAGS := -f elf64
LDFLAGS := 

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/lib

# Detect architecture
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
    NOVA_ARCH := x86_64
else ifeq ($(ARCH),aarch64)
    NOVA_ARCH := aarch64
else ifeq ($(ARCH),arm64)
    NOVA_ARCH := aarch64
else
    NOVA_ARCH := generic
endif

.PHONY: all clean kernel runtime compiler ai tools tests install

all: kernel runtime compiler ai tools

kernel:
	@echo "Building Nova kernel for $(NOVA_ARCH)..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	@$(MAKE) -C src/kernel

runtime:
	@echo "Building Nova runtime..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	@$(MAKE) -C src/runtime

compiler:
	@echo "Building Nova compiler..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	@$(MAKE) -C src/compiler

ai:
	@echo "Building Nova AI subsystem..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	@$(MAKE) -C src/ai

tools:
	@echo "Building Nova tools..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
	@$(MAKE) -C tools

tests:
	@echo "Building Nova tests..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
	@$(MAKE) -C tests

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

install:
	@echo "Installing Nova..."
	@install -d /usr/local/include
	@cp -r include/* /usr/local/include/
	@install -d /usr/local/lib
	@cp $(LIB_DIR)/*.a /usr/local/lib/ 2>/dev/null || true
	@install -d /usr/local/bin
	@cp $(BIN_DIR)/* /usr/local/bin/ 2>/dev/null || true

help:
	@echo "Nova Project Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build all components (default)"
	@echo "  kernel   - Build kernel only"
	@echo "  runtime  - Build runtime only"
	@echo "  compiler - Build compiler only"
	@echo "  ai       - Build AI subsystem only"
	@echo "  tools    - Build tools only"
	@echo "  tests    - Build tests"
	@echo "  clean    - Clean build artifacts"
	@echo "  install  - Install Nova system-wide"
	@echo "  help     - Show this message"

# ══════════════════════════════════════════════════════════════════════════════
# Unified Benchmark System (v4)
# ══════════════════════════════════════════════════════════════════════════════

PLATFORM := $(shell python3 scripts/detect_platform.py)
BASELINE_NOVA := baselines/$(PLATFORM).json
BASELINE_NATIVE := baselines/native_$(PLATFORM).json
PACKED_BASELINE := baselines/pack_$(PLATFORM).json

.PHONY: bench-unified bench-unified-ci bench-unified-baseline

bench-unified:
	@echo "Running local unified benchmark for $(PLATFORM)..."
	python3 scripts/bench_report_v4.py \
		--bench benchmarks/unit_bench_working.zn \
		--runs 10 --warmup 2 \
		--out BENCHMARK_RESULTS_V4.md \
		--json NOVA.json \
		--nova ./nova
	python3 scripts/bench_native_gemm_report.py \
		--make-target bench-hgemv-build --bin bench/bench_hgemv \
		--make-target bench-hgemm-build --bin bench/bench_hgemm \
		--threads 1 4 \
		--runs 5 --warmup 2 \
		--out NATIVE_V4_REPORT.md --json NATIVE.json
	python3 scripts/render_unified_bench_comment.py \
		--nova NOVA.json --native NATIVE.json --out bench_comment.md

bench-unified-ci:
	@echo "Running CI unified regression test for $(PLATFORM)..."
	@test -f "$(BASELINE_NOVA)" || (echo "❌ Missing baseline: $(BASELINE_NOVA)" && exit 1)
	@test -f "$(BASELINE_NATIVE)" || (echo "❌ Missing baseline: $(BASELINE_NATIVE)" && exit 1)
	python3 scripts/bench_report_v4.py \
		--bench benchmarks/unit_bench_working.zn \
		--runs 20 --warmup 3 \
		--baseline-json "$(BASELINE_NOVA)" \
		--max-regression-pct 3.0 --fail-on-regression \
		--out BENCHMARK_RESULTS_V4.md \
		--json NOVA.json \
		--nova ./nova
	python3 scripts/bench_native_gemm_report.py \
		--make-target bench-hgemv-build --bin bench/bench_hgemv \
		--make-target bench-hgemm-build --bin bench/bench_hgemm \
		--threads 1 4 \
		--runs 10 --warmup 5 \
		--baseline-json "$(BASELINE_NATIVE)" \
		--max-regression-pct 3.0 --fail-on-regression \
		--out NATIVE_V4_REPORT.md --json NATIVE.json
	python3 scripts/render_unified_bench_comment.py \
		--nova NOVA.json --native NATIVE.json --out bench_comment.md

bench-unified-pack-ci:
	@echo "Running CI unified regression test using PACKED baseline for $(PLATFORM)..."
	@test -f "$(PACKED_BASELINE)" || (echo "❌ Missing packed baseline: $(PACKED_BASELINE)" && exit 1)
	python3 scripts/bench_report_v4.py \
		--bench benchmarks/unit_bench_working.zn \
		--runs 20 --warmup 3 \
		--baseline-pack "$(PACKED_BASELINE)" \
		--max-regression-pct 3.0 --fail-on-regression \
		--out BENCHMARK_RESULTS_V4.md \
		--json NOVA.json \
		--nova ./nova
	python3 scripts/bench_native_gemm_report.py \
		--make-target bench-hgemv-build --bin bench/bench_hgemv \
		--make-target bench-hgemm-build --bin bench/bench_hgemm \
		--threads 1 4 \
		--runs 10 --warmup 5 \
		--baseline-pack "$(PACKED_BASELINE)" \
		--max-regression-pct 3.0 --fail-on-regression \
		--out NATIVE_V4_REPORT.md --json NATIVE.json
	python3 scripts/render_unified_bench_comment.py \
		--nova NOVA.json --native NATIVE.json --out bench_comment.md

bench-unified-baseline:
	@echo "Generating unified baselines for $(PLATFORM)..."
	@mkdir -p baselines
	python3 scripts/bench_report_v4.py \
		--bench benchmarks/unit_bench_working.zn \
		--runs 30 --warmup 5 \
		--out "BENCHMARK_BASELINE_$(PLATFORM).md" \
		--json "$(BASELINE_NOVA)" \
		--nova ./nova
	python3 scripts/bench_native_gemm_report.py \
		--make-target bench-hgemv-build --bin bench/bench_hgemv \
		--make-target bench-hgemm-build --bin bench/bench_hgemm \
		--threads 1 4 \
		--runs 20 --warmup 5 \
		--out "NATIVE_BASELINE_$(PLATFORM).md" \
		--json "$(BASELINE_NATIVE)"
	python3 scripts/pack_baseline.py \
		--nova "$(BASELINE_NOVA)" \
		--native "$(BASELINE_NATIVE)" \
		--out "$(PACKED_BASELINE)"

.PHONY: bench-history bench-trend

bench-history:
	@echo "Logging performance history for $(PLATFORM)..."
	python3 scripts/log_bench_history.py \
		--nova NOVA.json \
		--native NATIVE.json \
		--log bench_history.jsonl

bench-trend:
	@echo "Summarizing performance trends for $(PLATFORM)..."
	python3 scripts/summarize_bench_trend.py \
		--log bench_history.jsonl \
		--out bench_trend.md
