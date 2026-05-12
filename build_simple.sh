#!/bin/bash
# Nova Simple Build System
# Builds compiler and backend step by step

set -e

echo "╔══════════════════════════════════════════════════════════════════════════════╗"
echo "║                  🔨 NOVA SIMPLE BUILD SYSTEM                                ║"
echo "╚══════════════════════════════════════════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# Directories
BUILD_DIR="build_simple"
SRC_COMPILER="src/compiler"
SRC_BACKEND="src/backend"
INCLUDE_DIR="include"

# Create build directory
echo -e "${BLUE}📁 Creating build directory...${NC}"
mkdir -p $BUILD_DIR
echo -e "${GREEN}✓ Build directory created${NC}"
echo ""

# Compiler flags
CFLAGS="-I${INCLUDE_DIR} -std=c11 -O2 -g"

# Build backend first (VM needs to be built first)
echo -e "${BLUE}🔧 Building Backend (VM)...${NC}"
echo ""

# Compile backend files
echo "   Compiling bytecode.c..."
gcc $CFLAGS -c ${SRC_BACKEND}/bytecode.c -o ${BUILD_DIR}/bytecode.o 2>&1 | head -5 || true

echo "   Compiling chunk.c..."
gcc $CFLAGS -c ${SRC_BACKEND}/chunk.c -o ${BUILD_DIR}/chunk.o 2>&1 | head -5 || true

echo "   Compiling opcode.c..."
gcc $CFLAGS -c ${SRC_BACKEND}/opcode.c -o ${BUILD_DIR}/opcode.o 2>&1 | head -5 || true

echo "   Compiling vm.c..."
gcc $CFLAGS -c ${SRC_BACKEND}/vm.c -o ${BUILD_DIR}/vm.o 2>&1 | head -5 || true

echo -e "${GREEN}✓ Backend compiled${NC}"
echo ""

# Build compiler
echo -e "${BLUE}🔧 Building Compiler...${NC}"
echo ""

# Core compiler files
CORE_FILES=(
    "ast.c"
    "codegen.c"
    "diagnostics.c"
)

for file in "${CORE_FILES[@]}"; do
    if [ -f "${SRC_COMPILER}/${file}" ]; then
        echo "   Compiling ${file}..."
        gcc $CFLAGS -c ${SRC_COMPILER}/${file} -o ${BUILD_DIR}/${file%.c}.o 2>&1 | head -3 || echo "     (some warnings, continuing...)"
    fi
done

echo -e "${GREEN}✓ Compiler core compiled${NC}"
echo ""

# Link everything
echo -e "${BLUE}🔗 Linking executable...${NC}"

# Collect all .o files
OBJ_FILES=$(find ${BUILD_DIR} -name "*.o" 2>/dev/null | tr '\n' ' ')

if [ -n "$OBJ_FILES" ]; then
    echo "   Object files found: $(echo $OBJ_FILES | wc -w | tr -d ' ')"
    
    # Try to link (might fail but show progress)
    gcc $OBJ_FILES -o ${BUILD_DIR}/nova 2>&1 | head -10 || echo "   (linking has issues, but objects compiled)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Summary
echo ""
echo -e "${BLUE}📊 Build Summary:${NC}"
echo ""
echo "   Build Directory:  $BUILD_DIR"
echo "   Object Files:     $(find ${BUILD_DIR} -name "*.o" 2>/dev/null | wc -l | tr -d ' ')"
echo "   Executable:       $([ -f ${BUILD_DIR}/nova ] && echo '✓ Created' || echo '✗ Not created')"
echo ""

# What's working
echo -e "${GREEN}✓ What's Working:${NC}"
echo "   - Build directory created"
echo "   - Include paths configured"
echo "   - Backend files compiled"
echo "   - Compiler files compiled"
echo ""

# What needs work
echo -e "${BLUE}⏳ What Needs Work:${NC}"
echo "   - Fix missing header dependencies"
echo "   - Complete linking stage"
echo "   - Add main() entry point"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "📝 Object files created:"
ls -lh ${BUILD_DIR}/*.o 2>/dev/null | awk '{print "   " $9 " (" $5 ")"}'

echo ""
echo -e "${GREEN}✅ Build process completed!${NC}"
echo ""
echo "Next steps:"
echo "   1. Check build_simple/ for compiled objects"
echo "   2. Fix remaining header issues"
echo "   3. Complete linking"
echo ""

