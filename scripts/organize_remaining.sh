#!/bin/bash

# Move remaining categorized docs
mv AI_*.md docs/reports/ 2>/dev/null
mv ML_*.md docs/reports/ 2>/dev/null
mv ALL_*.md docs/reports/ 2>/dev/null
mv ASYNC_*.md docs/reports/ 2>/dev/null
mv BACKEND_*.md docs/reports/ 2>/dev/null
mv C_INTEGRATION*.md docs/reports/ 2>/dev/null
mv CODEGEN_*.md docs/reports/ 2>/dev/null
mv COMPILATION_*.md docs/reports/ 2>/dev/null
mv COMPILER_*.md docs/reports/ 2>/dev/null
mv CROSS_PLATFORM*.md docs/reports/ 2>/dev/null
mv ERROR_HANDLING*.md docs/reports/ 2>/dev/null
mv FEATURE_*.md docs/reports/ 2>/dev/null
mv FRONTEND_*.md docs/reports/ 2>/dev/null
mv GENERICS_*.md docs/reports/ 2>/dev/null
mv GPU_*.md docs/reports/ 2>/dev/null
mv MOBILE_*.md docs/reports/ 2>/dev/null
mv NOVA_BOOTSTRAP*.md docs/reports/ 2>/dev/null
mv NOVA_COMPILER*.md docs/reports/ 2>/dev/null
mv OPTIMIZATION_*.md docs/reports/ 2>/dev/null
mv PARSER_*.md docs/reports/ 2>/dev/null
mv PATTERN_MATCHING*.md docs/reports/ 2>/dev/null
mv PHYSICS_*.md docs/reports/ 2>/dev/null
mv PRODUCTION_*.md docs/reports/ 2>/dev/null
mv PYTORCH_*.md docs/reports/ 2>/dev/null
mv RESTORATION_*.md docs/reports/ 2>/dev/null
mv SELF_LEARNING*.md docs/reports/ 2>/dev/null
mv THREE_MODULES*.md docs/reports/ 2>/dev/null
mv TYPE_SYSTEM*.md docs/reports/ 2>/dev/null
mv UNIT_ALGEBRA*.md docs/reports/ 2>/dev/null
mv UNIT_CONVERSION*.md docs/reports/ 2>/dev/null
mv UNIFIED_BENCH*.md docs/benchmarks/ 2>/dev/null
mv ADVANCED_*.md docs/reports/ 2>/dev/null

# Final status and completion reports
mv FINAL_*.md docs/completed/ 2>/dev/null
mv COMPLETE_*.md docs/completed/ 2>/dev/null

# Quick references
mv QUICK_STATUS.md docs/reports/ 2>/dev/null
mv SUMMARY.md docs/reports/ 2>/dev/null

echo "Remaining docs organized!"
