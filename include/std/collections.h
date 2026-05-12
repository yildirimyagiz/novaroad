/**
 * @file collections.h
 * @brief Data structures (vector, hashmap, set, list, deque, etc.)
 *
 * Nova provides generic, type-safe collections:
 * - Vec: Dynamic array with O(1) amortized append
 * - HashMap: Robin Hood hash table with O(1) average lookup
 * - HashSet: Set backed by HashMap
 * - LinkedList: Doubly-linked list
 * - Deque: Double-ended queue
 * - BTree: Balanced B-tree for ordered data
 */

#ifndef NOVA_COLLECTIONS_H
#define NOVA_COLLECTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Dynamic Array (Vector)
 * ======================================================================== */

typedef struct nova_vec nova_vec_t;

/**
 * @brief Create new vector
 * @return New vector, or NULL on failure
 */
nova_vec_t *nova_vec_new(void);

/**
 * @brief Create vector with capacity
 * @param capacity Initial capacity
 * @return New vector, or NULL on failure
 */
nova_vec_t *nova_vec_with_capacity(size_t capacity);

/**
 * @brief Push item to end
 * @param vec Vector
 * @param item Item to push
 * @return true on success, false on failure
 */
bool nova_vec_push(nova_vec_t *vec, void *item);

/**
 * @brief Pop item from end
 * @param vec Vector
 * @return Popped item, or NULL if empty
 */
void *nova_vec_pop(nova_vec_t *vec);

/**
 * @brief Get item at index
 * @param vec Vector
 * @param index Index
 * @return Item, or NULL if out of bounds
 */
void *nova_vec_get(nova_vec_t *vec, size_t index);

/**
 * @brief Set item at index
 * @param vec Vector
 * @param index Index
 * @param item New item
 * @return Previous item, or NULL if out of bounds
 */
void *nova_vec_set(nova_vec_t *vec, size_t index, void *item);

/**
 * @brief Insert item at index
 * @param vec Vector
 * @param index Index
 * @param item Item to insert
 * @return true on success
 */
bool nova_vec_insert(nova_vec_t *vec, size_t index, void *item);

/**
 * @brief Remove item at index
 * @param vec Vector
 * @param index Index
 * @return Removed item, or NULL if out of bounds
 */
void *nova_vec_remove(nova_vec_t *vec, size_t index);

/**
 * @brief Get vector length
 * @param vec Vector
 * @return Number of items
 */
size_t nova_vec_len(nova_vec_t *vec);

/**
 * @brief Get vector capacity
 * @param vec Vector
 * @return Reserved capacity
 */
size_t nova_vec_capacity(nova_vec_t *vec);

/**
 * @brief Check if vector is empty
 * @param vec Vector
 * @return true if empty
 */
bool nova_vec_is_empty(nova_vec_t *vec);

/**
 * @brief Clear vector (keep capacity)
 * @param vec Vector
 */
void nova_vec_clear(nova_vec_t *vec);

/**
 * @brief Reserve capacity
 * @param vec Vector
 * @param additional Additional capacity to reserve
 */
void nova_vec_reserve(nova_vec_t *vec, size_t additional);

/**
 * @brief Shrink capacity to fit length
 * @param vec Vector
 */
void nova_vec_shrink_to_fit(nova_vec_t *vec);

/**
 * @brief Sort vector
 * @param vec Vector
 * @param cmp Comparison function
 */
void nova_vec_sort(nova_vec_t *vec, int (*cmp)(const void *, const void *));

/**
 * @brief Reverse vector
 * @param vec Vector
 */
void nova_vec_reverse(nova_vec_t *vec);

/**
 * @brief Destroy vector
 * @param vec Vector
 */
void nova_vec_destroy(nova_vec_t *vec);

/* Vector iteration */
typedef struct nova_vec_iter nova_vec_iter_t;

nova_vec_iter_t *nova_vec_iter(nova_vec_t *vec);
bool nova_vec_iter_next(nova_vec_iter_t *iter, void **item);
void nova_vec_iter_destroy(nova_vec_iter_t *iter);

/* ========================================================================
 * Hash Map
 * ======================================================================== */

typedef struct nova_hashmap nova_hashmap_t;
typedef struct nova_hashmap_entry nova_hashmap_entry_t;

/**
 * @brief Create new hashmap
 * @return New hashmap, or NULL on failure
 */
nova_hashmap_t *nova_hashmap_new(void);

/**
 * @brief Create hashmap with capacity
 * @param capacity Initial capacity
 * @return New hashmap, or NULL on failure
 */
nova_hashmap_t *nova_hashmap_with_capacity(size_t capacity);

/**
 * @brief Insert key-value pair
 * @param map HashMap
 * @param key Key (copied)
 * @param value Value
 * @return Previous value if key existed, NULL otherwise
 */
void nova_hashmap_insert(nova_hashmap_t *map, const char *key, void *value);

/**
 * @brief Get value for key
 * @param map HashMap
 * @param key Key
 * @return Value, or NULL if not found
 */
void *nova_hashmap_get(nova_hashmap_t *map, const char *key);

/**
 * @brief Check if key exists
 * @param map HashMap
 * @param key Key
 * @return true if key exists
 */
bool nova_hashmap_contains(nova_hashmap_t *map, const char *key);

/**
 * @brief Remove key-value pair
 * @param map HashMap
 * @param key Key
 * @return Removed value, or NULL if not found
 */
void nova_hashmap_remove(nova_hashmap_t *map, const char *key);

/**
 * @brief Get map size
 * @param map HashMap
 * @return Number of entries
 */
size_t nova_hashmap_size(nova_hashmap_t *map);

/**
 * @brief Check if map is empty
 * @param map HashMap
 * @return true if empty
 */
bool nova_hashmap_is_empty(nova_hashmap_t *map);

/**
 * @brief Clear map
 * @param map HashMap
 */
void nova_hashmap_clear(nova_hashmap_t *map);

/**
 * @brief Destroy map
 * @param map HashMap
 */
void nova_hashmap_destroy(nova_hashmap_t *map);

/* HashMap iteration */
typedef struct nova_hashmap_iter nova_hashmap_iter_t;

nova_hashmap_iter_t *nova_hashmap_iter(nova_hashmap_t *map);
bool nova_hashmap_iter_next(nova_hashmap_iter_t *iter, const char **key, void **value);
void nova_hashmap_iter_destroy(nova_hashmap_iter_t *iter);

/* ========================================================================
 * Hash Set
 * ======================================================================== */

typedef struct nova_hashset nova_hashset_t;

/**
 * @brief Create new hashset
 * @return New hashset, or NULL on failure
 */
nova_hashset_t *nova_hashset_new(void);

/**
 * @brief Insert item
 * @param set HashSet
 * @param item Item (copied if string)
 * @return true if inserted, false if already present
 */
bool nova_hashset_insert(nova_hashset_t *set, const char *item);

/**
 * @brief Check if item exists
 * @param set HashSet
 * @param item Item
 * @return true if present
 */
bool nova_hashset_contains(nova_hashset_t *set, const char *item);

/**
 * @brief Remove item
 * @param set HashSet
 * @param item Item
 * @return true if removed
 */
bool nova_hashset_remove(nova_hashset_t *set, const char *item);

/**
 * @brief Get set size
 * @param set HashSet
 * @return Number of items
 */
size_t nova_hashset_size(nova_hashset_t *set);

/**
 * @brief Destroy set
 * @param set HashSet
 */
void nova_hashset_destroy(nova_hashset_t *set);

/* ========================================================================
 * Linked List (Doubly-linked)
 * ======================================================================== */

typedef struct nova_list nova_list_t;
typedef struct nova_list_node nova_list_node_t;

/**
 * @brief Create new list
 * @return New list, or NULL on failure
 */
nova_list_t *nova_list_new(void);

/**
 * @brief Push item to front
 * @param list List
 * @param item Item
 */
void nova_list_push_front(nova_list_t *list, void *item);

/**
 * @brief Push item to back
 * @param list List
 * @param item Item
 */
void nova_list_push_back(nova_list_t *list, void *item);

/**
 * @brief Pop item from front
 * @param list List
 * @return Item, or NULL if empty
 */
void *nova_list_pop_front(nova_list_t *list);

/**
 * @brief Pop item from back
 * @param list List
 * @return Item, or NULL if empty
 */
void *nova_list_pop_back(nova_list_t *list);

/**
 * @brief Get list length
 * @param list List
 * @return Number of items
 */
size_t nova_list_len(nova_list_t *list);

/**
 * @brief Destroy list
 * @param list List
 */
void nova_list_destroy(nova_list_t *list);

/* ========================================================================
 * Deque (Double-ended queue)
 * ======================================================================== */

typedef struct nova_deque nova_deque_t;

/**
 * @brief Create new deque
 * @return New deque, or NULL on failure
 */
nova_deque_t *nova_deque_new(void);

/**
 * @brief Push item to front
 * @param deque Deque
 * @param item Item
 */
void nova_deque_push_front(nova_deque_t *deque, void *item);

/**
 * @brief Push item to back
 * @param deque Deque
 * @param item Item
 */
void nova_deque_push_back(nova_deque_t *deque, void *item);

/**
 * @brief Pop item from front
 * @param deque Deque
 * @return Item, or NULL if empty
 */
void *nova_deque_pop_front(nova_deque_t *deque);

/**
 * @brief Pop item from back
 * @param deque Deque
 * @return Item, or NULL if empty
 */
void *nova_deque_pop_back(nova_deque_t *deque);

/**
 * @brief Get item at index
 * @param deque Deque
 * @param index Index
 * @return Item, or NULL if out of bounds
 */
void *nova_deque_get(nova_deque_t *deque, size_t index);

/**
 * @brief Get deque length
 * @param deque Deque
 * @return Number of items
 */
size_t nova_deque_len(nova_deque_t *deque);

/**
 * @brief Destroy deque
 * @param deque Deque
 */
void nova_deque_destroy(nova_deque_t *deque);

/* ========================================================================
 * B-Tree (Ordered map)
 * ======================================================================== */

typedef struct nova_btree nova_btree_t;

/**
 * @brief Create new B-tree
 * @param order B-tree order (typically 32-128)
 * @return New B-tree, or NULL on failure
 */
nova_btree_t *nova_btree_new(size_t order);

/**
 * @brief Insert key-value pair
 * @param tree B-tree
 * @param key Key (integer)
 * @param value Value
 */
void nova_btree_insert(nova_btree_t *tree, uint64_t key, void *value);

/**
 * @brief Get value for key
 * @param tree B-tree
 * @param key Key
 * @return Value, or NULL if not found
 */
void *nova_btree_get(nova_btree_t *tree, uint64_t key);

/**
 * @brief Remove key-value pair
 * @param tree B-tree
 * @param key Key
 * @return Removed value, or NULL if not found
 */
void *nova_btree_remove(nova_btree_t *tree, uint64_t key);

/**
 * @brief Destroy B-tree
 * @param tree B-tree
 */
void nova_btree_destroy(nova_btree_t *tree);

/* ========================================================================
 * Priority Queue (Binary heap)
 * ======================================================================== */

typedef struct nova_pqueue nova_pqueue_t;

/**
 * @brief Create new priority queue
 * @param cmp Comparison function (min-heap if cmp(a,b) < 0 means a has higher priority)
 * @return New priority queue, or NULL on failure
 */
nova_pqueue_t *nova_pqueue_new(int (*cmp)(const void *, const void *));

/**
 * @brief Push item
 * @param pq Priority queue
 * @param item Item
 */
void nova_pqueue_push(nova_pqueue_t *pq, void *item);

/**
 * @brief Pop highest priority item
 * @param pq Priority queue
 * @return Item, or NULL if empty
 */
void *nova_pqueue_pop(nova_pqueue_t *pq);

/**
 * @brief Peek highest priority item
 * @param pq Priority queue
 * @return Item, or NULL if empty
 */
void *nova_pqueue_peek(nova_pqueue_t *pq);

/**
 * @brief Get queue size
 * @param pq Priority queue
 * @return Number of items
 */
size_t nova_pqueue_size(nova_pqueue_t *pq);

/**
 * @brief Destroy priority queue
 * @param pq Priority queue
 */
void nova_pqueue_destroy(nova_pqueue_t *pq);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_COLLECTIONS_H */
