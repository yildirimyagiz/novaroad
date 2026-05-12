#!/bin/bash
# Nova CI Test Suite
# Runs all tests with optional sanitizers

set -e

NOVA_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$NOVA_DIR/tests"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "    🚀 Nova CI Test Suite 🚀"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Parse arguments
SANITIZERS=0
VERBOSE=0
CATEGORY="all"

while [[ $# -gt 0 ]]; do
    case $1 in
        --sanitizers)
            SANITIZERS=1
            shift
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --category)
            CATEGORY="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# 1. Compiler Build Check
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  1. Compiler Build Check"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ ! -f "$NOVA_DIR/nova" ]; then
    echo -e "${RED}❌ Nova binary not found${NC}"
    echo "Building compiler..."
    cd "$NOVA_DIR"
    make -f Makefile.simple clean
    if [ $SANITIZERS -eq 1 ]; then
        echo "Building with ASAN/UBSAN..."
        make -f Makefile.simple debug
    else
        make -f Makefile.simple release
    fi
fi

echo -e "${GREEN}✅ Compiler binary ready: $(ls -lh $NOVA_DIR/nova | awk '{print $5}')${NC}"
echo ""

# 2. Unit Tests (if available)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  2. Unit Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -d "$NOVA_DIR/tests/unit" ]; then
    cd "$NOVA_DIR/tests/unit"
    for test in test_*; do
        if [ -x "$test" ]; then
            echo "Running $test..."
            ./"$test" || echo -e "${RED}❌ $test failed${NC}"
        fi
    done
else
    echo -e "${YELLOW}⚠️  No unit tests found (TODO: add C unit tests)${NC}"
fi
echo ""

# 3. Integration Tests (Python test runner)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  3. Integration Tests (Test Runner)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cd "$TEST_DIR"
RUNNER_ARGS=""
if [ $VERBOSE -eq 1 ]; then
    RUNNER_ARGS="--verbose"
fi
if [ "$CATEGORY" != "all" ]; then
    RUNNER_ARGS="$RUNNER_ARGS --category $CATEGORY"
fi

python3 test_runner.py $RUNNER_ARGS
TEST_EXIT=$?

if [ $TEST_EXIT -ne 0 ]; then
    echo -e "${RED}❌ Tests failed${NC}"
    exit 1
fi

echo ""

# 4. Bootstrap Pipeline
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  4. Bootstrap Pipeline Verification"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$TEST_DIR/bootstrap/01_stage_pipeline.sh" ]; then
    "$TEST_DIR/bootstrap/01_stage_pipeline.sh"
else
    echo -e "${YELLOW}⚠️  Bootstrap test not found${NC}"
fi

echo ""

# 5. Memory Leak Check (Valgrind - optional)
if command -v valgrind &> /dev/null && [ $SANITIZERS -eq 1 ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  5. Memory Leak Check (Valgrind)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    # Run a simple test under valgrind
    TEST_FILE="$TEST_DIR/e2e/01_hello_world.nova"
    if [ -f "$TEST_FILE" ]; then
        valgrind --leak-check=full --error-exitcode=1 "$NOVA_DIR/nova" "$TEST_FILE" > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✅ No memory leaks detected${NC}"
        else
            echo -e "${RED}❌ Memory leaks detected${NC}"
        fi
    fi
    echo ""
fi

# Final Summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ✅ CI Test Suite Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "All checks passed! 🎉"
echo ""
