/**
 * Nova String Wrapper Functions
 * Provides String_len and String_from for Nova compiler compatibility
 */

#include "nova_string.h"
#include <stdlib.h>
#include <string.h>

// Wrapper for String length - returns size_t as usize
size_t String_len(const NovaString *str) {
    return nova_string_len(str);
}

// Wrapper for String creation from C string literal
NovaString *String_from(const char *cstr) {
    return nova_string_from_cstr(cstr);
}

// Wrapper for String creation from literal (alias for String_from)
NovaString *String_from_literal(const char *cstr) {
    return nova_string_from_cstr(cstr);
}
