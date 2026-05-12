#!/bin/bash
# Complete build with main entry point

set -e

echo "╔══════════════════════════════════════════════════════════════════════════════╗"
echo "║                  🔨 NOVA COMPLETE BUILD                                     ║"
echo "╚══════════════════════════════════════════════════════════════════════════════╝"
echo ""

BUILD_DIR="build_simple"
CFLAGS="-Iinclude -std=c11 -O2"

# Compile main
echo "🔨 Compiling main entry point..."
gcc $CFLAGS -c src/compiler/main_simple.c -o ${BUILD_DIR}/main_simple.o
echo "✓ Main compiled"
echo ""

# Link everything
echo "🔗 Linking final executable..."
gcc ${BUILD_DIR}/*.o -o ${BUILD_DIR}/nova 2>&1 | grep -v "warning" || true
echo ""

if [ -f "${BUILD_DIR}/nova" ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  ✅ BUILD SUCCESS!"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "📦 Executable created:"
    ls -lh ${BUILD_DIR}/nova
    echo ""
    echo "🧪 Testing executable:"
    ${BUILD_DIR}/nova
    echo ""
    echo "🎉 Nova compiler is ready!"
    echo ""
    echo "To compile a Nova program:"
    echo "   ${BUILD_DIR}/nova apps/gpu_army_full/app.zn"
    echo ""
else
    echo "❌ Linking failed - but we have object files!"
    echo ""
    echo "Object files created:"
    ls -lh ${BUILD_DIR}/*.o
    echo ""
fi

