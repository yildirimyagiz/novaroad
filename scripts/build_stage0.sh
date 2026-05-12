#!/bin/bash
set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           STAGE 0 BUILD - Nova Sovereign Compiler           ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Project Root
ROOT="/Users/yldyagz/novaRoad/nova"
cd "$ROOT"

# Build directory
BUILD_DIR="$ROOT/build_stage0"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Clean previous object files to avoid duplicate symbols
echo "Cleaning old object files..."
rm -f *.o

# Compiler source files (relative to $ROOT/src)
# Note: compiler/backend/codegen.c removed to avoid conflict with compiler/codegen.c
SOURCES=(
    compiler/main.c
    compiler/frontend/ast.c
    compiler/codegen.c
    compiler/compiler.c
    compiler/diagnostics.c
    compiler/frontend/lexer.c
    compiler/frontend/parser.c
    compiler/semantic.c
    compiler/sourcemgr.c
    compiler/tokens.c
    compiler/borrow_checker.c
    compiler/dimensions.c
    compiler/module_registry.c
    compiler/physics_constants.c
    compiler/backend/llvm/nova_llvm_backend.c
    compiler/backend/llvm/nova_llvm_codegen.c
    compiler/backend/llvm/nova_llvm_emit.c
    compiler/backend/vm/chunk.c
    backend/bytecode.c
    backend/opcode.c
    backend/vm.c
    std/alloc/alloc.c
    std/file/file_io.c
    runtime/value.c
)

echo "Compiling C sources..."
for src in "${SOURCES[@]}"; do
    echo "  → $src"
    # Use path-based object name to avoid naming collisions
    obj_name=$(echo "$src" | tr '/' '_').o
    
    clang -c "$ROOT/src/$src" \
        -I"$ROOT/src/compiler" \
        -I"$ROOT/include" \
        -I/opt/homebrew/opt/llvm/include \
        -DHAVE_LLVM \
        -O2 \
        -o "$obj_name"
done

echo ""
echo "Linking novac binary..."
clang *.o \
    -L/opt/homebrew/opt/llvm/lib \
    -lLLVM \
    -lstdc++ \
    -lpthread \
    -o novac

echo ""
echo "✅ Stage 0 build complete!"
ls -lh novac

echo "Built version info:"
./novac --version 2>&1 || echo "(CLI version check failed, but binary exists)"
