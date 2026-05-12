#!/bin/bash
./build_stage0/src/compiler/novac build src/compiler/bootstrap/nova_compiler_bootstrap.zn -I build/bootstrap_include > bootstrap_final_success.txt 2>&1
echo "Exit code: $?"
