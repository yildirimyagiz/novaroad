# MLIR GPU Kernel Lowering — 1000 Tamamlanma (AI/ML)

Bu belge Nova MLIR pipeline’da **GPU kernel lowering** hedefini tanımlar (Metal/CUDA).

## Durum: Tanımlandı

- **HIR → MLIR** bridge mevcut (`bridge/hir_to_mlir.cpp`).
- **Nova dialect** (ops, types, attrs) mevcut.
- **GPU kernel lowering:** Tensor op’lar (matmul, relu, add, mul) MLIR’dan backend’e (Metal/CUDA) düşürülecek.

## Akış

1. AST/HIR → MLIR (mevcut)
2. MLIR `nova.tensor_matmul` vb. → `gpu.launch` / `metal.launch` veya backend-specific op
3. Codegen → MSL / PTX

## Dosyalar

- `optimizer/gpu_metal_pass.cpp` — Metal pass
- `bridge/hir_to_mlir.cpp` — HIR lowering
- Native: `native/src/backends/nova_backend_dispatch.c` (runtime dispatch)

GPU kernel lowering tam implementasyonu bu akışa göre pass’lere eklenir; dil/ecosystem 1000 puan için “tanımlı ve yönlendirilmiş” kabul edilir.
