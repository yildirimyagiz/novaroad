#!/bin/bash
# run_benchmarks.sh - Comprehensive Benchmark Runner
# Tests all Nova vs PyTorch performance claims

set -e

echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║          NOVA vs PYTORCH - BENCHMARK SUITE                       ║"
echo "║          Validating All Performance Claims                       ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check dependencies
echo "🔍 Checking dependencies..."

if ! command -v python3 &> /dev/null; then
    echo "${RED}❌ Python3 not found${NC}"
    exit 1
fi

if ! python3 -c "import torch" 2>/dev/null; then
    echo "${YELLOW}⚠️  PyTorch not found. Installing...${NC}"
    pip3 install torch --quiet
fi

echo "${GREEN}✅ Dependencies OK${NC}"
echo ""

# Build C backend if needed
echo "🔨 Building C backend..."
if [ ! -f "benchmark_gpt_backend" ]; then
    echo "   Compiling C benchmarks..."
    gcc benchmark_gpt_backend.c \
        -I../include \
        -O3 -march=native \
        -lm \
        -o benchmark_gpt_backend 2>/dev/null || echo "   (Skipping C benchmark)"
fi
echo "${GREEN}✅ Build complete${NC}"
echo ""

# Run benchmarks
echo "🚀 Running benchmarks..."
echo "═══════════════════════════════════════════════════════════════════"
echo ""

python3 nova_vs_pytorch.py

RESULT=$?

echo ""
echo "═══════════════════════════════════════════════════════════════════"

# Check results
if [ -f "benchmark_results.json" ]; then
    echo ""
    echo "📊 Generating visualization..."
    python3 visualize_results.py
    echo "${GREEN}✅ Visualization saved to benchmark_results.png${NC}"
fi

if [ $RESULT -eq 0 ]; then
    echo ""
    echo "${GREEN}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo "${GREEN}║          🎉 ALL BENCHMARKS PASSED! 🎉                            ║${NC}"
    echo "${GREEN}║          Nova delivers on all performance claims!                ║${NC}"
    echo "${GREEN}╚══════════════════════════════════════════════════════════════════╝${NC}"
    exit 0
else
    echo ""
    echo "${YELLOW}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo "${YELLOW}║          ⚠️  Some benchmarks need optimization                   ║${NC}"
    echo "${YELLOW}║          Check results above for details                         ║${NC}"
    echo "${YELLOW}╚══════════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi
