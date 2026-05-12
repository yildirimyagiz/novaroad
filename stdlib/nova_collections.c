/**
 * Nova Collections Implementation
 */

#include "nova_collections.h"
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// VECTOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

#define INITIAL_VEC_CAPACITY 16

NovaVec *nova_vec_new(void) {
  return nova_vec_with_capacity(INITIAL_VEC_CAPACITY);
}

NovaVec *nova_vec_with_capacity(size_t capacity) {
  NovaVec *vec = (NovaVec *)malloc(sizeof(NovaVec));
  if (!vec) return NULL;
  
  vec->data = (void **)malloc(capacity * sizeof(void*));
  vec->length = 0;
  vec->capacity = capacity;
  vec->destructor = NULL;
  
  return vec;
}

void nova_vec_destroy(NovaVec *vec) {
  if (!vec) return;
  
  if (vec->destructor) {
    for (size_t i = 0; i < vec->length; i++) {
      vec->destructor(vec->data[i]);
    }
  }
  
  free(vec->data);
  free(vec);
}

void nova_vec_set_destructor(NovaVec *vec, void (*destructor)(void*)) {
  if (vec) vec->destructor = destructor;
}

static void nova_vec_ensure_capacity(NovaVec *vec, size_t needed) {
  if (needed <= vec->capacity) return;
  
  size_t new_capacity = vec->capacity * 2;
  while (new_capacity < needed) {
    new_capacity *= 2;
  }
  
  vec->data = (void **)realloc(vec->data, new_capacity * sizeof(void*));
  vec->capacity = new_capacity;
}

void nova_vec_push(NovaVec *vec, void *element) {
  if (!vec) return;
  
  nova_vec_ensure_capacity(vec, vec->length + 1);
  vec->data[vec->length++] = element;
}

void *nova_vec_pop(NovaVec *vec) {
  if (!vec || vec->length == 0) return NULL;
  
  return vec->data[--vec->length];
}

void *nova_vec_get(const NovaVec *vec, size_t index) {
  if (!vec || index >= vec->length) return NULL;
  return vec->data[index];
}

void nova_vec_set(NovaVec *vec, size_t index, void *element) {
  if (!vec || index >= vec->length) return;
  
  if (vec->destructor) {
    vec->destructor(vec->data[index]);
  }
  
  vec->data[index] = element;
}

void nova_vec_insert(NovaVec *vec, size_t index, void *element) {
  if (!vec || index > vec->length) return;
  
  nova_vec_ensure_capacity(vec, vec->length + 1);
  memmove(&vec->data[index + 1], &vec->data[index], 
          (vec->length - index) * sizeof(void*));
  vec->data[index] = element;
  vec->length++;
}

void nova_vec_remove(NovaVec *vec, size_t index) {
  if (!vec || index >= vec->length) return;
  
  if (vec->destructor) {
    vec->destructor(vec->data[index]);
  }
  
  memmove(&vec->data[index], &vec->data[index + 1],
          (vec->length - index - 1) * sizeof(void*));
  vec->length--;
}

void nova_vec_clear(NovaVec *vec) {
  if (!vec) return;
  
  if (vec->destructor) {
    for (size_t i = 0; i < vec->length; i++) {
      vec->destructor(vec->data[i]);
    }
  }
  
  vec->length = 0;
}

size_t nova_vec_len(const NovaVec *vec) {
  return vec ? vec->length : 0;
}

bool nova_vec_is_empty(const NovaVec *vec) {
  return !vec || vec->length == 0;
}

void **nova_vec_as_array(const NovaVec *vec) {
  return vec ? vec->data : NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// HASH MAP IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

#define INITIAL_HASHMAP_CAPACITY 32
#define LOAD_FACTOR_THRESHOLD 0.75

static size_t hash_string(const char *str) {
  size_t hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

NovaHashMap *nova_hashmap_new(void) {
  return nova_hashmap_with_capacity(INITIAL_HASHMAP_CAPACITY);
}

NovaHashMap *nova_hashmap_with_capacity(size_t capacity) {
  NovaHashMap *map = (NovaHashMap *)malloc(sizeof(NovaHashMap));
  if (!map) return NULL;
  
  map->buckets = (HashMapEntry **)calloc(capacity, sizeof(HashMapEntry*));
  map->capacity = capacity;
  map->size = 0;
  map->value_destructor = NULL;
  
  return map;
}

void nova_hashmap_destroy(NovaHashMap *map) {
  if (!map) return;
  
  for (size_t i = 0; i < map->capacity; i++) {
    HashMapEntry *entry = map->buckets[i];
    while (entry) {
      HashMapEntry *next = entry->next;
      free(entry->key);
      if (map->value_destructor) {
        map->value_destructor(entry->value);
      }
      free(entry);
      entry = next;
    }
  }
  
  free(map->buckets);
  free(map);
}

void nova_hashmap_set_destructor(NovaHashMap *map, void (*destructor)(void*)) {
  if (map) map->value_destructor = destructor;
}

static void nova_hashmap_resize(NovaHashMap *map) {
  size_t new_capacity = map->capacity * 2;
  HashMapEntry **new_buckets = (HashMapEntry **)calloc(new_capacity, sizeof(HashMapEntry*));
  
  // Rehash all entries
  for (size_t i = 0; i < map->capacity; i++) {
    HashMapEntry *entry = map->buckets[i];
    while (entry) {
      HashMapEntry *next = entry->next;
      
      size_t new_index = hash_string(entry->key) % new_capacity;
      entry->next = new_buckets[new_index];
      new_buckets[new_index] = entry;
      
      entry = next;
    }
  }
  
  free(map->buckets);
  map->buckets = new_buckets;
  map->capacity = new_capacity;
}

void nova_hashmap_insert(NovaHashMap *map, const char *key, void *value) {
  if (!map || !key) return;
  
  // Check load factor
  if ((double)map->size / map->capacity > LOAD_FACTOR_THRESHOLD) {
    nova_hashmap_resize(map);
  }
  
  size_t index = hash_string(key) % map->capacity;
  HashMapEntry *entry = map->buckets[index];
  
  // Check if key exists
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      if (map->value_destructor) {
        map->value_destructor(entry->value);
      }
      entry->value = value;
      return;
    }
    entry = entry->next;
  }
  
  // Create new entry
  HashMapEntry *new_entry = (HashMapEntry *)malloc(sizeof(HashMapEntry));
  new_entry->key = strdup(key);
  new_entry->value = value;
  new_entry->next = map->buckets[index];
  map->buckets[index] = new_entry;
  map->size++;
}

void *nova_hashmap_get(const NovaHashMap *map, const char *key) {
  if (!map || !key) return NULL;
  
  size_t index = hash_string(key) % map->capacity;
  HashMapEntry *entry = map->buckets[index];
  
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }
  
  return NULL;
}

bool nova_hashmap_contains(const NovaHashMap *map, const char *key) {
  return nova_hashmap_get(map, key) != NULL;
}

void *nova_hashmap_remove(NovaHashMap *map, const char *key) {
  if (!map || !key) return NULL;
  
  size_t index = hash_string(key) % map->capacity;
  HashMapEntry *entry = map->buckets[index];
  HashMapEntry *prev = NULL;
  
  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      if (prev) {
        prev->next = entry->next;
      } else {
        map->buckets[index] = entry->next;
      }
      
      void *value = entry->value;
      free(entry->key);
      free(entry);
      map->size--;
      
      return value;
    }
    prev = entry;
    entry = entry->next;
  }
  
  return NULL;
}

void nova_hashmap_clear(NovaHashMap *map) {
  if (!map) return;
  
  for (size_t i = 0; i < map->capacity; i++) {
    HashMapEntry *entry = map->buckets[i];
    while (entry) {
      HashMapEntry *next = entry->next;
      free(entry->key);
      if (map->value_destructor) {
        map->value_destructor(entry->value);
      }
      free(entry);
      entry = next;
    }
    map->buckets[i] = NULL;
  }
  
  map->size = 0;
}

size_t nova_hashmap_size(const NovaHashMap *map) {
  return map ? map->size : 0;
}

bool nova_hashmap_is_empty(const NovaHashMap *map) {
  return !map || map->size == 0;
}

NovaHashMapIter nova_hashmap_iter(const NovaHashMap *map) {
  NovaHashMapIter iter = {
    .map = map,
    .bucket_index = 0,
    .current = map ? map->buckets[0] : NULL
  };
  return iter;
}

bool nova_hashmap_iter_next(NovaHashMapIter *iter, const char **key, void **value) {
  if (!iter || !iter->map) return false;
  
  // Find next non-null entry
  while (!iter->current && iter->bucket_index < iter->map->capacity - 1) {
    iter->bucket_index++;
    iter->current = iter->map->buckets[iter->bucket_index];
  }
  
  if (!iter->current) return false;
  
  *key = iter->current->key;
  *value = iter->current->value;
  iter->current = iter->current->next;
  
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// LINKED LIST IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

NovaList *nova_list_new(void) {
  NovaList *list = (NovaList *)calloc(1, sizeof(NovaList));
  return list;
}

void nova_list_destroy(NovaList *list) {
  if (!list) return;
  
  nova_list_clear(list);
  free(list);
}

void nova_list_set_destructor(NovaList *list, void (*destructor)(void*)) {
  if (list) list->destructor = destructor;
}

void nova_list_push_front(NovaList *list, void *data) {
  if (!list) return;
  
  ListNode *node = (ListNode *)malloc(sizeof(ListNode));
  node->data = data;
  node->next = list->head;
  node->prev = NULL;
  
  if (list->head) {
    list->head->prev = node;
  } else {
    list->tail = node;
  }
  
  list->head = node;
  list->length++;
}

void nova_list_push_back(NovaList *list, void *data) {
  if (!list) return;
  
  ListNode *node = (ListNode *)malloc(sizeof(ListNode));
  node->data = data;
  node->next = NULL;
  node->prev = list->tail;
  
  if (list->tail) {
    list->tail->next = node;
  } else {
    list->head = node;
  }
  
  list->tail = node;
  list->length++;
}

void *nova_list_pop_front(NovaList *list) {
  if (!list || !list->head) return NULL;
  
  ListNode *node = list->head;
  void *data = node->data;
  
  list->head = node->next;
  if (list->head) {
    list->head->prev = NULL;
  } else {
    list->tail = NULL;
  }
  
  free(node);
  list->length--;
  
  return data;
}

void *nova_list_pop_back(NovaList *list) {
  if (!list || !list->tail) return NULL;
  
  ListNode *node = list->tail;
  void *data = node->data;
  
  list->tail = node->prev;
  if (list->tail) {
    list->tail->next = NULL;
  } else {
    list->head = NULL;
  }
  
  free(node);
  list->length--;
  
  return data;
}

void nova_list_clear(NovaList *list) {
  if (!list) return;
  
  ListNode *node = list->head;
  while (node) {
    ListNode *next = node->next;
    if (list->destructor) {
      list->destructor(node->data);
    }
    free(node);
    node = next;
  }
  
  list->head = NULL;
  list->tail = NULL;
  list->length = 0;
}

size_t nova_list_len(const NovaList *list) {
  return list ? list->length : 0;
}

bool nova_list_is_empty(const NovaList *list) {
  return !list || list->length == 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// SET IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

NovaSet *nova_set_new(void) {
  NovaSet *set = (NovaSet *)malloc(sizeof(NovaSet));
  set->map = nova_hashmap_new();
  return set;
}

void nova_set_destroy(NovaSet *set) {
  if (!set) return;
  nova_hashmap_destroy(set->map);
  free(set);
}

void nova_set_insert(NovaSet *set, const char *element) {
  if (!set) return;
  nova_hashmap_insert(set->map, element, (void*)1);
}

bool nova_set_contains(const NovaSet *set, const char *element) {
  if (!set) return false;
  return nova_hashmap_contains(set->map, element);
}

void nova_set_remove(NovaSet *set, const char *element) {
  if (!set) return;
  nova_hashmap_remove(set->map, element);
}

size_t nova_set_size(const NovaSet *set) {
  return set ? nova_hashmap_size(set->map) : 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// QUEUE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

NovaQueue *nova_queue_new(void) {
  NovaQueue *queue = (NovaQueue *)malloc(sizeof(NovaQueue));
  queue->list = nova_list_new();
  return queue;
}

void nova_queue_destroy(NovaQueue *queue) {
  if (!queue) return;
  nova_list_destroy(queue->list);
  free(queue);
}

void nova_queue_enqueue(NovaQueue *queue, void *data) {
  if (!queue) return;
  nova_list_push_back(queue->list, data);
}

void *nova_queue_dequeue(NovaQueue *queue) {
  if (!queue) return NULL;
  return nova_list_pop_front(queue->list);
}

size_t nova_queue_len(const NovaQueue *queue) {
  return queue ? nova_list_len(queue->list) : 0;
}

bool nova_queue_is_empty(const NovaQueue *queue) {
  return !queue || nova_list_is_empty(queue->list);
}

// ═══════════════════════════════════════════════════════════════════════════
// STACK IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

NovaStack *nova_stack_new(void) {
  NovaStack *stack = (NovaStack *)malloc(sizeof(NovaStack));
  stack->vec = nova_vec_new();
  return stack;
}

void nova_stack_destroy(NovaStack *stack) {
  if (!stack) return;
  nova_vec_destroy(stack->vec);
  free(stack);
}

void nova_stack_push(NovaStack *stack, void *data) {
  if (!stack) return;
  nova_vec_push(stack->vec, data);
}

void *nova_stack_pop(NovaStack *stack) {
  if (!stack) return NULL;
  return nova_vec_pop(stack->vec);
}

void *nova_stack_peek(const NovaStack *stack) {
  if (!stack || nova_vec_is_empty(stack->vec)) return NULL;
  return nova_vec_get(stack->vec, nova_vec_len(stack->vec) - 1);
}

size_t nova_stack_len(const NovaStack *stack) {
  return stack ? nova_vec_len(stack->vec) : 0;
}

bool nova_stack_is_empty(const NovaStack *stack) {
  return !stack || nova_vec_is_empty(stack->vec);
}
