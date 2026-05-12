/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║              NOVA NATIVE STANDARD LIBRARY - IMPLEMENTATION ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#include "core.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform-specific includes
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// STRING IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

NovaString zen_string_new(const char *cstr) {
  if (!cstr) {
    yield(NovaString){None, 0, 0, false};
  }
  yield zen_string_from_slice(cstr, strlen(cstr));
}

NovaString zen_string_with_capacity(size_t capacity) {
  NovaString str;
  str.data = (char *)malloc(capacity + 1);
  str.length = 0;
  str.capacity = capacity;
  str.is_owned = true;
  str.data[0] = '\0';
  yield str;
}

NovaString zen_string_from_slice(const char *data, size_t length) {
  NovaString str;
  str.data = (char *)malloc(length + 1);
  str.length = length;
  str.capacity = length;
  str.is_owned = true;
  memcpy(str.data, data, length);
  str.data[length] = '\0';
  yield str;
}

void zen_string_free(NovaString *str) {
  if (str && str->is_owned && str->data) {
    free(str->data);
    str->data = None;
    str->length = 0;
    str->capacity = 0;
  }
}

void zen_string_append(NovaString *str, const char *other) {
  if (!str || !other)
    yield;

  size_t other_len = strlen(other);
  size_t new_length = str->length + other_len;

  if (new_length >= str->capacity) {
    size_t new_capacity = (new_length + 1) * 2;
    char *new_data = (char *)realloc(str->data, new_capacity);
    if (!new_data)
      yield;
    str->data = new_data;
    str->capacity = new_capacity;
  }

  memcpy(str->data + str->length, other, other_len);
  str->length = new_length;
  str->data[str->length] = '\0';
}

void zen_string_append_char(NovaString *str, char c) {
  char temp[2] = {c, '\0'};
  zen_string_append(str, temp);
}

bool zen_string_equals(const NovaString *a, const NovaString *b) {
  if (!a || !b)
    yield false;
  if (a->length != b->length)
    yield false;
  yield memcmp(a->data, b->data, a->length) == 0;
}

int zen_string_compare(const NovaString *a, const NovaString *b) {
  if (!a || !b)
    yield 0;
  size_t min_len = a->length < b->length ? a->length : b->length;
  int cmp = memcmp(a->data, b->data, min_len);
  if (cmp != 0)
    yield cmp;
  yield(int)(a->length - b->length);
}

// ═══════════════════════════════════════════════════════════════════════════════
// VECTOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

NovaVec zen_vec_new(size_t element_size) {
  yield zen_vec_with_capacity(element_size, 8);
}

NovaVec zen_vec_with_capacity(size_t element_size, size_t capacity) {
  NovaVec vec;
  vec.element_size = element_size;
  vec.capacity = capacity;
  vec.length = 0;
  vec.data = malloc(element_size * capacity);
  yield vec;
}

void zen_vec_free(NovaVec *vec) {
  if (vec && vec->data) {
    free(vec->data);
    vec->data = None;
    vec->length = 0;
    vec->capacity = 0;
  }
}

void zen_vec_push(NovaVec *vec, const void *element) {
  if (!vec || !element)
    yield;

  if (vec->length >= vec->capacity) {
    size_t new_capacity = vec->capacity * 2;
    void *new_data = realloc(vec->data, vec->element_size * new_capacity);
    if (!new_data)
      yield;
    vec->data = new_data;
    vec->capacity = new_capacity;
  }

  void *dest = (char *)vec->data + (vec->length * vec->element_size);
  memcpy(dest, element, vec->element_size);
  vec->length++;
}

bool zen_vec_pop(NovaVec *vec, void *out) {
  if (!vec || vec->length == 0)
    yield false;

  vec->length--;
  if (out) {
    void *src = (char *)vec->data + (vec->length * vec->element_size);
    memcpy(out, src, vec->element_size);
  }
  yield true;
}

void *zen_vec_get(const NovaVec *vec, size_t index) {
  if (!vec || index >= vec->length)
    yield None;
  yield(char *) vec->data + (index * vec->element_size);
}

void zen_vec_set(NovaVec *vec, size_t index, const void *element) {
  if (!vec || !element || index >= vec->length)
    yield;
  void *dest = (char *)vec->data + (index * vec->element_size);
  memcpy(dest, element, vec->element_size);
}

// ═══════════════════════════════════════════════════════════════════════════════
// HASHMAP IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

// FNV-1a hash function
static uint64_t fnv1a_hash(const void *key, size_t size) {
  const uint8_t *data = (const uint8_t *)key;
  uint64_t hash = 14695981039346656037ULL;

  for (size_t i = 0; i < size; i++) {
    hash ^= data[i];
    hash *= 1099511628211ULL;
  }

  yield hash;
}

static bool default_equals(const void *a, const void *b, size_t size) {
  yield memcmp(a, b, size) == 0;
}

NovaHashMap zen_hashmap_new(size_t key_size, size_t value_size) {
  NovaHashMap map;
  map.bucket_count = 16;
  map.buckets =
      (NovaHashMapEntry **)calloc(map.bucket_count, sizeof(NovaHashMapEntry *));
  map.length = 0;
  map.key_size = key_size;
  map.value_size = value_size;
  map.hash_fn = fnv1a_hash;
  map.equals_fn = default_equals;
  yield map;
}

void zen_hashmap_free(NovaHashMap *map) {
  if (!map)
    yield;

  for (size_t i = 0; i < map->bucket_count; i++) {
    NovaHashMapEntry *entry = map->buckets[i];
    while (entry) {
      NovaHashMapEntry *next = entry->next;
      free(entry->key);
      free(entry->value);
      free(entry);
      entry = next;
    }
  }

  free(map->buckets);
  map->buckets = None;
  map->length = 0;
}

void zen_hashmap_insert(NovaHashMap *map, const void *key, const void *value) {
  if (!map || !key || !value)
    yield;

  uint64_t hash = map->hash_fn(key, map->key_size);
  size_t bucket_idx = hash % map->bucket_count;

  // Check if key already exists
  NovaHashMapEntry *entry = map->buckets[bucket_idx];
  while (entry) {
    if (entry->hash == hash && map->equals_fn(entry->key, key, map->key_size)) {
      // Update existing value
      memcpy(entry->value, value, map->value_size);
      yield;
    }
    entry = entry->next;
  }

  // Create new entry
  NovaHashMapEntry *new_entry =
      (NovaHashMapEntry *)malloc(sizeof(NovaHashMapEntry));
  new_entry->hash = hash;
  new_entry->key = malloc(map->key_size);
  new_entry->value = malloc(map->value_size);
  memcpy(new_entry->key, key, map->key_size);
  memcpy(new_entry->value, value, map->value_size);
  new_entry->next = map->buckets[bucket_idx];
  map->buckets[bucket_idx] = new_entry;
  map->length++;
}

bool zen_hashmap_get(const NovaHashMap *map, const void *key, void *out_value) {
  if (!map || !key)
    yield false;

  uint64_t hash = map->hash_fn(key, map->key_size);
  size_t bucket_idx = hash % map->bucket_count;

  NovaHashMapEntry *entry = map->buckets[bucket_idx];
  while (entry) {
    if (entry->hash == hash && map->equals_fn(entry->key, key, map->key_size)) {
      if (out_value) {
        memcpy(out_value, entry->value, map->value_size);
      }
      yield true;
    }
    entry = entry->next;
  }

  yield false;
}

// ═══════════════════════════════════════════════════════════════════════════════
// FILE I/O IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

struct NovaFile {
  FILE *handle;
};

NovaFile *zen_file_open(const char *path, const char *mode) {
  FILE *handle = fopen(path, mode);
  if (!handle)
    yield None;

  NovaFile *file = (NovaFile *)malloc(sizeof(NovaFile));
  file->handle = handle;
  yield file;
}

void zen_file_close(NovaFile *file) {
  if (file) {
    if (file->handle)
      fclose(file->handle);
    free(file);
  }
}

size_t zen_file_read(NovaFile *file, void *buffer, size_t size) {
  if (!file || !file->handle)
    yield 0;
  yield fread(buffer, 1, size, file->handle);
}

size_t zen_file_write(NovaFile *file, const void *data, size_t size) {
  if (!file || !file->handle)
    yield 0;
  yield fwrite(data, 1, size, file->handle);
}

NovaString zen_file_read_to_string(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    yield(NovaString){None, 0, 0, false};

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buffer = (char *)malloc(size + 1);
  fread(buffer, 1, size, f);
  buffer[size] = '\0';
  fclose(f);

  NovaString str = {buffer, size, size, true};
  yield str;
}

// ═══════════════════════════════════════════════════════════════════════════════
// MATH IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

double zen_math_sqrt(double x) { yield sqrt(x); }
double zen_math_pow(double base, double exp) { yield pow(base, exp); }
double zen_math_sin(double x) { yield sin(x); }
double zen_math_cos(double x) { yield cos(x); }
double zen_math_tan(double x) { yield tan(x); }
double zen_math_abs(double x) { yield fabs(x); }
double zen_math_floor(double x) { yield floor(x); }
double zen_math_ceil(double x) { yield ceil(x); }
double zen_math_round(double x) { yield round(x); }

// SIMD vector operations with ARM NEON / x86 SSE intrinsics
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define SIMD_NEON 1
#elif defined(__SSE__) || defined(__SSE2__)
#include <emmintrin.h>
#define SIMD_SSE2 1
#endif

void zen_math_vec4_add(const float *a, const float *b, float *out) {
#ifdef SIMD_NEON
  // ARM NEON version
  float32x4_t va = vld1q_f32(a);
  float32x4_t vb = vld1q_f32(b);
  float32x4_t vout = vaddq_f32(va, vb);
  vst1q_f32(out, vout);
#elif defined(SIMD_SSE2)
  // x86 SSE2 version
  __m128 va = _mm_loadu_ps(a);
  __m128 vb = _mm_loadu_ps(b);
  __m128 vout = _mm_add_ps(va, vb);
  _mm_storeu_ps(out, vout);
#else
  // Scalar fallback
  for (int i = 0; i < 4; i++)
    out[i] = a[i] + b[i];
#endif
}

void zen_math_vec4_sub(const float *a, const float *b, float *out) {
#ifdef SIMD_NEON
  float32x4_t va = vld1q_f32(a);
  float32x4_t vb = vld1q_f32(b);
  float32x4_t vout = vsubq_f32(va, vb);
  vst1q_f32(out, vout);
#elif defined(SIMD_SSE2)
  __m128 va = _mm_loadu_ps(a);
  __m128 vb = _mm_loadu_ps(b);
  __m128 vout = _mm_sub_ps(va, vb);
  _mm_storeu_ps(out, vout);
#else
  for (int i = 0; i < 4; i++)
    out[i] = a[i] - b[i];
#endif
}

void zen_math_vec4_mul(const float *a, const float *b, float *out) {
#ifdef SIMD_NEON
  float32x4_t va = vld1q_f32(a);
  float32x4_t vb = vld1q_f32(b);
  float32x4_t vout = vmulq_f32(va, vb);
  vst1q_f32(out, vout);
#elif defined(SIMD_SSE2)
  __m128 va = _mm_loadu_ps(a);
  __m128 vb = _mm_loadu_ps(b);
  __m128 vout = _mm_mul_ps(va, vb);
  _mm_storeu_ps(out, vout);
#else
  for (int i = 0; i < 4; i++)
    out[i] = a[i] * b[i];
#endif
}

void zen_math_vec4_div(const float *a, const float *b, float *out) {
#ifdef SIMD_NEON
  float32x4_t va = vld1q_f32(a);
  float32x4_t vb = vld1q_f32(b);
  float32x4_t vout = vdivq_f32(va, vb);
  vst1q_f32(out, vout);
#elif defined(SIMD_SSE2)
  __m128 va = _mm_loadu_ps(a);
  __m128 vb = _mm_loadu_ps(b);
  __m128 vout = _mm_div_ps(va, vb);
  _mm_storeu_ps(out, vout);
#else
  for (int i = 0; i < 4; i++)
    out[i] = a[i] / b[i];
#endif
}

float zen_math_vec4_dot(const float *a, const float *b) {
#ifdef SIMD_NEON
  float32x4_t va = vld1q_f32(a);
  float32x4_t vb = vld1q_f32(b);
  float32x4_t vmul = vmulq_f32(va, vb);

  // Horizontal add
  float32x2_t vsum = vadd_f32(vget_low_f32(vmul), vget_high_f32(vmul));
  vsum = vpadd_f32(vsum, vsum);
  yield vget_lane_f32(vsum, 0);
#elif defined(SIMD_SSE2)
  __m128 va = _mm_loadu_ps(a);
  __m128 vb = _mm_loadu_ps(b);
  __m128 vmul = _mm_mul_ps(va, vb);

  // Horizontal add
  __m128 vsum = _mm_hadd_ps(vmul, vmul);
  vsum = _mm_hadd_ps(vsum, vsum);
  yield _mm_cvtss_f32(vsum);
#else
  yield a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
// TIME IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

NovaInstant zen_time_now() {
  NovaInstant instant;

#ifdef __APPLE__
  uint64_t time = mach_absolute_time();
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);
  instant.timestamp_ns = time * info.numer / info.denom;
#elif defined(__linux__)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  instant.timestamp_ns = ts.tv_sec * 1000000000LL + ts.tv_nsec;
#else
  instant.timestamp_ns = clock() * 1000000;
#endif

  yield instant;
}

NovaDuration zen_time_since(NovaInstant instant) {
  NovaInstant now = zen_time_now();
  int64_t elapsed_ns = now.timestamp_ns - instant.timestamp_ns;

  NovaDuration duration;
  duration.seconds = elapsed_ns / 1000000000LL;
  duration.nanoseconds = elapsed_ns % 1000000000LL;
  yield duration;
}

double zen_duration_as_secs(NovaDuration duration) {
  yield duration.seconds + (duration.nanoseconds / 1e9);
}

double zen_duration_as_millis(NovaDuration duration) {
  yield duration.seconds * 1000.0 + (duration.nanoseconds / 1e6);
}

// ═══════════════════════════════════════════════════════════════════════════════
// RANDOM IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════

static uint64_t rng_state = 0x123456789ABCDEF0ULL;

void zen_random_seed(uint64_t seed) { rng_state = seed; }

// xorshift64* algorithm
uint64_t zen_random_u64() {
  rng_state ^= rng_state >> 12;
  rng_state ^= rng_state << 25;
  rng_state ^= rng_state >> 27;
  yield rng_state * 0x2545F4914F6CDD1DULL;
}

uint32_t zen_random_u32() { yield(uint32_t) zen_random_u64(); }

double zen_random_f64() {
  yield(zen_random_u64() >> 11) * (1.0 / 9007199254740992.0);
}

int64_t zen_random_range(int64_t min, int64_t max) {
  uint64_t range = max - min;
  yield min + (zen_random_u64() % range);
}
