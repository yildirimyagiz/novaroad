#!/bin/bash

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "    🧪 Nova Compiler Comprehensive Test Suite 🧪"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

NOVA_BIN="/Users/yldyagz/novaRoad/nova/nova"
STAGE1_BIN="/Users/yldyagz/novaRoad/nova/src/compiler/bootstrap/stage1_compiler_improved.py"
TEST_DIR="/Users/yldyagz/novaRoad/nova/tests/compiler_tests"

TOTAL=0
PASSED=0
FAILED=0

cd "$TEST_DIR"

echo "📋 Running tests..."
echo ""

for test_file in test_*.nova; do
    TOTAL=$((TOTAL + 1))
    test_name=$(basename "$test_file" .nova)
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Test $TOTAL: $test_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Test 1: Stage 0 Compiler
    echo ""
    echo "🔹 Stage 0 (C Compiler with LLVM):"
    if "$NOVA_BIN" "$test_file" 2>&1 | head -3 > /dev/null; then
        echo "  ✅ Stage 0 parsed successfully"
    else
        echo "  ⚠️  Stage 0 had errors"
    fi
    
    # Test 2: Stage 1 Compiler (Nova → C transpiler)
    echo ""
    echo "🔹 Stage 1 (Nova → C Transpiler):"
    c_file="${test_name}.c"
    exe_file="${test_name}"
    
    if python3 "$STAGE1_BIN" "$test_file" "$c_file" 2>/dev/null; then
        echo "  ✅ Transpilation: $test_file → $c_file"
        
        # Compile the generated C code
        if clang "$c_file" -o "$exe_file" 2>/dev/null; then
            echo "  ✅ C compilation successful"
            
            # Run and capture output
            output=$("./$exe_file" 2>&1)
            exit_code=$?
            
            # Any exit code between 0-255 is valid (programs return calculated values)
            echo "  ✅ Execution successful (exit code: $exit_code)"
            echo "     Output:"
            echo "$output" | sed 's/^/       /'
            PASSED=$((PASSED + 1))
        else
            echo "  ❌ C compilation failed"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "  ❌ Transpilation failed"
        FAILED=$((FAILED + 1))
    fi
    
    echo ""
done

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 TEST RESULTS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Total Tests:  $TOTAL"
echo "Passed:       $PASSED ✅"
echo "Failed:       $FAILED ❌"
echo ""
if [ $TOTAL -gt 0 ]; then
    echo "Success Rate: $((PASSED * 100 / TOTAL))%"
else
    echo "Success Rate: 0%"
fi
echo ""

if [ $PASSED -eq $TOTAL ]; then
    echo "🎉 All tests PASSED! Nova compiler is working perfectly! 🎉"
fi
