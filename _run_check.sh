#!/bin/bash
echo "=== CHECKING NOVAC ==="
ls -la /Users/yldyagz/novaRoad/nova/build_stage0/src/compiler/novac
file /Users/yldyagz/novaRoad/nova/build_stage0/src/compiler/novac
echo "=== RUNNING NOVAC ==="
/Users/yldyagz/novaRoad/nova/build_stage0/src/compiler/novac --help 2>&1 || echo "novac --help failed"
echo "=== RUNNING BOOTSTRAP ==="
timeout 15 /Users/yldyagz/novaRoad/nova/build_stage0/src/compiler/novac build /Users/yldyagz/novaRoad/nova/src/compiler/bootstrap/nova_compiler_bootstrap.zn -I /Users/yldyagz/novaRoad/nova/build/bootstrap_include 2>&1
echo "=== EXIT CODE: $? ==="
