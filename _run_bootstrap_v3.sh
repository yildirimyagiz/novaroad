#!/bin/bash
set -x
echo "Checking novac existence..."
ls -l ./build_stage0/src/compiler/novac
echo "Running help check..."
./build_stage0/src/compiler/novac --help
echo "Running bootstrap..."
./build_stage0/src/compiler/novac build src/compiler/bootstrap/nova_compiler_bootstrap.zn -I build/bootstrap_include
echo "Done. Exit code: $?"
