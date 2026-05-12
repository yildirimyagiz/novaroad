# Native Modül Denetimi: Eksikler ve Performans

Bu belge `native/src` (backends, compiler, compute, formal, memory, monitoring, optimizer, platform, quantization, runtime, sparse, tools, wasm), `native/stdlib` ve `nova_context.c` üzerinde yapılan incelemenin özetidir.

---

## 1. Eksik Modüller / Tutarsızlıklar

### 1.0 Autocalibration — her platform için maksimum verim
- **Hedef:** Özellik eşitliği değil, platform bazlı **maksimum verim**.
- **`native/include/nova_autocal.h`** ve **`native/src/autocal/nova_autocal.c`** eklendi:
  - `nova_autocal_run()`: CPU için tile/prefetch autotune (mevcut `nova_autotune` kullanır); opsiyonel olarak tüm mevcut GPU backend’leri için matmul benchmark ve en iyi GFLOPS kaydı.
  - `nova_autocal_run_platform(platform)`: Sadece belirtilen platform için kalibrasyon.
  - `nova_autocal_get_config(platform)`: Kalibrasyon sonrası en iyi config (tile_m, tile_k, gpu_threadgroup, peak_gflops, mem_gb_s).
  - `nova_autocal_report()`: Tüm kalibre edilmiş platformların özeti.
  - `nova_autocal_export_baseline(filepath)`: Kalibre config’leri JSON baseline dosyasına yazar.
- **GPU kalibrasyonu:** Backend dispatch lib’e eklendiğinde ve `NOVA_AUTOCAL_USE_DISPATCH` tanımlı derlendiğinde Metal/CUDA/ROCm/Vulkan/OpenCL için ayrı ayrı matmul benchmark çalışır; her platform için maksimum verim config’i saklanır.
- **"nova Auto calbration"** klasörü: Stub olarak kaldı; tam kalibrasyon için ana lib’in `nova_autocal_run` kullanılır.

**Varsayılan build:** Dispatch her zaman dahil; `nova_backend_dispatch.c`, `nova_cpu_backend.c` ve CUDA/ROCm/Vulkan/OpenCL stub’ları ile birlikte derlenir. **Fallback:** Aktif backend hata döndürürse (ret &lt; 0) her `nova_dispatch_*` çağrısı otomatik olarak CPU’ya düşer. Metal: relu implemente, softmax CPU fallback.  
**Full build (sadece autocal GPU):** `cmake -DNOVA_FULL_BUILD=ON ..` yalnızca `NOVA_AUTOCAL_USE_DISPATCH` ekler (autocal tüm backend’leri kalibre eder). Link düzeltmeleri: `nova_obligation.c`, `nova_graph_runtime.c` (get_device_memory_limit) ana lib’e eklendi.

### 1.1 `quantization/` ve `sparse/` (artık dolu)
- **`native/src/quantization/`** – Hiç dosya yok. Sadece CPU kernel tarafında `nova_kernel_matmul_delta_sparse_f32` ve tools’ta “quantization” kelimesi geçiyor; ayrı bir quantization modülü (INT8/INT4, per-channel, calibration) yok.
- **`native/src/sparse/`** – Hiç dosya yok. Sparse tensor formatları (CSR/COO/blocked), sparse-dense matmul API’si ve backend dispatch’e sparse yolu eklenmemiş.

**Öneri:**  
- `quantization/`: En azından `nova_quantize_f32_to_int8`, scale/zero-point saklama ve (opsiyonel) calibration API’si.  
- `sparse/`: Sparse tensor tipi, format seçimi ve `nova_dispatch_sparse_matmul` benzeri tek noktadan dispatch.

### 1.2 İki farklı `nova_context` implementasyonu
- **`native/src/nova_context.c`** (build’e giren): Sadece proof cache + global pool; fabric, scheduler, optimizer, profiler, invariant_engine, learning, economics, fault_monitor **hiç oluşturulmuyor**.
- **`native/src/compute/nova_context.c`**: Tüm katmanları init/shutdown ediyor ama **CMakeLists.txt bu dosyayı kullanmıyor**; lib’e sadece `src/nova_context.c` ekli.

Sonuç: Header’da tanımlı olan `ctx->fabric`, `ctx->scheduler` vb. kullanılırsa null dereference riski var.

**Öneri:**  
- Ya tek context kaynağı seçin: ya `compute/nova_context.c`’yi lib’e alıp `src/nova_context.c`’yi kaldırın, ya da `src/nova_context.c` içinde aynı katmanları (fabric, scheduler, …) create/destroy edin.  
- Proof cache ve formal ayarlar `compute` versiyonunda da kullanılıyorsa oraya taşınmalı.

### 1.3 Backend dispatch: Metal ve CPU fallback
- **Fallback:** Tüm `nova_dispatch_*` çağrılarında aktif backend hata (ret &lt; 0) döndürürse otomatik CPU’ya düşülür.
- Metal: add, mul, matmul, **relu** implemente; softmax CPU fallback (`nova_metal_softmax` -1 döner).

### 1.4 Platform
- **`platform/nova_hardware_model.c`** tek dosya; `nova_autotune.h` ve `nova_benchmark_memory_bandwidth()` ile uyumlu.
- `NovaHardwareProfile` ve `nova_benchmark_*` optimizer/autotune tarafında tanımlı; platform modülü ince ve tutarlı görünüyor.

### 1.5 Memory
- **`nova_allocator.c`**: `nova_malloc_aligned` içinde `MemoryTag` kullanılmıyor (tag’e göre izleme yok). `nova_temp_pool_alloc` global tek bir pool ve tek `g_pool_offset` kullanıyor → **thread-safe değil**.

---

## 2. Performans Önerileri

### 2.1 CPU kernel’lar (`backends/cpu/nova_kernels_cpu.c`)
- Matmul: NEON ile 8-row unroll, K-tiling, prefetch ve `nova_get_tuned_config()` kullanımı mevcut; iyi.
- **Softmax**: `expf()` kullanılıyor; `fast_expf` (veya benzeri) sadece bazı kernel’larda var. Softmax’ta da hızlı exp kullanılabilir (numerik tolerans test edilmeli).
- **Sigmoid**: Tam `expf(-x)`; burada da fast exp veya lookup tablosu düşünülebilir.

### 2.2 Backend dispatch
- **Flash attention**: Şu an yalnızca CPU’ya gidiyor (`nova_dispatch_flash_attention`). CUDA/Metal için fused flash attention eklendiğinde dispatch’e case’ler eklenmeli.
- Metal relu/softmax eklenirse (yukarıdaki gibi) dispatch güncellenmeli.

### 2.3 Context ve bellek havuzu
- **`src/nova_context.c`**: `config.default_pool_size == 0` ise pool yok; büyük çalıştırmalarda tekrarlı malloc/free olabilir. Varsayılan küçük bir pool (ör. 256KB) veya “lazy pool” açmak peak alloc sayısını azaltabilir.
- **`compute/nova_context.c`**: 256MB varsayılan pool kullanıyor; bu versiyon lib’e alınırsa bellek ayak izi buna göre yönetilmeli.

### 2.4 Memory allocator
- `nova_temp_pool_alloc`: Thread-local pool veya per-thread offset ile thread-safe yapılabilir; yoğun JIT/grafik çalıştırmada yarar sağlar.
- `MemoryTag`: İleride profil/debug için tag bazlı istatistik toplanacaksa `nova_malloc_aligned` içinde tag kaydı eklenmeli.

### 2.5 Formal / arena
- `nova_memory.c` arena ve pool’da `nova_formal_check_invariant` çağrıları var; formal kapalıyken bunların maliyeti minimal olmalı (inline/no-op branch). Hot path’te gereksiz kontroller varsa formal_mode’a göre kapatılabilir.

---

## 3. Kısa Özet Tablo

| Alan            | Durum / Eksik                                      | Öncelik  |
|-----------------|----------------------------------------------------|----------|
| quantization/   | Boş klasör                                         | Yüksek   |
| sparse/         | Boş klasör                                         | Yüksek   |
| nova_context  | İki implementasyon; build’e giren minimal          | Kritik   |
| Backend Metal   | add/mul dispatch’e eklendi                          | Tamamlandı |
| Allocator       | Tag kullanılmıyor, temp pool thread-safe değil      | Orta     |
| CPU softmax/sigmoid | expf yerine fast exp değerlendirilebilir        | Düşük    |
| Flash attention | Sadece CPU; CUDA/Metal path yok                     | Orta     |

---

## 4. Yapılan Kod Değişikliği

- **`native/src/backends/nova_backend_dispatch.c`**  
  - `nova_dispatch_add`: `NOVA_BACKEND_METAL` case eklendi, `nova_metal_add` çağrılıyor.  
  - `nova_dispatch_mul`: `NOVA_BACKEND_METAL` case eklendi, `nova_metal_mul` çağrılıyor.

Bu sayede Metal seçiliyken add/mul işlemleri CPU’ya düşmeden Metal backend’ine gidiyor.
