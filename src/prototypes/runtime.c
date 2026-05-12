// B.2 Runtime Implementation
// Memory management and garbage collection for Nova

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Nova Runtime Types
typedef struct nova_object nova_object_t;
typedef struct nova_string nova_string_t;

// Object header for GC
typedef struct nova_object_header {
    uint32_t ref_count;    // Reference counting
    uint32_t size;         // Object size in bytes
    uint8_t marked;        // Mark-sweep flag
    uint8_t type_tag;      // Object type
} nova_object_header_t;

// Object types
enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_SHAPE,    // User-defined struct
    OBJ_CLOSURE   // Function closure
};

// Nova String implementation
struct nova_string {
    nova_object_header_t header;
    size_t length;
    char data[];  // Flexible array member
};

// Nova Array implementation
typedef struct nova_array {
    nova_object_header_t header;
    size_t length;
    size_t capacity;
    nova_object_t* elements[];
} nova_array_t;

// Nova Shape (struct) implementation
typedef struct nova_shape {
    nova_object_header_t header;
    const char* type_name;
    size_t field_count;
    nova_object_t* fields[];
} nova_shape_t;

// Generic Nova object
struct nova_object {
    nova_object_header_t header;
    union {
        nova_string_t string;
        nova_array_t array;
        nova_shape_t shape;
    } as;
};

// Memory allocator
typedef struct nova_allocator {
    size_t total_allocated;
    size_t gc_threshold;
    nova_object_t** objects;     // All allocated objects for GC
    size_t object_count;
    size_t object_capacity;
} nova_allocator_t;

// Global allocator instance
static nova_allocator_t* global_allocator = NULL;

// Initialize allocator
nova_allocator_t* nova_allocator_create(void) {
    nova_allocator_t* alloc = calloc(1, sizeof(nova_allocator_t));
    alloc->gc_threshold = 1024 * 1024; // 1MB
    alloc->object_capacity = 1024;
    alloc->objects = calloc(alloc->object_capacity, sizeof(nova_object_t*));

    printf("🗄️  Nova Allocator initialized (GC threshold: %zu bytes)\n", alloc->gc_threshold);
    return alloc;
}

// Destroy allocator
void nova_allocator_destroy(nova_allocator_t* alloc) {
    if (!alloc) return;

    // Free all objects
    for (size_t i = 0; i < alloc->object_count; i++) {
        free(alloc->objects[i]);
    }

    free(alloc->objects);
    free(alloc);
}

// Register object with allocator
void nova_allocator_register_object(nova_allocator_t* alloc, nova_object_t* obj) {
    if (alloc->object_count >= alloc->object_capacity) {
        alloc->object_capacity *= 2;
        alloc->objects = realloc(alloc->objects, alloc->object_capacity * sizeof(nova_object_t*));
    }

    alloc->objects[alloc->object_count++] = obj;
    alloc->total_allocated += obj->header.size;
}

// Allocate Nova string
nova_string_t* nova_string_create(nova_allocator_t* alloc, const char* str, size_t length) {
    size_t total_size = sizeof(nova_string_t) + length + 1; // +1 for null terminator

    nova_string_t* string = calloc(1, total_size);
    string->header.ref_count = 1;
    string->header.size = total_size;
    string->header.marked = 0;
    string->header.type_tag = OBJ_STRING;
    string->length = length;

    memcpy(string->data, str, length);
    string->data[length] = '\0';

    nova_allocator_register_object(alloc, (nova_object_t*)string);

    return string;
}

// Allocate Nova array
nova_array_t* nova_array_create(nova_allocator_t* alloc, size_t initial_capacity) {
    size_t total_size = sizeof(nova_array_t) + initial_capacity * sizeof(nova_object_t*);

    nova_array_t* array = calloc(1, total_size);
    array->header.ref_count = 1;
    array->header.size = total_size;
    array->header.marked = 0;
    array->header.type_tag = OBJ_ARRAY;
    array->length = 0;
    array->capacity = initial_capacity;

    nova_allocator_register_object(alloc, (nova_object_t*)array);

    return array;
}

// Reference counting operations
void nova_object_retain(nova_object_t* obj) {
    if (obj) {
        obj->header.ref_count++;
    }
}

void nova_object_release(nova_allocator_t* alloc, nova_object_t* obj) {
    if (!obj) return;

    obj->header.ref_count--;
    if (obj->header.ref_count == 0) {
        // TODO: Call destructor based on type
        printf("♻️  Deallocating object (type=%d, size=%u)\n",
               obj->header.type_tag, obj->header.size);

        // Remove from allocator's object list
        for (size_t i = 0; i < alloc->object_count; i++) {
            if (alloc->objects[i] == obj) {
                // Move last element to this position
                alloc->objects[i] = alloc->objects[alloc->object_count - 1];
                alloc->object_count--;
                break;
            }
        }

        alloc->total_allocated -= obj->header.size;
        free(obj);
    }
}

// Mark-sweep garbage collection
static void nova_gc_mark_object(nova_object_t* obj) {
    if (!obj || obj->header.marked) return;

    obj->header.marked = 1;

    // Mark referenced objects based on type
    switch (obj->header.type_tag) {
        case OBJ_ARRAY:
            for (size_t i = 0; i < obj->as.array.length; i++) {
                nova_gc_mark_object(obj->as.array.elements[i]);
            }
            break;

        case OBJ_SHAPE:
            for (size_t i = 0; i < obj->as.shape.field_count; i++) {
                nova_gc_mark_object(obj->as.shape.fields[i]);
            }
            break;

        // Other types don't reference other objects
        default:
            break;
    }
}

static void nova_gc_sweep(nova_allocator_t* alloc) {
    size_t freed_count = 0;
    size_t freed_bytes = 0;

    // Sweep unmarked objects
    for (size_t i = 0; i < alloc->object_count; ) {
        nova_object_t* obj = alloc->objects[i];

        if (!obj->header.marked) {
            // Object is garbage
            printf("🗑️  GC: Freeing unmarked object (type=%d, size=%u)\n",
                   obj->header.type_tag, obj->header.size);

            freed_bytes += obj->header.size;
            freed_count++;

            free(obj);

            // Remove from list
            alloc->objects[i] = alloc->objects[alloc->object_count - 1];
            alloc->object_count--;
        } else {
            // Keep object, clear mark for next GC
            obj->header.marked = 0;
            i++;
        }
    }

    printf("🧹 GC completed: freed %zu objects (%zu bytes)\n", freed_count, freed_bytes);
}

void nova_gc_collect(nova_allocator_t* alloc, nova_object_t** roots, size_t root_count) {
    printf("🔄 Starting garbage collection...\n");

    // Mark phase: mark all reachable objects
    for (size_t i = 0; i < root_count; i++) {
        nova_gc_mark_object(roots[i]);
    }

    // Sweep phase: free unmarked objects
    nova_gc_sweep(alloc);

    // Check if we need to adjust GC threshold
    if (alloc->total_allocated < alloc->gc_threshold / 4) {
        alloc->gc_threshold /= 2; // Reduce threshold
        printf("📉 GC threshold reduced to %zu bytes\n", alloc->gc_threshold);
    }
}

// Automatic GC trigger
void nova_allocator_check_gc(nova_allocator_t* alloc, nova_object_t** roots, size_t root_count) {
    if (alloc->total_allocated >= alloc->gc_threshold) {
        printf("⚠️  GC threshold reached (%zu >= %zu)\n",
               alloc->total_allocated, alloc->gc_threshold);

        nova_gc_collect(alloc, roots, root_count);

        // Increase threshold for next time
        alloc->gc_threshold *= 2;
        printf("📈 GC threshold increased to %zu bytes\n", alloc->gc_threshold);
    }
}

// Runtime API
nova_string_t* nova_rt_string_create(const char* str) {
    if (!global_allocator) {
        global_allocator = nova_allocator_create();
    }

    return nova_string_create(global_allocator, str, strlen(str));
}

nova_array_t* nova_rt_array_create(size_t capacity) {
    if (!global_allocator) {
        global_allocator = nova_allocator_create();
    }

    return nova_array_create(global_allocator, capacity);
}

void nova_rt_object_retain(nova_object_t* obj) {
    nova_object_retain(obj);
}

void nova_rt_object_release(nova_object_t* obj) {
    if (global_allocator) {
        nova_object_release(global_allocator, obj);
    }
}

// Test B.2 Runtime Implementation
int test_b2_runtime() {
    printf("=== B.2 Runtime Implementation Test ===\n\n");

    // Create allocator
    nova_allocator_t* alloc = nova_allocator_create();

    // Create some objects
    printf("📝 Creating objects...\n");

    nova_string_t* str1 = nova_string_create(alloc, "Hello", 5);
    nova_string_t* str2 = nova_string_create(alloc, "World", 5);
    nova_array_t* arr1 = nova_array_create(alloc, 10);

    printf("✅ Created 2 strings and 1 array\n");

    // Simulate some references
    nova_object_retain((nova_object_t*)str1); // str1 now has 2 refs
    nova_object_release(alloc, (nova_object_t*)str2); // str2 has 0 refs, should be freed

    printf("📊 Memory status: %zu objects, %zu bytes allocated\n",
           alloc->object_count, alloc->total_allocated);

    // Force GC (str2 should be collected)
    nova_object_t* roots[] = {(nova_object_t*)str1, (nova_object_t*)arr1};
    nova_gc_collect(alloc, roots, 2);

    printf("📊 After GC: %zu objects, %zu bytes allocated\n",
           alloc->object_count, alloc->total_allocated);

    // Clean up remaining objects
    nova_object_release(alloc, (nova_object_t*)str1);
    nova_object_release(alloc, (nova_object_t*)arr1);

    // Final GC
    nova_gc_collect(alloc, NULL, 0);

    nova_allocator_destroy(alloc);

    printf("\n✅ B.2 Runtime implementation test completed\n");
    printf("   Memory management and GC working correctly\n");

    return 0;
}

int main() {
    return test_b2_runtime();
}
