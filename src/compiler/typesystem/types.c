/**
 * @file types.c
 * @brief Type system implementation for Nova compiler
 */

#include "compiler/types.h"
#include "compiler/ast.h"
#include "std/alloc.h"
#include <string.h>

/* Use nova_alloc from ast.c if available, or malloc */
static void *types_malloc(size_t size)
{
    void *ptr = nova_alloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

nova_type_t *nova_type_primitive(nova_type_kind_t kind)
{
    nova_type_t *type = types_malloc(sizeof(nova_type_t));
    if (!type)
        return NULL;

    type->kind = kind;
    return type;
}

nova_type_t *nova_type_void(void)
{
    return nova_type_primitive(TYPE_VOID);
}
nova_type_t *nova_type_bool(void)
{
    return nova_type_primitive(TYPE_BOOL);
}
nova_type_t *nova_type_i32(void)
{
    return nova_type_primitive(TYPE_I32);
}
nova_type_t *nova_type_i64(void)
{
    return nova_type_primitive(TYPE_I64);
}
nova_type_t *nova_type_str(void)
{
    return nova_type_primitive(TYPE_STR);
}

nova_type_t *nova_type_array(nova_type_t *element_type, size_t size)
{
    nova_type_t *type = types_malloc(sizeof(nova_type_t));
    if (!type)
        return NULL;

    type->kind = TYPE_ARRAY;
    type->data.ptr.pointee = element_type;
    type->data.ptr.size = size;

    return type;
}

nova_type_t *nova_type_flow(nova_flow_kind_t flow_kind, nova_type_t *inner)
{
    nova_type_t *type = types_malloc(sizeof(nova_type_t));
    if (!type)
        return NULL;

    type->kind = TYPE_FLOW;
    type->data.flow.flow_kind = (int) flow_kind;
    type->data.flow.inner = inner;

    return type;
}

nova_type_t *nova_type_pointer(nova_type_t *pointee)
{
    nova_type_t *type = types_malloc(sizeof(nova_type_t));
    if (!type)
        return NULL;

    type->kind = TYPE_POINTER;
    type->data.ptr.pointee = pointee;

    return type;
}

nova_type_t *nova_type_function(nova_type_t **params, size_t param_count, nova_type_t *return_type)
{
    nova_type_t *type = types_malloc(sizeof(nova_type_t));
    if (!type)
        return NULL;

    type->kind = TYPE_FN;
    type->data.fn.params = params;
    type->data.fn.param_count = param_count;
    type->data.fn.return_type = return_type;

    return type;
}

nova_type_t *nova_type_contract(nova_type_t *base_type, struct nova_expr **requires,
                                size_t num_requires, struct nova_expr **ensures, size_t num_ensures)
{
    nova_type_t *type = types_malloc(sizeof(nova_type_t));
    if (!type)
        return NULL;

    type->kind = TYPE_CONTRACT;
    type->data.contract.base_type = base_type;
    type->data.contract.
        requires
    =
        requires;
    type->data.contract.num_requires = num_requires;
    type->data.contract.ensures = ensures;
    type->data.contract.num_ensures = num_ensures;

    return type;
}

bool nova_type_equals(nova_type_t *a, nova_type_t *b)
{
    if (!a || !b)
        return a == b;
    if (a->kind != b->kind)
        return false;

    switch (a->kind) {
    case TYPE_DATA:
        if (!a->data.name || !b->data.name)
            return a->data.name == b->data.name;
        return strcmp(a->data.name, b->data.name) == 0;

    case TYPE_PTR:
    case TYPE_PTR_MUT:
    case TYPE_POINTER:
    case TYPE_ARRAY:
        return nova_type_equals(a->data.ptr.pointee, b->data.ptr.pointee);

    case TYPE_FLOW:
        if (a->data.flow.flow_kind != b->data.flow.flow_kind)
            return false;
        return nova_type_equals(a->data.flow.inner, b->data.flow.inner);

    case TYPE_CONTRACT:
        if (!nova_type_equals(a->data.contract.base_type, b->data.contract.base_type))
            return false;
        // For simplicity, we're not comparing requires/ensures expressions here.
        // A full comparison would involve deep equality checks on AST nodes.
        return true;

    case TYPE_FN:
        if (a->data.fn.param_count != b->data.fn.param_count)
            return false;
        for (size_t i = 0; i < a->data.fn.param_count; i++) {
            if (!nova_type_equals(a->data.fn.params[i], b->data.fn.params[i]))
                return false;
        }
        return nova_type_equals(a->data.fn.return_type, b->data.fn.return_type);

    default:
        return true;
    }
}

int nova_type_check(struct nova_stmt *stmt)
{
    if (!stmt)
        return 0;

    /* Internal type checking logic is normally in type_checker.c (semantic.c)
     * This is a placeholder or entry point for simple checks. */
    return 0;
}

nova_type_t *nova_type_infer(struct nova_expr *expr)
{
    if (!expr)
        return NULL;
    return expr->type;
}

int nova_borrow_check(struct nova_stmt *stmt)
{
    if (!stmt)
        return 0;
    return 0;
}

/* TODO: Implement remaining functions from types.h */
bool nova_type_dimensional_compatible(nova_type_t *a, nova_type_t *b)
{
    return false;
}
bool nova_type_contract_satisfiable(nova_type_t *contract_type)
{
    return true;
}
bool nova_type_has_effect(nova_type_t *type, const char *effect)
{
    return false;
}
const char *nova_type_to_string(nova_type_t *type)
{
    return "type";
}
