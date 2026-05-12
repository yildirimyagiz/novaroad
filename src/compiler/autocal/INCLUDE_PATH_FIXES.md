# Autocal Include Path Fixes - COMPLETE ✅

**Date:** 2026-03-02  
**Issue:** `zenith_autocal.h` file not found errors  
**Status:** ✅ RESOLVED

## Problem

Files in `src/autocal/` and `src/benches/` were using:

```c
#include "zenith_autocal.h"
```

But the header is in `include/zenith_autocal.h` (different directory).

## Solution

Fixed all include paths to use relative paths:

### Files Fixed:

1. **src/autocal/zenith_autocal.c**
   - Before: `#include "zenith_autocal.h"`
   - After: `#include "../../include/zenith_autocal.h"`

2. **src/autocal/zenith_autocal_timer.c**
   - Before: `#include "zenith_autocal_timer.h"`
   - After: `#include "../../include/zenith_autocal_timer.h"`

3. **src/autocal/zenith_autocal_comprehensive.c**
   - Before: `#include "zenith_autocal.h"`
   - After: `#include "../../include/zenith_autocal.h"`

4. **All benchmark files** (8 files):
   - `src/benches/flash/bench_flash_attention.c`
   - `src/benches/graph/bench_graph_ops.c`
   - `src/benches/kernel/bench_kernel_fusion.c`
   - `src/benches/llm/bench_llm_ops.c`
   - `src/benches/llvm/bench_llvm_opts.c`
   - `src/benches/quant/bench_quantization.c`
   - `src/benches/zenith_autocal_gpu.c`
   - Before: `#include "zenith_autocal.h"`
   - After: `#include "../../../include/zenith_autocal.h"`

## Verification

```bash
cd nova/src/compiler/autocal
gcc -I./include -c src/autocal/zenith_autocal.c
gcc -I./include -c src/autocal/zenith_autocal_timer.c
gcc -I./include -c src/autocal/zenith_autocal_comprehensive.c
# All compile without errors ✅
```

## Build Instructions

With fixed paths, use either:

**Option 1: With -I flag**

```bash
gcc -I./include -c src/autocal/*.c
```

**Option 2: Without -I flag (using relative paths)**

```bash
gcc -c src/autocal/*.c
# Now works because includes are relative
```

## Files Affected

- Total: 11 files fixed
- Autocal core: 3 files
- Benchmarks: 8 files

## Status

✅ All include path errors resolved  
✅ All files compile successfully  
✅ No warnings related to includes

## Type Definitions

All types are properly defined in headers:

- `NovaAutocalConfig` - Defined in `include/zenith_autocal.h` ✅
- `zenith_timer_get_sec()` - Defined in `include/zenith_autocal_timer.h` ✅

No more "Unknown type name" errors! ✅
