#!/bin/bash
# ══════════════════════════════════════════════════════════════════════════════
# NOVA COMPUTE OS - Unified Build & Execution Orchestrator
# ══════════════════════════════════════════════════════════════════════════════

set -e

# Colors for premium look
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
GOLD='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

echo -e "${GOLD}${BOLD}"
echo "  _   _  _____      __  _____ "
echo " | \ | |/ _ \ \    / / |  _  |"
echo " |  \| | | | \ \  / /  | | | |"
echo " | .   | | | |\ \/ /   | | | |"
echo " | |\  | |_| | \  /    \ \_/ /"
echo " |_| \_|\___/   \/      \___/ "
echo -e "      THE COMPUTE OPERATING SYSTEM${NC}"
echo ""

# 1. Build Phase
echo -e "${CYAN}${BOLD}[1/2] 🔨 Building Nova System...${NC}"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null
make -j$(sysctl -n hw.ncpu) > /dev/null
cd ..

if [ ! -f "build/tools/znc" ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Build successful!${NC}"
echo ""

# 2. Execution Phase
INPUT_FILE=${1:-"apps/gpu_army_full/app.zn"}

if [ ! -f "$INPUT_FILE" ]; then
    echo -e "${RED}❌ Input file not found: $INPUT_FILE${NC}"
    echo -e "Usage: ./BUILD_AND_RUN.sh [path/to/app.zn]"
    exit 1
fi

echo -e "${BLUE}${BOLD}[2/2] 🚀 Compiling & Running: ${INPUT_FILE}...${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

./build/tools/znc "$INPUT_FILE" --backend vm

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GOLD}✨ Nova App Lifecycle Completed${NC}"
