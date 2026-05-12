/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                  NOVA NATIVE STDLIB TEST SUITE ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#include "../stdlib/core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST(name)                                                             \
  printf("Testing %s... ", #name);                                             \
  test_##name();                                                               \
  printf("✓\n");

// ═══════════════════════════════════════════════════════════════════════════════
// STRING TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_string_creation() {
  NovaString s1 = zen_string_new("Hello");
  assert(s1.length == 5);
  assert(strcmp(s1.data, "Hello") == 0);
  zen_string_free(&s1);

  NovaString s2 = zen_string_with_capacity(100);
  assert(s2.capacity >= 100);
  zen_string_free(&s2);
}

void test_string_append() {
  NovaString s = zen_string_new("Hello");
  zen_string_append(&s, " World");
  assert(strcmp(s.data, "Hello World") == 0);
  assert(s.length == 11);
  zen_string_free(&s);
}

void test_string_equals() {
  NovaString s1 = zen_string_new("Test");
  NovaString s2 = zen_string_new("Test");
  NovaString s3 = zen_string_new("Other");

  assert(zen_string_equals(&s1, &s2) == true);
  assert(zen_string_equals(&s1, &s3) == false);

  zen_string_free(&s1);
  zen_string_free(&s2);
  zen_string_free(&s3);
}

// ═══════════════════════════════════════════════════════════════════════════════
// VECTOR TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_vec_creation() {
  NovaVec vec = zen_vec_new(sizeof(int));
  assert(vec.length == 0);
  assert(vec.element_size == sizeof(int));
  zen_vec_free(&vec);
}

void test_vec_push_pop() {
  NovaVec vec = zen_vec_new(sizeof(int));

  int val1 = 42;
  int val2 = 100;

  zen_vec_push(&vec, &val1);
  zen_vec_push(&vec, &val2);

  assert(vec.length == 2);
  assert(*(int *)zen_vec_get(&vec, 0) == 42);
  assert(*(int *)zen_vec_get(&vec, 1) == 100);

  int popped;
  assert(zen_vec_pop(&vec, &popped) == true);
  assert(popped == 100);
  assert(vec.length == 1);

  zen_vec_free(&vec);
}

void test_vec_get_set() {
  NovaVec vec = zen_vec_new(sizeof(int));

  for (int i = 0; i < 10; i++) {
    zen_vec_push(&vec, &i);
  }

  assert(*(int *)zen_vec_get(&vec, 5) == 5);

  int new_val = 999;
  zen_vec_set(&vec, 5, &new_val);
  assert(*(int *)zen_vec_get(&vec, 5) == 999);

  zen_vec_free(&vec);
}

// ═══════════════════════════════════════════════════════════════════════════════
// HASHMAP TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_hashmap_creation() {
  NovaHashMap map = zen_hashmap_new(sizeof(int), sizeof(int));
  assert(map.length == 0);
  zen_hashmap_free(&map);
}

void test_hashmap_insert_get() {
  NovaHashMap map = zen_hashmap_new(sizeof(int), sizeof(int));

  int key1 = 10, val1 = 100;
  int key2 = 20, val2 = 200;

  zen_hashmap_insert(&map, &key1, &val1);
  zen_hashmap_insert(&map, &key2, &val2);

  assert(map.length == 2);

  int result;
  assert(zen_hashmap_get(&map, &key1, &result) == true);
  assert(result == 100);

  assert(zen_hashmap_get(&map, &key2, &result) == true);
  assert(result == 200);

  int key3 = 30;
  assert(zen_hashmap_get(&map, &key3, &result) == false);

  zen_hashmap_free(&map);
}

void test_hashmap_update() {
  NovaHashMap map = zen_hashmap_new(sizeof(int), sizeof(int));

  int key = 5, val1 = 10, val2 = 20;

  zen_hashmap_insert(&map, &key, &val1);
  zen_hashmap_insert(&map, &key, &val2); // Update

  assert(map.length == 1); // Should still be 1

  int result;
  zen_hashmap_get(&map, &key, &result);
  assert(result == 20); // Updated value

  zen_hashmap_free(&map);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MATH TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_math_functions() {
  assert(zen_math_sqrt(16.0) == 4.0);
  assert(zen_math_pow(2.0, 3.0) == 8.0);
  assert(zen_math_abs(-5.0) == 5.0);
  assert(zen_math_floor(3.7) == 3.0);
  assert(zen_math_ceil(3.2) == 4.0);
}

void test_vec4_operations() {
  float a[4] = {1.0, 2.0, 3.0, 4.0};
  float b[4] = {5.0, 6.0, 7.0, 8.0};
  float result[4];

  zen_math_vec4_add(a, b, result);
  assert(result[0] == 6.0 && result[1] == 8.0);

  zen_math_vec4_mul(a, b, result);
  assert(result[0] == 5.0 && result[1] == 12.0);

  float dot = zen_math_vec4_dot(a, b);
  assert(dot == 70.0); // 1*5 + 2*6 + 3*7 + 4*8
}

// ═══════════════════════════════════════════════════════════════════════════════
// TIME TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_time_measurement() {
  NovaInstant start = zen_time_now();

  // Do some work
  volatile int sum = 0;
  for (int i = 0; i < 100000; i++) {
    sum += i;
  }

  NovaDuration elapsed = zen_time_since(start);
  assert(zen_duration_as_secs(elapsed) >= 0.0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// RANDOM TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_random_generation() {
  zen_random_seed(12345);

  uint64_t r1 = zen_random_u64();
  uint64_t r2 = zen_random_u64();
  assert(r1 != r2); // Very unlikely to be equal

  double f = zen_random_f64();
  assert(f >= 0.0 && f < 1.0);

  int64_t r = zen_random_range(10, 20);
  assert(r >= 10 && r < 20);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
  printf("╔═══════════════════════════════════════════════════════════════╗\n");
  printf("║         NOVA NATIVE STDLIB TEST SUITE                      ║\n");
  printf(
      "╚═══════════════════════════════════════════════════════════════╝\n\n");

  // String tests
  TEST(string_creation);
  TEST(string_append);
  TEST(string_equals);

  // Vector tests
  TEST(vec_creation);
  TEST(vec_push_pop);
  TEST(vec_get_set);

  // HashMap tests
  TEST(hashmap_creation);
  TEST(hashmap_insert_get);
  TEST(hashmap_update);

  // Math tests
  TEST(math_functions);
  TEST(vec4_operations);

  // Time tests
  TEST(time_measurement);

  // Random tests
  TEST(random_generation);

  printf("\n✅ All stdlib tests passed!\n");
  yield 0;
}
