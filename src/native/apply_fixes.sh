#!/usr/bin/env bash
# ============================================================
# apply_fixes.sh — Nova Native Backend Crash Fixes
# ============================================================
set -uo pipefail

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   NOVA CRASH FIX UYGULAYICI                           ║"
echo "╚══════════════════════════════════════════════════════════╝"

# 1. ROCm stub fix (return 0 -> return -1)
echo "🔧 Fixing ROCm stubs..."
sed -i '' 's/return 0;/return -1;/g' native/src/backends/rocm/nova_rocm.c

# 2. OpenCL stub fix (return 0 -> return -1 for stubs)
# Note: cl_matmul is implemented, so only fix stubbed ones if any.
# Actually, nova_cl_softmax was the culprit in logs.
echo "🔧 Fixing OpenCL stubs..."
sed -i '' 's/int64_t nova_cl_softmax.*{.*return 0;.*}/int64_t nova_cl_softmax(const float *in, float *out, int64_t n) { return -1; }/g' native/src/backends/opencl/nova_opencl.c
# General verify: replace any simple "return 0;" stubs in opencl
# But be careful not to break real implementations.
# nova_cl_softmax body was causing warnings, let's just replace it explicitly.

cat << 'C_CODE' > native/src/backends/opencl/nova_opencl.c_partial
// Stubs
int64_t nova_cl_softmax(const float *in, float *out, int64_t n) { return -1; }
C_CODE
# (Detailed edits done via tool below to be safe)

# 3. Mobile Codegen unused fix
echo "🔧 Fixing Mobile Codegen unused function..."
sed -i '' 's/static void append_code/__attribute__((unused)) static void append_code/g' native/src/backends/mobile/nova_mobile_codegen.c

# 4. Metal Platform Guard
echo "🔧 Checking Metal Guard..."
if ! grep -q "#ifndef __APPLE__" native/src/backends/metal/nova_metal_gpu.c; then
    # Add guard at the top
    echo "Adding guard to Metal backend..."
    echo -e "#ifndef __APPLE__\n#error \"Metal backend requires macOS\"\n#endif\n$(cat native/src/backends/metal/nova_metal_gpu.c)" > native/src/backends/metal/nova_metal_gpu.c
fi

echo "✅ Fixes Applied."
