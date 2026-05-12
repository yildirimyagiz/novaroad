#!/bin/bash

# Create test organization structure
mkdir -p tests/{unit,integration,bootstrap,stability}

# Move test files
mv simple_test.c tests/unit/ 2>/dev/null
mv simple_test_runner.c tests/unit/ 2>/dev/null
mv test_framework.c tests/unit/ 2>/dev/null
mv test_borrow_checker.c tests/unit/ 2>/dev/null
mv test_borrow_checker_simple.c tests/unit/ 2>/dev/null

# ML tests
mv test_ml_complete.c tests/integration/ 2>/dev/null
mv test_ml_simple.c tests/integration/ 2>/dev/null

# Gate tests (important system tests)
mv abi_gate_tests.c tests/integration/ 2>/dev/null
mv bootstrap_gate_tests.c tests/bootstrap/ 2>/dev/null
mv final_release_gate_verification.c tests/integration/ 2>/dev/null

# Stability tests
mv cli_stability_tests.c tests/stability/ 2>/dev/null

# Move executables (if any remain in root)
mv simple_test tests/unit/ 2>/dev/null
mv simple_test_runner tests/unit/ 2>/dev/null
mv test_borrow_checker_simple tests/unit/ 2>/dev/null
mv abi_gate_tests tests/integration/ 2>/dev/null
mv bootstrap_gate_tests tests/bootstrap/ 2>/dev/null
mv cli_stability_tests tests/stability/ 2>/dev/null
mv final_release_gate_verification tests/integration/ 2>/dev/null

# Move other test-related files
mv test_*.nova tests/unit/ 2>/dev/null
mv *_test tests/unit/ 2>/dev/null

echo "Test files organized!"
