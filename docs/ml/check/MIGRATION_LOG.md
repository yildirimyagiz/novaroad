# Nova → novaRoad/nova Taşıma Logu
> 📅 Tarih: 2026-02-26  
> 🔄 Kaynak: `/Users/yldyagz/Nova/`  
> 📁 Hedef: `/Users/yldyagz/novaRoad/nova/`

---

## ✅ Tamamlanan Taşımalar

### 1. `Nova/ml` → `zn/ml/`
- **Durum:** ⏭️ Atlandı — Dosyalar birebir aynı, taşıma gerekmedi

### 2. `Nova/stdlib/ai/` → `zn/stdlib/ai/`
- **Durum:** ⏭️ Atlandı — Tüm alt dizinler ve dosyalar birebir aynı

### 3. `Nova/ml2/matlab_engine_nova/` → `zn/ml/matlab_engine_nova/`
- **Durum:** ✅ Taşındı
- **Dosyalar (10):** `engine.zn`, `engine_error.zn`, `engine_helper.zn`, `engine_session.zn`, `example_usage.zn`, `feval_future.zn`, `future_result.zn`, `matlab_engine.zn`, `matlab_future.zn`, `mod.zn`

### 4. `Nova/native/include/` → `include/` (yeni başlıklar)
- **Durum:** ✅ Taşındı (14 yeni header)
- **Dosyalar:**
  - `nova/ai/numerics/nova_numerics.h`
  - `nova/api/nova_python_binding.h`
  - `nova/compute/graph/nova_graph.h`
  - `nova/formal/isabelle/nova_isabelle_bridge.h`
  - `nova/formal/klee/nova_klee_bridge.h`
  - `nova/formal/klee/nova_klee_harness.h`
  - `nova/formal/nova_proof_cache.h`
  - `nova/formal/nova_proof_manifest.h`
  - `nova/formal/smt/nova_cvc5_bridge.h`
  - `nova/formal/smt/nova_smt_ir.h`
  - `nova/memory/planner/nova_memory_planner.h`
  - `nova/optimizer/fusion/nova_kernel_fusion.h`
  - `nova/runtime/nova_deterministic_reduction.h`

### 5. `Nova/native/src/` → `src/native/src/` (rsync --ignore-existing)
- **Durum:** ✅ Taşındı
- **Alt dizinler ve dosya sayıları:**

| Dizin | Dosya Sayısı |
|-------|-------------|
| `ai/` | 41 |
| `api/` | 1 |
| `autocal/` | 1 |
| `backends/` | 40 |
| `cluster/` | 2 |
| `compiler/` | 39 |
| `compute/` | 44 |
| `experimental/` | 2 |
| `formal/` | 25 |
| `groq-ai/` | 61 |
| `memory/` | 4 |
| `monitoring/` | 2 |
| `optimizer/` | 19 |
| `platform/` | 1 |
| `quantization/` | 1 |
| `runtime/` | 20 |
| `sparse/` | 1 |
| `tools/` | 52 |
| `wasm/` | 1 |
| **Toplam** | **~357** |

### 6. `Nova/native/bootstrap/` → `src/native/src/bootstrap/`
- **Durum:** ✅ Taşındı (9 dosya)
- `nova_runtime_io.c`, `nova_runtime_net.c`, `nova_stage0.py`, `nova_stage1.zn`, `nova_stage1` (binary), `nova_stage2.o`, `Makefile`, `README.md`, `stage2.log`

### 7. `Nova/native/calibration/` → `src/native/src/calibration/`
- **Durum:** ✅ Taşındı (20 dosya)
- Nova autocal sistemi + nova autocal sistemi
- Header'lar + kaynak dosyalar

### 8. `Nova/native/tests/nova_n/` → `tests/nova_n/`
- **Durum:** ✅ Taşındı (6 dosya)
- `dgm.zn`, `test_advanced_generics.zn`, `test_memory_safety.zn`, `test_memory_safety_error.zn`, `test_pro_opt.zn`, `test_traits_generics.zn`

### 9. `Nova/native/stdlib/pm/` → `stdlib/pm/`
- **Durum:** ✅ Taşındı (4 dosya)
- `registry.c`, `registry.h`, `resolver.c`, `resolver.h`

### 10. `Nova/native/calma_core/` → `src/native/src/calma_core/`
- **Durum:** ✅ Taşındı (8 dosya)
- `calma_lexer.c`, `calma_parser.c`, `calma_parser.h`, `calma_bridge.py`, `calma_compiler.py`, `include/calma_kernel.h`

---

## ⏭️ Atlanılanlar (Gerek Yok)

| Kaynak | Neden Atlandı |
|--------|---------------|
| `Nova/native/echosystem/.venv/` | Python venv — proje dosyası değil |
| `Nova/native/build/*.o` | Derlenmiş objeler — yeniden derlenir |
| `Nova/native/docs/` | Proje dokümanları — Nova docs ile çakışır |

---

## 📊 Özet

| Kategori | Taşınan Dosya |
|----------|--------------|
| zn/ml/matlab_engine_nova | 10 |
| include/ (yeni headers) | 13 |
| src/native/src/ (tüm dizinler) | ~357 |
| src/native/src/bootstrap | 9 |
| src/native/src/calibration | 20 |
| tests/nova_n | 6 |
| stdlib/pm | 4 |
| src/native/src/calma_core | 8 |
| **TOPLAM** | **~427** |
