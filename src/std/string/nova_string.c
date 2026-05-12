/**
 * @file nova_string.c
 * @brief UTF-8 string implementation
 */

#include "std/string.h"
#include "std/alloc.h"
#include <string.h>

struct nova_string {
    char *data;
    size_t length;
    size_t capacity;
};

nova_string_t *nova_string_new(const char *cstr)
{
    nova_string_t *str = nova_alloc(sizeof(nova_string_t));
    if (!str) return NULL;
    
    str->length = strlen(cstr);
    str->capacity = str->length + 1;
    str->data = nova_alloc(str->capacity);
    
    if (str->data) {
        memcpy(str->data, cstr, str->length + 1);
    }
    
    return str;
}

nova_string_t *nova_string_with_capacity(size_t capacity)
{
    nova_string_t *str = nova_alloc(sizeof(nova_string_t));
    if (!str) return NULL;
    
    str->length = 0;
    str->capacity = capacity;
    str->data = nova_alloc(capacity);
    
    if (str->data) {
        str->data[0] = '\0';
    }
    
    return str;
}

size_t nova_string_len(const nova_string_t *str)
{
    return str->length;
}

const char *nova_string_cstr(const nova_string_t *str)
{
    return str->data;
}

nova_string_t *nova_string_concat(const nova_string_t *a, const nova_string_t *b)
{
    size_t new_len = a->length + b->length;
    nova_string_t *result = nova_string_with_capacity(new_len + 1);
    
    if (result && result->data) {
        memcpy(result->data, a->data, a->length);
        memcpy(result->data + a->length, b->data, b->length);
        result->data[new_len] = '\0';
        result->length = new_len;
    }
    
    return result;
}

int nova_string_cmp(const nova_string_t *a, const nova_string_t *b)
{
    return strcmp(a->data, b->data);
}

void nova_string_destroy(nova_string_t *str)
{
    if (str) {
        nova_free(str->data);
        nova_free(str);
    }
}
