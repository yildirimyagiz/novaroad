# Nova Kernels - Cross-Platform Makefile
# Supports: macOS (ARM/Intel), Linux (x86_64/ARM), Windows

# Detect OS
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Compiler
CC = gcc
CXX = g++
NVCC = nvcc

# Base paths
SRC_DIR = .
INC_DIR = ../../../include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib

# Common flags
CFLAGS = -Wall -Wextra -O3 -fPIC -I$(INC_DIR)
LDFLAGS = -lm

# Platform-specific configuration
ifeq ($(UNAME_S),Darwin)
    # macOS
    PLATFORM = macos
    ifeq ($(UNAME_M),arm64)
        # Apple Silicon (M1/M2/M3)
        ARCH = arm64
        CFLAGS += -march=armv8-a
        SIMD_FLAGS = -DNOVA_ARM_NEON
    else
        # Intel Mac
        ARCH = x86_64
        CFLAGS += -march=native
        SIMD_FLAGS = -mavx2 -DNOVA_X86_SIMD
    endif
    SHARED_EXT = .dylib
    SHARED_FLAGS = -dynamiclib
else ifeq ($(UNAME_S),Linux)
    # Linux
    PLATFORM = linux
    ifeq ($(UNAME_M),aarch64)
        # ARM64 Linux
        ARCH = arm64
        CFLAGS += -march=armv8-a
        SIMD_FLAGS = -DNOVA_ARM_NEON
    else
        # x86_64 Linux
        ARCH = x86_64
        CFLAGS += -march=native
        SIMD_FLAGS = -mavx2 -DNOVA_X86_SIMD
    endif
    SHARED_EXT = .so
    SHARED_FLAGS = -shared
else
    # Windows (MinGW/Cygwin)
    PLATFORM = windows
    ARCH = x86_64
    SIMD_FLAGS = -mavx2 -DNOVA_X86_SIMD
    SHARED_EXT = .dll
    SHARED_FLAGS = -shared
endif

# Source files
SRCS = nova_kernels.c \
       nova_kernels_simd.c \
       nova_kernels_fused.c \
       nova_kernel_dispatcher.c

# Object files
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Library name
LIB_NAME = libnova_kernels
STATIC_LIB = $(LIB_DIR)/$(LIB_NAME).a
SHARED_LIB = $(LIB_DIR)/$(LIB_NAME)$(SHARED_EXT)

# CUDA support (optional)
USE_CUDA ?= 0
ifeq ($(USE_CUDA),1)
    CUDA_SRCS = nova_kernels_cuda.cu
    CUDA_OBJS = $(CUDA_SRCS:%.cu=$(OBJ_DIR)/%.o)
    OBJS += $(CUDA_OBJS)
    CFLAGS += -DUSE_CUDA
    LDFLAGS += -lcudart
    NVCC_FLAGS = -O3 -arch=sm_75 -I$(INC_DIR)
endif

# Targets
.PHONY: all clean info test benchmark install

all: $(STATIC_LIB) $(SHARED_LIB)

# Create directories
$(OBJ_DIR) $(LIB_DIR):
	@mkdir -p $@

# Compile C files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) $(SIMD_FLAGS) -c $< -o $@

# Compile CUDA files
ifeq ($(USE_CUDA),1)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cu | $(OBJ_DIR)
	@echo "NVCC $<"
	@$(NVCC) $(NVCC_FLAGS) -c $< -o $@
endif

# Static library
$(STATIC_LIB): $(OBJS) | $(LIB_DIR)
	@echo "AR  $@"
	@ar rcs $@ $(OBJS)

# Shared library
$(SHARED_LIB): $(OBJS) | $(LIB_DIR)
	@echo "LD  $@"
	@$(CC) $(SHARED_FLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Clean
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@rm -f *.o *.a *.so *.dylib *.dll

# Info
info:
	@echo "╔═══════════════════════════════════════════════════════════════╗"
	@echo "║           Nova Kernels - Build Configuration               ║"
	@echo "╚═══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Platform:    $(PLATFORM)"
	@echo "Architecture: $(ARCH)"
	@echo "Compiler:    $(CC)"
	@echo "SIMD Flags:  $(SIMD_FLAGS)"
	@echo "CUDA:        $(USE_CUDA)"
	@echo ""
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Static lib:      $(STATIC_LIB)"
	@echo "Shared lib:      $(SHARED_LIB)"
	@echo ""

# Test compilation
test: all
	@echo "╔═══════════════════════════════════════════════════════════════╗"
	@echo "║                  Compilation Test                             ║"
	@echo "╚═══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "✅ All kernels compiled successfully!"
	@echo ""
	@ls -lh $(LIB_DIR)/

# Benchmark (compile benchmark suite)
benchmark: all
	@echo "Building benchmark..."
	@$(CC) $(CFLAGS) ../tests/benchmark_kernels.c \
		-L$(LIB_DIR) -lnova_kernels \
		-o $(BUILD_DIR)/benchmark_kernels \
		$(LDFLAGS)
	@echo "✅ Benchmark built: $(BUILD_DIR)/benchmark_kernels"

# Install (copy to system)
install: all
	@echo "Installing to /usr/local..."
	@sudo cp $(STATIC_LIB) /usr/local/lib/
	@sudo cp $(SHARED_LIB) /usr/local/lib/
	@sudo cp $(INC_DIR)/nova_kernels*.h /usr/local/include/
	@echo "✅ Installed successfully"

# Help
help:
	@echo "Nova Kernels Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build static and shared libraries (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  info       - Show build configuration"
	@echo "  test       - Test compilation"
	@echo "  benchmark  - Build benchmark suite"
	@echo "  install    - Install to /usr/local"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  USE_CUDA=1 - Enable CUDA support"
	@echo ""
	@echo "Examples:"
	@echo "  make all"
	@echo "  make all USE_CUDA=1"
	@echo "  make clean && make all"
