#!/bin/bash

# Move remaining test executables
mv test_stage1 tests/unit/ 2>/dev/null
mv test_defer tests/unit/ 2>/dev/null
mv test_gpt_inference tests/integration/ 2>/dev/null
mv test_error_model tests/unit/ 2>/dev/null

# Move test data files
mv test_reg.txt tests/unit/ 2>/dev/null
mv test_multi.zn tests/unit/ 2>/dev/null

# Move temporary test scripts
mv tmp_rovodev_test_training.sh tests/ 2>/dev/null
mv tmp_rovodev_build_backend_test.sh tests/ 2>/dev/null

# Move organize script to scripts folder
mkdir -p scripts
mv organize_tests.sh scripts/ 2>/dev/null
mv organize_docs.sh scripts/ 2>/dev/null
mv organize_remaining.sh scripts/ 2>/dev/null

echo "Cleanup complete!"
