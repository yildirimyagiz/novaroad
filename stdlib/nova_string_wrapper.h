/**
 * Nova String Wrapper Functions
 * Provides String_len and String_from for Nova compiler compatibility
 */

#ifndef NOVA_STRING_WRAPPER_H
#define NOVA_STRING_WRAPPER_H

#include "nova_string.h"

// Wrapper functions for Nova compiler
size_t String_len(const NovaString *str);
NovaString *String_from(const char *cstr);
NovaString *String_from_literal(const char *cstr);

// Type alias for convenience
typedef NovaString String;

#endif // NOVA_STRING_WRAPPER_H
