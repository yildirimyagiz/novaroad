#!/bin/bash
echo "Testing novac_fixed..."
./novac_fixed --help > novac_help_direct.txt 2>&1
echo "Running bootstrap..."
./novac_fixed build src/compiler/bootstrap/nova_compiler_bootstrap.zn -I build/bootstrap_include > bootstrap_direct.txt 2>&1
echo "Done. Status: $?"
