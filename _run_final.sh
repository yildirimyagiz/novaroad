#!/bin/bash
touch STARTED_BOOTSTRAP
./build_stage0/src/compiler/novac build src/compiler/bootstrap/nova_compiler_bootstrap.zn -I build/bootstrap_include > BOOTSTRAP_RESULT.txt 2>&1
touch FINISHED_BOOTSTRAP
