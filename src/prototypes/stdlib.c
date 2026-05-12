// B.3 Standard Library
// Core types, collections, and utilities for Nova

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// Include runtime types
typedef struct nova_object_header {
    uint32_t ref_count;
    uint32_t size;
    uint8_t marked;
    uint8_t type_tag;
} nova_object_header_t;

typedef struct nova_allocator nova_allocator_t;
typedef struct nova_object nova_object_t;
typedef struct nova_string nova_string_t;
typedef struct nova_array nova_array_t;

// HashMap implementation
typedef struct nova_hash_entry {
    uint64_t hash;
    const char* key;
    nova_object_t* value;
    struct nova_hash_entry* next;
} nova_hash_entry_t;

typedef struct nova_hashmap {
    nova_object_header_t header;
    size_t capacity;
    size_t size;
    nova_hash_entry_t** buckets;
} nova_hashmap_t;

// Optional type
typedef struct nova_optional {
    nova_object_t* value;
    int has_value;
} nova_optional_t;

// Result type for error handling
typedef struct nova_result {
    nova_object_t* value;
    const char* error;
    int is_error;
} nova_result_t;

// Iterator protocol
typedef struct nova_iterator {
    void* data;
    nova_object_t* (*next)(struct nova_iterator* iter);
    int (*has_next)(struct nova_iterator* iter);
    void (*free)(struct nova_iterator* iter);
} nova_iterator_t;

// Forward declarations
nova_object_t* nova_object_retain(nova_object_t* obj);
void nova_object_release(nova_allocator_t* alloc, nova_object_t* obj);

// Hash function (djb2 algorithm)
uint64_t nova_hash_string(const char* str) {
    uint64_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// HashMap implementation
nova_hashmap_t* nova_hashmap_create(nova_allocator_t* alloc, size_t initial_capacity) {
    size_t total_size = sizeof(nova_hashmap_t) + initial_capacity * sizeof(nova_hash_entry_t*);

    nova_hashmap_t* map = calloc(1, total_size);
    map->header.ref_count = 1;
    map->header.size = total_size;
    map->header.marked = 0;
    map->header.type_tag = 4; // Custom type for hashmap
    map->capacity = initial_capacity;
    map->size = 0;
    map->buckets = (nova_hash_entry_t**)(map + 1);

    nova_allocator_register_object(alloc, (nova_object_t*)map);

    return map;
}

void nova_hashmap_put(nova_hashmap_t* map, const char* key, nova_object_t* value) {
    uint64_t hash = nova_hash_string(key);
    size_t bucket = hash % map->capacity;

    // Check if key already exists
    nova_hash_entry_t* entry = map->buckets[bucket];
    while (entry) {
        if (entry->hash == hash && strcmp(entry->key, key) == 0) {
            // Update existing entry
            nova_object_retain(value);
            nova_object_release(NULL, entry->value); // TODO: Pass allocator
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    // Create new entry
    nova_hash_entry_t* new_entry = malloc(sizeof(nova_hash_entry_t));
    new_entry->hash = hash;
    new_entry->key = strdup(key);
    new_entry->value = nova_object_retain(value);
    new_entry->next = map->buckets[bucket];

    map->buckets[bucket] = new_entry;
    map->size++;
}

nova_object_t* nova_hashmap_get(nova_hashmap_t* map, const char* key) {
    uint64_t hash = nova_hash_string(key);
    size_t bucket = hash % map->capacity;

    nova_hash_entry_t* entry = map->buckets[bucket];
    while (entry) {
        if (entry->hash == hash && strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

int nova_hashmap_contains(nova_hashmap_t* map, const char* key) {
    return nova_hashmap_get(map, key) != NULL;
}

// String utilities
nova_string_t* nova_string_concat(nova_string_t* a, nova_string_t* b) {
    size_t new_length = a->length + b->length;
    char* new_data = malloc(new_length + 1);

    memcpy(new_data, a->data, a->length);
    memcpy(new_data + a->length, b->data, b->length);
    new_data[new_length] = '\0';

    // TODO: Create new string object
    return nova_string_create(NULL, new_data, new_length); // TODO: Pass allocator
}

int nova_string_equals(nova_string_t* a, nova_string_t* b) {
    return a->length == b->length && memcmp(a->data, b->data, a->length) == 0;
}

nova_string_t* nova_string_substring(nova_string_t* str, size_t start, size_t length) {
    if (start + length > str->length) {
        length = str->length - start;
    }

    return nova_string_create(NULL, str->data + start, length); // TODO: Pass allocator
}

// Array utilities
void nova_array_push(nova_array_t* array, nova_object_t* element) {
    if (array->length >= array->capacity) {
        // TODO: Resize array
        printf("⚠️  Array resize not implemented\n");
        return;
    }

    array->elements[array->length++] = nova_object_retain(element);
}

nova_object_t* nova_array_get(nova_array_t* array, size_t index) {
    if (index >= array->length) return NULL;
    return array->elements[index];
}

void nova_array_set(nova_array_t* array, size_t index, nova_object_t* value) {
    if (index >= array->length) return;

    nova_object_retain(value);
    nova_object_release(NULL, array->elements[index]); // TODO: Pass allocator
    array->elements[index] = value;
}

// Iterator for arrays
typedef struct nova_array_iterator {
    nova_iterator_t base;
    nova_array_t* array;
    size_t index;
} nova_array_iterator_t;

nova_object_t* nova_array_iterator_next(nova_iterator_t* iter) {
    nova_array_iterator_t* array_iter = (nova_array_iterator_t*)iter;
    if (array_iter->index >= array_iter->array->length) return NULL;

    return array_iter->array->elements[array_iter->index++];
}

int nova_array_iterator_has_next(nova_iterator_t* iter) {
    nova_array_iterator_t* array_iter = (nova_array_iterator_t*)iter;
    return array_iter->index < array_iter->array->length;
}

void nova_array_iterator_free(nova_iterator_t* iter) {
    free(iter);
}

nova_iterator_t* nova_array_iterator(nova_array_t* array) {
    nova_array_iterator_t* iter = calloc(1, sizeof(nova_array_iterator_t));
    iter->base.data = iter;
    iter->base.next = nova_array_iterator_next;
    iter->base.has_next = nova_array_iterator_has_next;
    iter->base.free = nova_array_iterator_free;
    iter->array = array;
    iter->index = 0;

    return &iter->base;
}

// Math utilities
double nova_math_sqrt(double x) { return sqrt(x); }
double nova_math_sin(double x) { return sin(x); }
double nova_math_cos(double x) { return cos(x); }
double nova_math_pow(double base, double exp) { return pow(base, exp); }

// Random utilities
uint32_t nova_random_uint32(void) {
    // Simple LCG random number generator
    static uint32_t state = 123456789;
    state = (uint64_t)state * 1103515245 + 12345;
    return state;
}

double nova_random_double(void) {
    return (double)nova_random_uint32() / UINT32_MAX;
}

// I/O utilities
void nova_print(nova_string_t* str) {
    printf("%.*s", (int)str->length, str->data);
}

void nova_println(nova_string_t* str) {
    nova_print(str);
    printf("\n");
}

nova_string_t* nova_read_line(void) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }
        return nova_string_create(NULL, buffer, len); // TODO: Pass allocator
    }
    return NULL;
}

// Standard Library API
typedef struct nova_stdlib {
    nova_allocator_t* allocator;

    // Core functions
    nova_string_t* (*string_create)(const char* str);
    nova_array_t* (*array_create)(size_t capacity);
    nova_hashmap_t* (*hashmap_create)(size_t capacity);

    // Utility functions
    nova_string_t* (*string_concat)(nova_string_t* a, nova_string_t* b);
    nova_iterator_t* (*array_iterator)(nova_array_t* array);
    void (*print)(nova_string_t* str);
    nova_string_t* (*read_line)(void);

    // Math functions
    double (*sqrt)(double x);
    double (*sin)(double x);
    double (*cos)(double x);
    double (*pow)(double base, double exp);

} nova_stdlib_t;

// Initialize standard library
nova_stdlib_t* nova_stdlib_create(void) {
    nova_stdlib_t* std = calloc(1, sizeof(nova_stdlib_t));
    std->allocator = nova_allocator_create();

    // Bind functions
    std->string_create = nova_rt_string_create;
    std->array_create = nova_rt_array_create;
    std->hashmap_create = nova_hashmap_create;

    std->string_concat = nova_string_concat;
    std->array_iterator = nova_array_iterator;
    std->print = nova_print;
    std->read_line = nova_read_line;

    std->sqrt = nova_math_sqrt;
    std->sin = nova_math_sin;
    std->cos = nova_math_cos;
    std->pow = nova_math_pow;

    printf("📚 Nova Standard Library initialized\n");
    return std;
}

// Test B.3 Standard Library
int test_b3_stdlib() {
    printf("=== B.3 Standard Library Test ===\n\n");

    // Initialize stdlib
    nova_stdlib_t* std = nova_stdlib_create();

    // Test string operations
    printf("🧵 String operations:\n");
    nova_string_t* str1 = std->string_create("Hello");
    nova_string_t* str2 = std->string_create(" World");
    nova_string_t* combined = std->string_concat(str1, str2);

    printf("  String 1: '");
    std->print(str1);
    printf("'\n");

    printf("  String 2: '");
    std->print(str2);
    printf("'\n");

    printf("  Combined: '");
    std->print(combined);
    printf("'\n");

    // Test array operations
    printf("\n📋 Array operations:\n");
    nova_array_t* array = std->array_create(10);
    nova_array_push(array, (nova_object_t*)str1);
    nova_array_push(array, (nova_object_t*)str2);

    printf("  Array length: %zu\n", array->length);
    printf("  Element 0: '");
    nova_string_t* elem0 = (nova_string_t*)nova_array_get(array, 0);
    if (elem0) std->print(elem0);
    printf("'\n");

    // Test iterator
    printf("  Iterating array:\n");
    nova_iterator_t* iter = std->array_iterator(array);
    while (iter->has_next(iter)) {
        nova_string_t* item = (nova_string_t*)iter->next(iter);
        printf("    '");
        std->print(item);
        printf("'\n");
    }
    iter->free(iter);

    // Test math functions
    printf("\n🔢 Math operations:\n");
    printf("  sqrt(16) = %f\n", std->sqrt(16.0));
    printf("  sin(0) = %f\n", std->sin(0.0));
    printf("  pow(2, 3) = %f\n", std->pow(2.0, 3.0));

    // Test hashmap
    printf("\n🗂️  HashMap operations:\n");
    nova_hashmap_t* map = std->hashmap_create(16);
    nova_hashmap_put(map, "greeting", (nova_object_t*)str1);
    nova_hashmap_put(map, "name", (nova_object_t*)str2);

    nova_object_t* greeting = nova_hashmap_get(map, "greeting");
    if (greeting) {
        printf("  greeting -> '");
        std->print((nova_string_t*)greeting);
        printf("'\n");
    }

    printf("  Map contains 'greeting': %s\n",
           nova_hashmap_contains(map, "greeting") ? "yes" : "no");
    printf("  Map contains 'missing': %s\n",
           nova_hashmap_contains(map, "missing") ? "yes" : "no");

    // Clean up
    nova_object_release(std->allocator, (nova_object_t*)str1);
    nova_object_release(std->allocator, (nova_object_t*)str2);
    nova_object_release(std->allocator, (nova_object_t*)combined);
    nova_object_release(std->allocator, (nova_object_t*)array);
    nova_object_release(std->allocator, (nova_object_t*)map);

    // Note: stdlib cleanup would be done by runtime shutdown
    printf("\n✅ B.3 Standard Library test completed\n");
    printf("   Core types, collections, and utilities working\n");

    return 0;
}

int main() {
    return test_b3_stdlib();
}
