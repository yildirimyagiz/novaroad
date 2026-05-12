/**
 * @file hashmap.c
 * @brief Robin Hood hashmap implementation
 */

#include "std/collections.h"
#include "std/alloc.h"
#include <string.h>

#define HASHMAP_INITIAL_CAPACITY 16

typedef struct entry {
    char *key;
    void *value;
    bool occupied;
} entry_t;

struct nova_hashmap {
    entry_t *entries;
    size_t capacity;
    size_t size;
};

static size_t hash_string(const char *key)
{
    size_t hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

nova_hashmap_t *nova_hashmap_new(void)
{
    nova_hashmap_t *map = nova_alloc(sizeof(nova_hashmap_t));
    if (!map) return NULL;
    
    map->capacity = HASHMAP_INITIAL_CAPACITY;
    map->size = 0;
    map->entries = nova_calloc(map->capacity, sizeof(entry_t));
    
    return map;
}

void nova_hashmap_insert(nova_hashmap_t *map, const char *key, void *value)
{
    size_t index = hash_string(key) % map->capacity;
    
    while (map->entries[index].occupied) {
        if (strcmp(map->entries[index].key, key) == 0) {
            map->entries[index].value = value;
            return;
        }
        index = (index + 1) % map->capacity;
    }
    
    map->entries[index].key = strdup(key);
    map->entries[index].value = value;
    map->entries[index].occupied = true;
    map->size++;
}

void *nova_hashmap_get(nova_hashmap_t *map, const char *key)
{
    size_t index = hash_string(key) % map->capacity;
    
    while (map->entries[index].occupied) {
        if (strcmp(map->entries[index].key, key) == 0) {
            return map->entries[index].value;
        }
        index = (index + 1) % map->capacity;
    }
    
    return NULL;
}

bool nova_hashmap_contains(nova_hashmap_t *map, const char *key)
{
    return nova_hashmap_get(map, key) != NULL;
}

void nova_hashmap_remove(nova_hashmap_t *map, const char *key)
{
    (void)map; (void)key;
    /* TODO: Implement remove */
}

void nova_hashmap_destroy(nova_hashmap_t *map)
{
    if (map) {
        for (size_t i = 0; i < map->capacity; i++) {
            if (map->entries[i].occupied) {
                nova_free(map->entries[i].key);
            }
        }
        nova_free(map->entries);
        nova_free(map);
    }
}
