/**
 * Nova Collections Library
 * Dynamic arrays, hash maps, and other data structures
 */

#ifndef NOVA_COLLECTIONS_H
#define NOVA_COLLECTIONS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// VECTOR (Dynamic Array)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  void **data;
  size_t length;
  size_t capacity;
  void (*destructor)(void*);  // Optional element destructor
} NovaVec;

// Vector operations
NovaVec *nova_vec_new(void);
NovaVec *nova_vec_with_capacity(size_t capacity);
void nova_vec_destroy(NovaVec *vec);
void nova_vec_set_destructor(NovaVec *vec, void (*destructor)(void*));

void nova_vec_push(NovaVec *vec, void *element);
void *nova_vec_pop(NovaVec *vec);
void *nova_vec_get(const NovaVec *vec, size_t index);
void nova_vec_set(NovaVec *vec, size_t index, void *element);
void nova_vec_insert(NovaVec *vec, size_t index, void *element);
void nova_vec_remove(NovaVec *vec, size_t index);
void nova_vec_clear(NovaVec *vec);

size_t nova_vec_len(const NovaVec *vec);
bool nova_vec_is_empty(const NovaVec *vec);
void **nova_vec_as_array(const NovaVec *vec);

// ═══════════════════════════════════════════════════════════════════════════
// HASH MAP
// ═══════════════════════════════════════════════════════════════════════════

typedef struct HashMapEntry {
  char *key;
  void *value;
  struct HashMapEntry *next;
} HashMapEntry;

typedef struct {
  HashMapEntry **buckets;
  size_t capacity;
  size_t size;
  void (*value_destructor)(void*);
} NovaHashMap;

// HashMap operations
NovaHashMap *nova_hashmap_new(void);
NovaHashMap *nova_hashmap_with_capacity(size_t capacity);
void nova_hashmap_destroy(NovaHashMap *map);
void nova_hashmap_set_destructor(NovaHashMap *map, void (*destructor)(void*));

void nova_hashmap_insert(NovaHashMap *map, const char *key, void *value);
void *nova_hashmap_get(const NovaHashMap *map, const char *key);
bool nova_hashmap_contains(const NovaHashMap *map, const char *key);
void *nova_hashmap_remove(NovaHashMap *map, const char *key);
void nova_hashmap_clear(NovaHashMap *map);

size_t nova_hashmap_size(const NovaHashMap *map);
bool nova_hashmap_is_empty(const NovaHashMap *map);

// Iterator
typedef struct {
  const NovaHashMap *map;
  size_t bucket_index;
  HashMapEntry *current;
} NovaHashMapIter;

NovaHashMapIter nova_hashmap_iter(const NovaHashMap *map);
bool nova_hashmap_iter_next(NovaHashMapIter *iter, const char **key, void **value);

// ═══════════════════════════════════════════════════════════════════════════
// LINKED LIST
// ═══════════════════════════════════════════════════════════════════════════

typedef struct ListNode {
  void *data;
  struct ListNode *next;
  struct ListNode *prev;
} ListNode;

typedef struct {
  ListNode *head;
  ListNode *tail;
  size_t length;
  void (*destructor)(void*);
} NovaList;

// List operations
NovaList *nova_list_new(void);
void nova_list_destroy(NovaList *list);
void nova_list_set_destructor(NovaList *list, void (*destructor)(void*));

void nova_list_push_front(NovaList *list, void *data);
void nova_list_push_back(NovaList *list, void *data);
void *nova_list_pop_front(NovaList *list);
void *nova_list_pop_back(NovaList *list);
void *nova_list_get(const NovaList *list, size_t index);
void nova_list_insert(NovaList *list, size_t index, void *data);
void nova_list_remove(NovaList *list, size_t index);
void nova_list_clear(NovaList *list);

size_t nova_list_len(const NovaList *list);
bool nova_list_is_empty(const NovaList *list);

// ═══════════════════════════════════════════════════════════════════════════
// SET (Hash Set)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaHashMap *map;  // Use HashMap internally, values are NULL
} NovaSet;

NovaSet *nova_set_new(void);
void nova_set_destroy(NovaSet *set);

void nova_set_insert(NovaSet *set, const char *element);
bool nova_set_contains(const NovaSet *set, const char *element);
void nova_set_remove(NovaSet *set, const char *element);
void nova_set_clear(NovaSet *set);

size_t nova_set_size(const NovaSet *set);
bool nova_set_is_empty(const NovaSet *set);

// Set operations
NovaSet *nova_set_union(const NovaSet *a, const NovaSet *b);
NovaSet *nova_set_intersection(const NovaSet *a, const NovaSet *b);
NovaSet *nova_set_difference(const NovaSet *a, const NovaSet *b);

// ═══════════════════════════════════════════════════════════════════════════
// QUEUE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaList *list;
} NovaQueue;

NovaQueue *nova_queue_new(void);
void nova_queue_destroy(NovaQueue *queue);

void nova_queue_enqueue(NovaQueue *queue, void *data);
void *nova_queue_dequeue(NovaQueue *queue);
void *nova_queue_peek(const NovaQueue *queue);

size_t nova_queue_len(const NovaQueue *queue);
bool nova_queue_is_empty(const NovaQueue *queue);

// ═══════════════════════════════════════════════════════════════════════════
// STACK
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaVec *vec;
} NovaStack;

NovaStack *nova_stack_new(void);
void nova_stack_destroy(NovaStack *stack);

void nova_stack_push(NovaStack *stack, void *data);
void *nova_stack_pop(NovaStack *stack);
void *nova_stack_peek(const NovaStack *stack);

size_t nova_stack_len(const NovaStack *stack);
bool nova_stack_is_empty(const NovaStack *stack);

#endif // NOVA_COLLECTIONS_H
