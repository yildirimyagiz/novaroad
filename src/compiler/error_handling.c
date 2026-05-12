/**
 * @file error_handling.c
 * @brief Error handling implementation (STUB)
 */

#include "nova_error.h"
#include <stdlib.h>
#include <stdbool.h>

// Stub: Create error
Error *error_create(const char *message, int line, int col)
{
    (void)message; (void)line; (void)col;
    return NULL;
}

// Stub: Free error
void error_free(Error *err)
{
    (void)err;
}

// Stub: Print error
void error_print(Error *err)
{
    (void)err;
}

// Stub: Check if type is Result
bool is_result_type(void *ty)
{
    (void)ty;
    return false;
}

// Stub: Check if type is Option
bool is_option_type(void *ty)
{
    (void)ty;
    return false;
}
