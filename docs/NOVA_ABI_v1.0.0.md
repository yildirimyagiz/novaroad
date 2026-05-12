# Nova ABI Specification

# Version: 1.0.0-rc1

# Status: ABI FREEZE - Binary compatibility guaranteed

# Date: 2026-02-24


# ABI FREEZE OUTPUTS SUMMARY

## Struct Field Order

- Fields are stored in declaration order.
- No implicit reordering for optimization.
- Explicit `repr(C)` ensures C-compatible layout when specified.

## Alignment Rules

- `uintptr_t`: `alignof(uintptr_t)` (8 bytes on 64-bit platforms).
- Primitives: Natural alignment (e.g., `i32` = 4 bytes, `i64` = 8 bytes).
- Structs: Maximum alignment of all fields.
- Arrays: Alignment of element type.
- Enums: Alignment of discriminant (1 byte) or payload (whichever is larger).

## Enum Discriminant Size

- Fixed as `uint8_t` (1 byte) for all enums.
- Discriminant values start from 0, incrementing sequentially.
- Payload alignment matches the most aligned variant.

## Padding Rules

- Struct padding inserted to satisfy field alignments.
- No implicit padding rearrangement.
- Minimum padding used; fields packed tightly where possible.
- End padding added to satisfy struct alignment.

## Mangled Symbol Format

- Functions: `_ZN[package][function][hash]E`
  - Package: `[length][name]` (e.g., `4nova`)
  - Function: `[length][name]` (e.g., `5print`)
  - Hash: 8-character hex from signature.
- Types: Length-prefixed (e.g., `3Vec3i32` for `Vec<i32>`).
- Traits: VTable index based on declaration order.

## FFI Boundary Rules

- C ABI compliance (System V AMD64 on Linux/macOS).
- Strings null-terminated for C interop.
- No reordering across FFI calls.
- Volatile/atomic operations preserved.
- Exception unwinding stops at FFI boundaries (panics do not propagate to C).

# ABI GATE (MANDATORY)

✅ **size_of<T>() snapshot tests**: Capture and verify `size_of` for all core types (String, Vec<T>, HashMap<K,V>, Option<T>, Result<T,E>) to detect layout changes.

✅ **align_of<T>() snapshot tests**: Capture and verify `align_of` for all core types to ensure alignment stability.

✅ **enum tag/payload layout tests**: Verify discriminant size (uint8_t) and payload positioning for Option<T> and Result<T,E>.

✅ **mangled symbol snapshot tests**: Capture and verify mangled names for functions, types, and traits to ensure linkage stability.

✅ **stage1/stage2 ABI behavior equivalence**: Ensure bootstrap compiler (stage1) and self-hosted compiler (stage2) produce identical ABI layouts and symbols.
# 1. DATA LAYOUT SPECIFICATIONS

# 1.1 String Layout (ABI Frozen)

#

# struct NovaString {

# uintptr_t len;        // Length in bytes (not codepoints)

# uintptr_t capacity;   // Allocated capacity in bytes

# uint8_t*  data;       // Pointer to UTF-8 encoded data

# }

#

# - All fields are uintptr_t (platform-dependent size)

# - Data pointer is never null (even for empty strings)

# - Memory layout is contiguous in heap allocation (header and data may be separate allocations)

# - String data is null-terminated for C interop (but not relied upon)

# 1.2 Vec<T> Layout (ABI Frozen)

#

# struct NovaVec<T> {

# uintptr_t len;        // Number of elements currently stored

# uintptr_t capacity;   // Number of elements allocated

# T*        data;       // Pointer to element array

# }

#

# - Header is always 3 * sizeof(uintptr_t) bytes

# - Element type T has platform-dependent alignment

# - Data pointer may be null only if capacity == 0

# - The backing buffer is heap-allocated and referenced by `data`.
# - Contiguity is guaranteed for the backing buffer itself, not necessarily header+buffer together.

# 1.3 HashMap<K,V> Layout (ABI Frozen)

#

# struct NovaHashMap<K,V> {

# NovaVec<NovaHashBucket<K,V>> buckets;  // Bucket array

# uintptr_t                      count;    // Total elements

# }

#

# struct NovaHashBucket<K,V> {

# K key

# V value

# uintptr_t hash;  // Cached hash value

# uint8_t   state; // EntryState enum

# }

#

# enum EntryState: uint8_t {

# EMPTY = 0

# OCCUPIED = 1

# DELETED = 2

# }

#

# - Uses open addressing with tombstones (EMPTY/OCCUPIED/DELETED states)
# - Probe strategy: linear (ABI frozen)
# - Load factor maintained between 0.5-0.75

- Hash values cached to avoid recomputation

# 1.4 Option<T> / Result<T,E> Enum Representation (ABI Frozen)

#

# Both use tagged union representation

#

# struct NovaOption<T> {

# uint8_t discriminant;  // 0 = None, 1 = Some

# union {

# T some_value

# } payload

# }

#

# struct NovaResult<T,E> {

# uint8_t discriminant;  // 0 = Ok, 1 = Err

# union {

# T ok_value

# E err_value

# } payload

# }

#

# - Discriminant always first byte (uint8_t, offset 0)
# - Payload starts at offset align_up(1, payload_align)
# - Padding may exist between discriminant and payload to satisfy payload alignment
# - Total enum size is rounded up to max(enum_align, payload_align)
# - Maximum enum variant count: 256 (uint8_t discriminant)

# =============================================================================

# 2. RUNTIME OBJECT HEADER (ABI Frozen)

# =============================================================================

# All heap-allocated objects have this header

#

# struct NovaObjectHeader {

# uintptr_t ref_count;     // Reference count for RC

# uint32_t  type_id;       // Runtime type identifier

# uint32_t  flags;         // Object flags (GC mark, etc.)

# uintptr_t size;          // Object size in bytes

# }

#

# - Header size: 24 bytes on 64-bit platforms (current field set)

# - Always located immediately before object data

# - Reference counting is atomic (thread-safe)

# - Type ID is globally unique per type

# - Flags include: GC_MARK_BIT = 0x01, GC_PINNED = 0x02

# GC metadata follows header for collectable objects

#

# struct NovaGCHeader {

# NovaObjectHeader header

# uintptr_t*        ptr_map;     // Bit map of pointers in object

# uintptr_t         ptr_map_len; // Length of pointer map

# }

# =============================================================================

# 3. NAME MANGLING RULES (ABI Frozen)

# =============================================================================

# 3.1 Function Name Mangling

#

# Nova functions are mangled as: _ZN[package][function][hash]E

#

# Examples

# - nova::print -> _ZN4nova5print12345678E

# - mypkg::myfunc -> _ZN5mypkg7myfunc87654321E

#

# Package names are prefixed with length: [length][name]

# Function names use same encoding

# Hash is 8-character hex from symbol signature

# 3.2 Type Name Mangling

#

# Generic types: [base][params]

# - Vec<i32> -> 3Vec3i32

# - HashMap<String,i32> -> 7HashMap6String3i32

#

# Length-prefixed encoding: [decimal_length][name]

# 3.3 Trait Method Dispatch
#
# v1.0.0 supports trait static dispatch only (monomorphized)
# - Same as regular function mangling with type parameters
#
# Dynamic dispatch/VTable ABI is not part of v1.0.0 ABI freeze

# =============================================================================

# 4. CALLING CONVENTION (ABI Frozen)

# =============================================================================

# 4.1 General Calling Convention

#

# - x86_64 Linux/macOS: System V AMD64 ABI
# - AArch64 Linux: AAPCS64
# - AArch64 macOS (Apple Silicon): Apple ARM64 ABI

# - Arguments passed in registers when possible

# - Stack used for overflow arguments

# - Return values in RAX/RDX (or XMM0 for floats)

# - Caller cleans up stack (if applicable)

# 4.2 Built-in Functions (ABI Frozen)

#

# Memory management

# - alloc(size: usize) -> *mut u8

# - dealloc(ptr: *mut u8, size: usize)

# - realloc(old: *mut u8, old_size: usize, new_size: usize) ->*mut u8

#

# Type operations

# - size_of<T>() -> usize

# - align_of<T>() -> usize

# - type_id<T>() -> u32

#

# Panic/unreachable

# - panic(message: str) -> !  (noreturn)

# - unreachable() -> !        (noreturn)

# 4.3 Runtime Bridge Functions

#

# GC interface

# - gc_collect() -> usize  (returns freed bytes)

# - gc_stats() -> GCStats

#

# String operations (C interop)

# - All string methods callable from C code

# - UTF-8 validation preserved across FFI

# =============================================================================

# 5. EXCEPTION HANDLING (ABI Frozen)

# =============================================================================

# Nova uses setjmp/longjmp for panic unwinding

#

# struct NovaPanicContext {

# jmp_buf jump_buffer

# const char* message

# const char* file

# uint32_t line

# struct NovaPanicContext* prev

# }

#

# - Thread-local panic stack

# - Unwinding preserves cleanup execution

# - Panic messages are heap-allocated strings

# - No exception specifications (unlike C++)

# =============================================================================

# 6. THREADING AND ATOMICITY (ABI Frozen)

# =============================================================================

# Reference counting is atomic

# - Uses platform atomic operations

# - Memory order: acquire/release semantics

# - Thread-safe for concurrent access

#

# GC is not concurrent (stop-the-world)

# - Single-threaded collection

# - All threads paused during GC

# - No concurrent mutator access

# =============================================================================

# 7. PLATFORM SPECIFICS (ABI Frozen)

# =============================================================================

# 7.1 Memory Alignment

#

# - uintptr_t: alignof(uintptr_t) (8 bytes on 64-bit)

# - Primitives: natural alignment (i32 = 4, i64 = 8)

# - Structs: max alignment of fields

# - Arrays: alignment of element type

# 7.2 Endianness

#

# - Native endianness (little-endian on x86_64)

# - No explicit endianness conversion in ABI

# - String data assumed native endian

# 7.3 Pointer Size

#

# - All pointer types: sizeof(uintptr_t)

# - usize/isize: platform word size

# - Consistent across compilation units

# =============================================================================

# 8. COMPILER BUILT-IN CONTRACTS (ABI Frozen)

# =============================================================================

# 8.1 Memory Layout Guarantees

#

# - Struct field order matches source declaration

# - No implicit padding rearrangement

# - Explicit repr(C) for C interop structs

# - Packed structs available but not default

# 8.2 Optimization Constraints

#

# - Volatile operations preserve order

# - Atomic operations use platform primitives

# - No reordering across FFI boundaries

# - Exception unwinding preserves cleanup

# 8.3 Linkage Guarantees

#

# - All public symbols exported with stable names

# - Weak linkage for optional symbols

# - Dynamic linking supported but not required

# - Static linking produces self-contained binaries

# =============================================================================

# IMPLEMENTATION NOTES

# =============================================================================

# ABI Stability Guarantees

# - Binary compatibility across patch versions

# - Compatible with dynamic linking

# - Preserved across compiler versions

# - Platform-specific extensions allowed

# Testing Requirements

# - ABI compatibility tests across versions

# - Cross-compilation validation

# - Dynamic linking verification

# - Platform-specific ABI testing

# Evolution Strategy
# - Existing public struct/enum layouts are immutable after ABI freeze
# - ABI additions must use new symbols/types or opaque handles
# - New types can be added without breaking existing ABI

# =============================================================================

# END OF NOVA v1.0.0 ABI SPECIFICATION

# =============================================================================
