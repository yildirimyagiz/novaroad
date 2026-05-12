#!/bin/bash
# Nova Compiler Build Script
set -e

echo "=== Nova Compiler Build Script ==="
echo

# Detect OS
OS=$(uname -s)
echo "Platform: $OS"

# Compiler settings
CC=${CC:-gcc}
CXX=${CXX:-g++}
CFLAGS="-std=c11 -Wall -Wextra -O2 -fPIC -Iinclude -Isrc"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -fPIC -Iinclude -Isrc"

# Create build directory
mkdir -p build/obj/compiler
mkdir -p build/obj/runtime
mkdir -p build/obj/stdlib
mkdir -p build/lib
mkdir -p build/bin

echo "Step 1: Compiling compiler core..."
for src in src/compiler/core/*.c; do
  obj="build/obj/compiler/$(basename ${src%.c}.o)"
  echo "  CC $(basename $src)"
  $CC $CFLAGS -c "$src" -o "$obj"
done

echo
echo "Step 2: Compiling runtime..."
for src in src/runtime/*.c; do
  obj="build/obj/runtime/$(basename ${src%.c}.o)"
  echo "  CC $(basename $src)"
  $CC $CFLAGS -c "$src" -o "$obj"
done

# Runtime subdirectories
for src in src/runtime/sandbox/*.c src/runtime/telemetry/*.c; do
  if [ -f "$src" ]; then
    obj="build/obj/runtime/$(basename ${src%.c}.o)"
    echo "  CC $(basename $src)"
    $CC $CFLAGS -c "$src" -o "$obj"
  fi
done

echo
echo "Step 3: Compiling standard library..."
for src in stdlib/*.c; do
  if [ -f "$src" ]; then
    obj="build/obj/stdlib/$(basename ${src%.c}.o)"
    echo "  CC $(basename $src)"
    $CC $CFLAGS -c "$src" -o "$obj"
  fi
done

echo
echo "Step 4: Creating libraries..."
ar rcs build/lib/libnova_compiler.a build/obj/compiler/*.o
echo "  ✓ libnova_compiler.a"

ar rcs build/lib/libnova_runtime.a build/obj/runtime/*.o
echo "  ✓ libnova_runtime.a"

ar rcs build/lib/libnova_stdlib.a build/obj/stdlib/*.o
echo "  ✓ libnova_stdlib.a"

echo
echo "Step 5: Linking compiler executable..."
$CC -o build/bin/novac \
  build/lib/libnova_compiler.a \
  build/lib/libnova_runtime.a \
  build/lib/libnova_stdlib.a \
  -lm -lpthread
echo "  ✓ novac"

echo
echo "=== Build Summary ==="
echo "Executables:"
ls -lh build/bin/
echo
echo "Libraries:"
ls -lh build/lib/
echo
echo "✅ Build complete!"
