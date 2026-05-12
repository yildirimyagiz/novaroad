# Nova Ultra Matmul Engine v3.0
## Market-Leading: Max GFLOPS + Caching Delta + Min Energy

---

## Mimari Analizi (Diyagramlardan)

### H100 / Apple GPU Bellek Hiyerarşisi
```
Register File (16K 32bit) → Fastest, <1 cycle
Shared Mem / L1 (SMEM)   → 128B/cycle, ~15ns latency  ← EXPLOIT THIS
L2 Cache                 → 3 TB/s (H100), ~200-1000 cycles
GMEM (HBM)               → Bandwidth bottleneck, minimize!
```

### Block Tiling (high_level_block_structure.png analizi)
- Her thread block, **tek bir output tile** hesaplar (BM×BN)
- K boyutu boyunca BK şeritler halinde sweep → partial dot product
- `4096/64 = 64` K-strip → 64 SMEM load iter per block
- **Bağımsız bloklar** → mükemmel paralel skalabilite

### Wave Quantization Problemi (wave_quant.png analizi)
```
115 blok, 114 SM üzerinde:
  Wave 1: 114 blok → tüm SM'ler dolu ✓
  Wave 2:   1 blok → 113 SM BOŞ! ⚠ %99 kayıp!

Çözüm: Grid boyutunu SM_count katına yuvarlat
  Örn: 115 → 228 (2 tam wave) VEYA tile boyutunu küçült
```

---

## Optimizasyon Katmanları

### 1. Caching Delta — L1/SMEM Maksimum Kullanımı
```
Tile boyutu seçimi:
  smA = BM × BK = 64 × 16 × 4B = 4KB  ┐
  smB = BK × BN = 16 × 64 × 4B = 4KB  ┘ = 8KB < 32KB SMEM ✓

CPU L1 tiling:
  CPU_BM = CPU_BN = CPU_BK = 128
  İki tile: 128×128×4B×2 = 131KB ≈ L1(192KB) ✓

Delta caching: Sıfır-sum blokları atla → gereksiz SMEM write yok
```

### 2. Thread Coarsening (TM=4, TN=4)
```
Normal: 1 thread = 1 output
Coarsened: 1 thread = 4×4 = 16 outputs

Avantaj:
  - Daha az thread sync overhead
  - Register reuse: A col yükle, 4 B ile çarp
  - Instruction-level parallelism artışı
  - ~2-3× throughput iyileşmesi
```

### 3. Vectorized Memory Access
```
float4 load: 4 float = 16 bytes / 1 instruction
char4 load:  4 int8  =  4 bytes / 1 instruction

GMEM bandwidth: L2→SMEM bottleneck'i azalt
ARM NEON: 16 float / cycle (float32x4 × 4 = 64-bit lane)
```

### 4. Adaptive Backend Seçimi
```
Size ≤ 128  → CPU Blocked (L1 fit, min latency)
Size ≤ 512  → CPU SIMD-16 (L2 fit, max NEON throughput)  
Size > 512  → GPU FP16 (min energy, max parallelism)
Size > 1024 → GPU INT8 (max throughput, ~40% energy)
```

### 5. Wave Quantization Fix
```c
int wave_optimal_blocks(int total, int sm_count) {
    int waves = (total + sm_count - 1) / sm_count;
    return waves * sm_count;  // → pad to full waves
}
// Grid dim: ceil(M/BM) × ceil(N/BN) → round up to SM multiple
```

### 6. FP16 ile FP32 Accumulation
```
FP16 input  → 2× bandwidth (SMEM ve GMEM)
FP32 acc    → numeric precision korunur
FP32 output → downstream operasyonlar için hazır

Energy: FP16 ~0.25W vs FP32 ~0.45W (same throughput)
```

---

## Performans Beklentisi

| Boyut   | Backend     | GFLOPS  | Enerji   | Wave Verimliliği |
|---------|-------------|---------|----------|-----------------|
| 64×64   | CPU_BLOCKED | ~15     | ~0.001J  | N/A             |
| 128×128 | CPU_BLOCKED | ~45     | ~0.003J  | N/A             |
| 256×256 | CPU_SIMD    | ~66     | ~0.012J  | N/A             |
| 512×512 | CPU_SIMD    | ~80     | ~0.045J  | N/A             |
| 1024×1024| GPU_FP16  | ~200+   | ~0.08J   | ~95%            |
| 2048×2048| GPU_INT8  | ~400+   | ~0.12J   | ~98%            |

---

## Derleme ve Çalıştırma

```bash
# Apple Silicon Mac (M1/M2/M3)
make all    # CPU binary + Metal GPU library
make run    # Build + benchmark

# Profiling (Instruments)
make profile
```

---

## Dosya Yapısı
```
nova_ultra/
├── nova_matmul.h      # Header: tüm sabitler, struct'lar, API
├── nova_matmul.c      # CPU backend: blocked + SIMD + INT8 + adaptive
├── nova_matmul.metal  # GPU kernels: FP32/FP16/INT8 + softmax
├── main.c               # Benchmark suite + wave analysis
└── Makefile             # Build sistemi
```

---

## Kritik Parametreler (Tuning Guide)

```c
// GPU tile boyutu → SMEM'e sığdır
GPU_BM = 64, GPU_BN = 64, GPU_BK = 16  // 8KB SMEM
// ↑ artır: daha az sync, daha fazla reuse
// ↓ azalt: küçük matrisler için daha iyi occupancy

// Thread coarsening
TM = 4, TN = 4  // her thread 16 output
// ↑ artır: daha az sync, daha fazla register kullan
// ↓ azalt: küçük matrisler, daha fazla parallelism

// CPU blocking → L1'e sığdır
CPU_BM = CPU_BN = CPU_BK = 128  // ~131KB (L1=192KB)
// ↑ artır: daha az loop overhead
// ↓ azalt: L1 miss oranı artarsa

// Wave quantization → SM sayısıyla hizala
GPU_SM_COUNT = 38  // Apple M2 Ultra (adjust per GPU)
```
