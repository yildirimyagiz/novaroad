#!/bin/bash

# Move object files to appropriate build directories
mkdir -p build_artifacts

# Move .o files
mv *.o build_artifacts/ 2>/dev/null

# Move remaining C source files (non-test)
mkdir -p src/legacy
mv defer_raii.c src/legacy/ 2>/dev/null
mv error_model.c src/legacy/ 2>/dev/null
mv borrow_checker.c src/compiler/ 2>/dev/null
mv package_manager.c src/tools/ 2>/dev/null
mv performance_tuning.c src/profiling/ 2>/dev/null
mv PHYSICS_MODULE_IMPLEMENTATION_CODE.c src/physics/ 2>/dev/null

echo "Build artifacts cleaned!"
