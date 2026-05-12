#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════
# compile_wasm.sh - Nova WebAssembly Build Script
# ═══════════════════════════════════════════════════════════════════════════

# Check for emcc (Emscripten Compiler)
if ! command -v emcc &> /dev/null
then
    echo "❌ emcc not found. Please install Emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

echo "🚀 Building Nova WASM Sovereignty Engine..."

emcc src/wasm/nova_wasm.c \
     src/compute/nova_bridge.c \
     src/compute/nova_mirror_v2.c \
     -I include \
     -s WASM=1 \
     -s EXPORTED_RUNTIME_METHODS='["cwrap", "getValue", "setValue"]' \
     -s ALLOW_MEMORY_GROWTH=1 \
     -s MODULARIZE=1 \
     -s EXPORT_NAME='NovaEngine' \
     -O3 -msimd128 \
     -o build/nova.js

echo "✅ Nova WASM build complete: build/nova.js, build/nova.wasm"
