#ifndef NOVA_FFI_H
#define NOVA_FFI_H

/*
 * Nova FFI ABI Header - v1.0
 *
 * FROZEN ABI CONTRACTS:
 *   - extern "C" symbol naming: nova_[module]_[function]
 *   - String ownership: copy-in, caller-frees
 *   - Result ABI: tagged union with error codes
 *   - Panic boundary: panic never crosses FFI
 *   - Allocator: caller provides alloc/free
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ══════════════════════════════════════════════════════════════════════════════
// BASIC TYPES
// ══════════════════════════════════════════════════════════════════════════════

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef size_t   usize;
typedef uintptr_t usize_ptr;

// ══════════════════════════════════════════════════════════════════════════════
// RESULT ABI (FROZEN)
// ══════════════════════════════════════════════════════════════════════════════

typedef enum NovaResultTag {
    NovaResultTag_Ok,
    NovaResultTag_Err,
} NovaResultTag;

typedef struct NovaResultFFI {
    NovaResultTag tag;
    u32 error_code;           // 0 = success, >0 = error
    const char* error_message; // null-terminated, caller-owned
} NovaResultFFI;

// ══════════════════════════════════════════════════════════════════════════════
// STRING ABI (FROZEN)
// ══════════════════════════════════════════════════════════════════════════════

typedef struct NovaStringFFI {
    const char* ptr;  // UTF-8 data, null-terminated
    usize len;        // byte length (excluding null)
} NovaStringFFI;

// ══════════════════════════════════════════════════════════════════════════════
// VECTOR ABI (FROZEN)
// ══════════════════════════════════════════════════════════════════════════════

// Opaque handle pattern - caller manages memory
typedef struct NovaVecFFIHandle NovaVecFFIHandle;

// ══════════════════════════════════════════════════════════════════════════════
// ALLOCATOR ABI (FROZEN)
// ══════════════════════════════════════════════════════════════════════════════

typedef struct NovaAllocatorFFI {
    void* (*alloc_fn)(usize size);
    void  (*free_fn)(void* ptr);
    void* (*realloc_fn)(void* ptr, usize size);
} NovaAllocatorFFI;

// ══════════════════════════════════════════════════════════════════════════════
// FFI FUNCTION DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

// ── String Operations ──────────────────────────────────────────────────────

NovaResultFFI nova_string_new(const char* str);
void nova_string_free(NovaStringFFI* str);

// ── Vector Operations ──────────────────────────────────────────────────────

NovaResultFFI nova_vec_new(usize capacity, usize element_size, NovaVecFFIHandle** out_handle);
NovaResultFFI nova_vec_len(NovaVecFFIHandle* vec, usize* out_len);
NovaResultFFI nova_vec_capacity(NovaVecFFIHandle* vec, usize* out_capacity);
NovaResultFFI nova_vec_get(NovaVecFFIHandle* vec, usize index, void* dest, usize element_size);
NovaResultFFI nova_vec_set(NovaVecFFIHandle* vec, usize index, const void* src, usize element_size);
NovaResultFFI nova_vec_push(NovaVecFFIHandle* vec, const void* element, usize element_size);
NovaResultFFI nova_vec_pop(NovaVecFFIHandle* vec, void* dest, usize element_size, bool* out_success);
void nova_vec_free(NovaVecFFIHandle* vec);

// ── Result Operations ──────────────────────────────────────────────────────

bool nova_result_is_ok(const NovaResultFFI* result);
bool nova_result_is_err(const NovaResultFFI* result);
u32 nova_result_error_code(const NovaResultFFI* result);
const char* nova_result_error_message(const NovaResultFFI* result);

// ── Allocator Operations ───────────────────────────────────────────────────

NovaResultFFI nova_allocator_with_caller(NovaAllocatorFFI allocator);

// ── Bridge Helper Operations ───────────────────────────────────────────────

const char* nova_bridge_string_to_swift(const char* nova_str, usize len);
NovaResultFFI nova_bridge_result_to_swift(const NovaResultFFI* result);

const char* nova_bridge_string_to_kotlin(const char* nova_str, usize len);
NovaResultFFI nova_bridge_result_to_kotlin(const NovaResultFFI* result);

// ══════════════════════════════════════════════════════════════════════════════
// PLATFORM SPECIFIC EXTENSIONS
// ══════════════════════════════════════════════════════════════════════════════

// iOS/macOS specific
#ifdef __APPLE__
    // Swift bridge extensions
    typedef struct {
        const char* _Nonnull swift_string;
        usize length;
    } SwiftString;

    SwiftString nova_bridge_swift_string_convert(const NovaStringFFI* nova_str);
#endif

// Android specific
#ifdef __ANDROID__
    // JNI bridge extensions
    typedef struct {
        const char* _Nonnull jni_string;
        usize length;
        bool needs_release;
    } JNIString;

    JNIString nova_bridge_jni_string_convert(const NovaStringFFI* nova_str);
#endif

#ifdef __cplusplus
}
#endif

#endif /* NOVA_FFI_H */
