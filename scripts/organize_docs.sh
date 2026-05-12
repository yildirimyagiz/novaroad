#!/bin/bash

# Benchmark reports
mv BENCHMARK*.md docs/benchmarks/ 2>/dev/null
mv bench_*.md docs/benchmarks/ 2>/dev/null
mv NATIVE_*.md docs/benchmarks/ 2>/dev/null
mv GEMINI_*.md docs/benchmarks/ 2>/dev/null
mv REALISTIC_PERFORMANCE_ANALYSIS.md docs/benchmarks/ 2>/dev/null

# Implementation status/completion reports
mv *_COMPLETE.md docs/completed/ 2>/dev/null
mv *_COMPLETION*.md docs/completed/ 2>/dev/null
mv COMPLETION_REPORT*.md docs/completed/ 2>/dev/null
mv *_SUMMARY.md docs/reports/ 2>/dev/null
mv *_STATUS*.md docs/reports/ 2>/dev/null
mv *_REPORT*.md docs/reports/ 2>/dev/null

# Analysis and planning
mv *_ANALYSIS.md docs/planning/ 2>/dev/null
mv *_ROADMAP*.md docs/planning/ 2>/dev/null
mv NOVA_GRAND_MASTER_PLAN*.md docs/planning/ 2>/dev/null
mv NOVA_COMPREHENSIVE_ROADMAP*.md docs/planning/ 2>/dev/null
mv NOVA_BLITZ*.md docs/planning/ 2>/dev/null
mv NOVA_PATENT*.md docs/planning/ 2>/dev/null
mv NOVA_GOOGLE*.md docs/planning/ 2>/dev/null
mv roadmapmain.md docs/planning/ 2>/dev/null
mv OVERALL_ROADMAP.md docs/planning/ 2>/dev/null

# Implementation guides
mv *_IMPLEMENTATION*.md docs/implementation/ 2>/dev/null
mv *_GUIDE.md docs/implementation/ 2>/dev/null
mv COMPILER_EXAMPLES.md docs/implementation/ 2>/dev/null
mv COMPILER_FEATURE_SUPPORT.md docs/implementation/ 2>/dev/null

# Test results
mv *_TEST*.md docs/testing/ 2>/dev/null
mv TEST_SUITE.md docs/testing/ 2>/dev/null
mv TESTING_SUMMARY.md docs/testing/ 2>/dev/null

# Session reports
mv FINAL_SESSION*.md docs/reports/ 2>/dev/null
mv SESSION_*.md docs/reports/ 2>/dev/null
mv ULTIMATE_*.md docs/reports/ 2>/dev/null

# API and specs
mv NOVA_ABI*.md docs/ 2>/dev/null
mv NOVA_STDLIB_API*.md docs/ 2>/dev/null

# Architecture docs
mv NOVA_UNIFIED_ARMY_ARCHITECTURE.md docs/architecture/ 2>/dev/null
mv NOVA_THE_ZERO_LATENCY_REVOLUTION.md docs/architecture/ 2>/dev/null

# Keep these in root
# README.md, CHANGELOG.md, CONTRIBUTING.md, INSTALL.md, QUICK_START.md

echo "Documentation organized!"
