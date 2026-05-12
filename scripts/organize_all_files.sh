#!/bin/bash

# Create directory structure
mkdir -p config
mkdir -p data/{benchmarks,snapshots,reports}
mkdir -p binaries
mkdir -p build_config

echo "Organizing files..."

# Configuration files
mv *.toml config/ 2>/dev/null
mv configure.ac config/ 2>/dev/null
mv meson.build config/ 2>/dev/null
mv setup.py config/ 2>/dev/null
mv package.json config/ 2>/dev/null
mv compile_flags.txt config/ 2>/dev/null
mv .clang-format .clang-tidy .clangd config/ 2>/dev/null

# Benchmark data files
mv BENCHMARK*.json data/benchmarks/ 2>/dev/null
mv NATIVE*.json data/benchmarks/ 2>/dev/null
mv bench_history.jsonl data/benchmarks/ 2>/dev/null
mv BENCHMARK*.txt data/reports/ 2>/dev/null

# Snapshot files
mv BENCHMARK_SNAPSHOT*.json data/snapshots/ 2>/dev/null

# Status/summary text files
mv *_STATUS*.txt data/reports/ 2>/dev/null
mv *_SUMMARY*.txt data/reports/ 2>/dev/null
mv bootstrap_determinism_report.txt data/reports/ 2>/dev/null

# Build configuration
mv CMakeLists*.txt build_config/ 2>/dev/null
mv Makefile.simple* build_config/ 2>/dev/null
mv makefile_build.log build_config/ 2>/dev/null

# Compile commands
mv compile_commands.json* build_config/ 2>/dev/null

# Binaries and executables
mv nova binaries/ 2>/dev/null
mv nova_bootstrap_stage0 binaries/ 2>/dev/null
mv gemini_benchmark binaries/ 2>/dev/null
mv package_manager binaries/ 2>/dev/null
mv perf_tuning binaries/ 2>/dev/null

# Release archives
mv nova-v1.0.0*.tar.gz* binaries/ 2>/dev/null

# Nova source files (move to examples or remove if temporary)
mkdir -p examples/quick_demos
mv ffi_bridge_m1.zn examples/quick_demos/ 2>/dev/null
mv m1_main_demo.zn examples/quick_demos/ 2>/dev/null
mv runtime_m1.zn examples/quick_demos/ 2>/dev/null
mv lib.zn examples/quick_demos/ 2>/dev/null
mv main.zn examples/quick_demos/ 2>/dev/null
mv tmp_two_functions.zn examples/quick_demos/ 2>/dev/null

# Keep main build files in root
mv build_config/CMakeLists.txt . 2>/dev/null
mv build_config/Makefile ../Makefile 2>/dev/null || echo "Makefile already in place"

echo "Organization complete!"
echo ""
echo "Summary:"
echo "  config/           - Configuration files"
echo "  data/benchmarks/  - Benchmark data"
echo "  data/snapshots/   - Snapshots"
echo "  data/reports/     - Status reports"
echo "  build_config/     - Alternative build configs"
echo "  binaries/         - Compiled binaries"
echo "  examples/quick_demos/ - Demo source files"
