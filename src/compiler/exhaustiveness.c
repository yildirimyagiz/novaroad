/**
 * @file exhaustiveness.c
 * @brief Pattern exhaustiveness checking (STUB)
 */

#include "compiler/nova_pattern.h"
#include <stdbool.h>
#include <stddef.h>

// Stub: Check if patterns are exhaustive
bool check_exhaustiveness(Pattern **patterns, size_t count, void *type)
{
    (void)patterns; (void)count; (void)type;
    return true;
}

// Stub: Find missing patterns
Pattern **find_missing_patterns(Pattern **patterns, size_t count, void *type, size_t *out_count)
{
    (void)patterns; (void)count; (void)type;
    *out_count = 0;
    return NULL;
}

// Stub: Check if pattern is reachable
bool is_pattern_reachable(Pattern *pattern, Pattern **previous, size_t count)
{
    (void)pattern; (void)previous; (void)count;
    return true;
}
