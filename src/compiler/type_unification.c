/**
 * @file type_unification.c
 * @brief Type unification for generics
 */

#include <compiler/ast.h>
#include <compiler/dimensions.h>
#include <compiler/module_registry.h>
#include <compiler/physics_constants.h>
#include <compiler/semantic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// TYPE SUBSTITUTION MAP
// ══════════════════════════════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════════════════════════════
// TYPE SUBSTITUTION MAP
// ══════════════════════════════════════════════════════════════════════════════

typedef struct {
    char *type_var;             // "T", "E", "K", "V"
    nova_type_t *concrete_type; // i64, String, etc.
} TypeSubstitution;

typedef struct {
    TypeSubstitution *substitutions;
    size_t count;
    size_t capacity;
} SubstitutionMap;

SubstitutionMap *subst_map_new(void)
{
    SubstitutionMap *map = malloc(sizeof(SubstitutionMap));
    map->capacity = 8;
    map->count = 0;
    map->substitutions = malloc(sizeof(TypeSubstitution) * map->capacity);
    return map;
}

void subst_map_add(SubstitutionMap *map, const char *var, nova_type_t *type)
{
    // Check if already exists
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->substitutions[i].type_var, var) == 0) {
            // Update existing
            map->substitutions[i].concrete_type = type;
            return;
        }
    }

    // Add new
    if (map->count >= map->capacity) {
        map->capacity *= 2;
        map->substitutions = realloc(map->substitutions, sizeof(TypeSubstitution) * map->capacity);
    }

    map->substitutions[map->count].type_var = strdup(var);
    map->substitutions[map->count].concrete_type = type;
    map->count++;
}

nova_type_t *subst_map_get(SubstitutionMap *map, const char *var)
{
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->substitutions[i].type_var, var) == 0) {
            return map->substitutions[i].concrete_type;
        }
    }
    return NULL;
}

void subst_map_free(SubstitutionMap *map)
{
    if (!map)
        return;
    for (size_t i = 0; i < map->count; i++) {
        free(map->substitutions[i].type_var);
    }
    free(map->substitutions);
    free(map);
}

// ══════════════════════════════════════════════════════════════════════════════
// TYPE UNIFICATION
// ══════════════════════════════════════════════════════════════════════════════

bool types_equal(nova_type_t *a, nova_type_t *b);

/**
 * Unify two types
 */
bool unify_types(nova_type_t *t1, nova_type_t *t2, SubstitutionMap *subst)
{
    // Case 1: t1 is a type variable
    if (t1->kind == TYPE_VAR) {
        nova_type_t *existing = subst_map_get(subst, t1->data.var_name);
        if (existing) {
            return types_equal(existing, t2);
        } else {
            subst_map_add(subst, t1->data.var_name, t2);
            return true;
        }
    }

    // Case 2: t2 is a type variable
    if (t2->kind == TYPE_VAR) {
        nova_type_t *existing = subst_map_get(subst, t2->data.var_name);
        if (existing) {
            return types_equal(existing, t1);
        } else {
            subst_map_add(subst, t2->data.var_name, t1);
            return true;
        }
    }

    // Case 3: Both are concrete types
    if (t1->kind != t2->kind) {
        return false; // Mismatch
    }

    switch (t1->kind) {
    case TYPE_I32:
    case TYPE_I64:
    case TYPE_F32:
    case TYPE_F64:
    case TYPE_STR:
    case TYPE_BOOL:
        return true; // Primitive types of same kind are equal

    case TYPE_DATA:
        if (!t1->data.name || !t2->data.name)
            return t1->data.name == t2->data.name;
        return strcmp(t1->data.name, t2->data.name) == 0;

    case TYPE_GENERIC: {
        if (strcmp(t1->data.generic.name, t2->data.generic.name) != 0) {
            return false;
        }

        if (t1->data.generic.arg_count != t2->data.generic.arg_count) {
            return false;
        }

        for (size_t i = 0; i < t1->data.generic.arg_count; i++) {
            if (!unify_types(t1->data.generic.args[i], t2->data.generic.args[i], subst)) {
                return false;
            }
        }

        return true;
    }

    case TYPE_TUPLE: {
        if (t1->data.tuple.count != t2->data.tuple.count) {
            return false;
        }

        for (size_t i = 0; i < t1->data.tuple.count; i++) {
            if (!unify_types(t1->data.tuple.elements[i], t2->data.tuple.elements[i], subst)) {
                return false;
            }
        }

        return true;
    }

    case TYPE_FN: {
        if (t1->data.fn.param_count != t2->data.fn.param_count) {
            return false;
        }

        for (size_t i = 0; i < t1->data.fn.param_count; i++) {
            if (!unify_types(t1->data.fn.params[i], t2->data.fn.params[i], subst)) {
                return false;
            }
        }

        return unify_types(t1->data.fn.return_type, t2->data.fn.return_type, subst);
    }

    default:
        return false;
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// TYPE SUBSTITUTION
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Apply substitution to a type
 */
nova_type_t *substitute_type(nova_type_t *type, SubstitutionMap *subst)
{
    if (!type)
        return NULL;

    if (type->kind == TYPE_VAR) {
        nova_type_t *concrete = subst_map_get(subst, type->data.var_name);
        if (concrete) {
            return concrete;
        }
        return type;
    }

    if (type->kind == TYPE_GENERIC) {
        nova_type_t *result = malloc(sizeof(nova_type_t));
        result->kind = TYPE_GENERIC;
        result->data.generic.name = strdup(type->data.generic.name);
        result->data.generic.arg_count = type->data.generic.arg_count;
        result->data.generic.args = malloc(sizeof(nova_type_t *) * type->data.generic.arg_count);

        for (size_t i = 0; i < type->data.generic.arg_count; i++) {
            result->data.generic.args[i] = substitute_type(type->data.generic.args[i], subst);
        }

        return result;
    }

    if (type->kind == TYPE_TUPLE) {
        nova_type_t *result = malloc(sizeof(nova_type_t));
        result->kind = TYPE_TUPLE;
        result->data.tuple.count = type->data.tuple.count;
        result->data.tuple.elements = malloc(sizeof(nova_type_t *) * type->data.tuple.count);

        for (size_t i = 0; i < type->data.tuple.count; i++) {
            result->data.tuple.elements[i] = substitute_type(type->data.tuple.elements[i], subst);
        }

        return result;
    }

    if (type->kind == TYPE_FN) {
        nova_type_t *result = malloc(sizeof(nova_type_t));
        result->kind = TYPE_FN;
        result->data.fn.param_count = type->data.fn.param_count;
        result->data.fn.params = malloc(sizeof(nova_type_t *) * type->data.fn.param_count);

        for (size_t i = 0; i < type->data.fn.param_count; i++) {
            result->data.fn.params[i] = substitute_type(type->data.fn.params[i], subst);
        }

        result->data.fn.return_type = substitute_type(type->data.fn.return_type, subst);

        return result;
    }

    return type;
}

// ══════════════════════════════════════════════════════════════════════════════
// TYPE INFERENCE
// ══════════════════════════════════════════════════════════════════════════════

bool infer_generic_types(nova_type_t **param_types, size_t param_count, nova_type_t **arg_types,
                         size_t arg_count, SubstitutionMap *subst)
{
    if (param_count != arg_count) {
        return false;
    }

    for (size_t i = 0; i < param_count; i++) {
        if (!unify_types(param_types[i], arg_types[i], subst)) {
            return false;
        }
    }

    return true;
}

bool extract_concrete_types(char **type_param_names, size_t count, SubstitutionMap *subst,
                            nova_type_t **concrete_types)
{
    for (size_t i = 0; i < count; i++) {
        nova_type_t *concrete = subst_map_get(subst, type_param_names[i]);
        if (!concrete) {
            return false;
        }
        concrete_types[i] = concrete;
    }

    return true;
}
