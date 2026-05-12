/**
 * @file error_semantic.c
 * @brief Error semantic analysis (STUB)
 */

#include "nova_error.h"
#include <stdbool.h>

// Stub: Check error propagation
bool check_error_propagation(void *expr, void *ctx)
{
    (void)expr; (void)ctx;
    return true;
}

// Stub: Check try/catch blocks
bool check_try_catch(void *stmt, void *ctx)
{
    (void)stmt; (void)ctx;
    return true;
}

// Stub: Infer error type
void *infer_error_type(void *expr, void *ctx)
{
    (void)expr; (void)ctx;
    return NULL;
}
