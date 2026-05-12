/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA NATIVE STANDARD LIBRARY - CORE                      ║
 * ║                                                                               ║
 * ║   High-performance native implementations of Nova standard library         ║
 * ║   • String manipulation with SSE/NEON optimization                           ║
 * ║   • Collection types (Vec, HashMap, HashSet)                                 ║
 * ║   • I/O operations with async support                                        ║
 * ║   • Math functions with hardware acceleration                                ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_STDLIB_CORE_H
#define NOVA_STDLIB_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// STRING TYPE (Zero-copy, SSE/NEON optimized)
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool is_owned;  // false for string literals
} NovaString;

// String creation
NovaString zen_string_new(const char *cstr);
NovaString zen_string_with_capacity(size_t capacity);
NovaString zen_string_from_slice(const char *data, size_t length);
void zen_string_free(NovaString *str);

// String operations
void zen_string_append(NovaString *str, const char *other);
void zen_string_append_char(NovaString *str, char c);
NovaString zen_string_concat(const NovaString *a, const NovaString *b);
NovaString zen_string_slice(const NovaString *str, size_t start, size_t end);
bool zen_string_equals(const NovaString *a, const NovaString *b);
int zen_string_compare(const NovaString *a, const NovaString *b);
bool zen_string_contains(const NovaString *str, const char *substr);
int zen_string_find(const NovaString *str, const char *substr);
NovaString zen_string_replace(const NovaString *str, const char *from, const char *to);
NovaString zen_string_to_upper(const NovaString *str);
NovaString zen_string_to_lower(const NovaString *str);
NovaString zen_string_trim(const NovaString *str);

// ═══════════════════════════════════════════════════════════════════════════════
// VECTOR (Dynamic Array)
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    void *data;
    size_t length;
    size_t capacity;
    size_t element_size;
} NovaVec;

// Vector creation
NovaVec zen_vec_new(size_t element_size);
NovaVec zen_vec_with_capacity(size_t element_size, size_t capacity);
void zen_vec_free(NovaVec *vec);

// Vector operations
void zen_vec_push(NovaVec *vec, const void *element);
bool zen_vec_pop(NovaVec *vec, void *out);
void *zen_vec_get(const NovaVec *vec, size_t index);
void zen_vec_set(NovaVec *vec, size_t index, const void *element);
void zen_vec_insert(NovaVec *vec, size_t index, const void *element);
void zen_vec_remove(NovaVec *vec, size_t index);
void zen_vec_clear(NovaVec *vec);
void zen_vec_reserve(NovaVec *vec, size_t additional);
void zen_vec_shrink_to_fit(NovaVec *vec);

// ═══════════════════════════════════════════════════════════════════════════════
// HASHMAP (Fast hash table)
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct NovaHashMapEntry {
    uint64_t hash;
    void *key;
    void *value;
    struct NovaHashMapEntry *next;
} NovaHashMapEntry;

typedef struct {
    NovaHashMapEntry **buckets;
    size_t bucket_count;
    size_t length;
    size_t key_size;
    size_t value_size;
    uint64_t (*hash_fn)(const void *key, size_t key_size);
    bool (*equals_fn)(const void *a, const void *b, size_t key_size);
} NovaHashMap;

// HashMap creation
NovaHashMap zen_hashmap_new(size_t key_size, size_t value_size);
void zen_hashmap_free(NovaHashMap *map);

// HashMap operations
void zen_hashmap_insert(NovaHashMap *map, const void *key, const void *value);
bool zen_hashmap_get(const NovaHashMap *map, const void *key, void *out_value);
bool zen_hashmap_remove(NovaHashMap *map, const void *key);
bool zen_hashmap_contains(const NovaHashMap *map, const void *key);
void zen_hashmap_clear(NovaHashMap *map);

// ═══════════════════════════════════════════════════════════════════════════════
// FILE I/O
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct NovaFile NovaFile;

// File operations
NovaFile *zen_file_open(const char *path, const char *mode);
void zen_file_close(NovaFile *file);
size_t zen_file_read(NovaFile *file, void *buffer, size_t size);
size_t zen_file_write(NovaFile *file, const void *data, size_t size);
NovaString zen_file_read_to_string(const char *path);
bool zen_file_write_string(const char *path, const NovaString *content);
bool zen_file_exists(const char *path);
bool zen_file_delete(const char *path);

// ═══════════════════════════════════════════════════════════════════════════════
// MATH (Hardware-accelerated)
// ═══════════════════════════════════════════════════════════════════════════════

// Basic math
double zen_math_sqrt(double x);
double zen_math_pow(double base, double exp);
double zen_math_sin(double x);
double zen_math_cos(double x);
double zen_math_tan(double x);
double zen_math_abs(double x);
double zen_math_floor(double x);
double zen_math_ceil(double x);
double zen_math_round(double x);

// SIMD vector operations (operate on 4 floats at once)
void zen_math_vec4_add(const float *a, const float *b, float *out);
void zen_math_vec4_sub(const float *a, const float *b, float *out);
void zen_math_vec4_mul(const float *a, const float *b, float *out);
void zen_math_vec4_div(const float *a, const float *b, float *out);
float zen_math_vec4_dot(const float *a, const float *b);

// ═══════════════════════════════════════════════════════════════════════════════
// TIME
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    int64_t seconds;
    int32_t nanoseconds;
} NovaDuration;

typedef struct {
    int64_t timestamp_ns;
} NovaInstant;

NovaInstant zen_time_now();
NovaDuration zen_time_since(NovaInstant instant);
void zen_time_sleep(NovaDuration duration);
double zen_duration_as_secs(NovaDuration duration);
double zen_duration_as_millis(NovaDuration duration);

// ═══════════════════════════════════════════════════════════════════════════════
// RANDOM
// ═══════════════════════════════════════════════════════════════════════════════

void zen_random_seed(uint64_t seed);
uint64_t zen_random_u64();
uint32_t zen_random_u32();
double zen_random_f64();  // [0.0, 1.0)
int64_t zen_random_range(int64_t min, int64_t max);

// ═══════════════════════════════════════════════════════════════════════════════
// CONVENIENCE MACROS
// ═══════════════════════════════════════════════════════════════════════════════

#define zen_str(literal) zen_string_from_slice(literal, sizeof(literal) - 1)
#define zen_vec_typed(type) zen_vec_new(sizeof(type))
#define zen_vec_push_typed(vec, type, value) \
    do { type _temp = (value); zen_vec_push(vec, &_temp); } while(0)

#ifdef __cplusplus
}
#endif

#endif // NOVA_STDLIB_CORE_H
