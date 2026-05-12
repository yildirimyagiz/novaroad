/**
 * @file test_generics_backend.c
 * @brief Generics backend tests
 */

#include "compiler/generics.h"
#include "compiler/types.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  ✓ PASS\n")

// Test 1: Type parameter creation
void test_type_parameter() {
    TEST("Type parameter creation");
    
    TypeParameter *param = type_parameter_new("T");
    assert(param != NULL);
    assert(strcmp(param->name, "T") == 0);
    assert(param->bound_count == 0);
    
    type_parameter_free(param);
    PASS();
}

// Test 2: Name mangling
void test_name_mangling() {
    TEST("Name mangling");
    
    Type *types[] = {type_prim(TYPE_I64)};
    char *mangled = mangle_generic_name("max", types, 1);
    
    assert(strcmp(mangled, "max_i64") == 0);
    
    free(mangled);
    PASS();
}

// Test 3: Multiple type parameters
void test_multiple_type_params() {
    TEST("Multiple type parameter mangling");
    
    Type *types[] = {
        type_prim(TYPE_I64),
        type_prim(TYPE_STRING),
    };
    char *mangled = mangle_generic_name("map", types, 2);
    
    assert(strcmp(mangled, "map_i64_String") == 0);
    
    free(mangled);
    PASS();
}

// Test 4: Generic registry
void test_generic_registry() {
    TEST("Generic registry");
    
    GenericRegistry *reg = generic_registry_new();
    assert(reg != NULL);
    assert(reg->count == 0);
    
    Type *types[] = {type_prim(TYPE_I64)};
    GenericInstance *inst = generic_instance_new("identity", types, 1);
    
    generic_registry_add(reg, inst);
    assert(reg->count == 1);
    
    generic_registry_free(reg);
    PASS();
}

// Test 5: Find existing instance
void test_find_instance() {
    TEST("Find existing instance");
    
    GenericRegistry *reg = generic_registry_new();
    
    Type *types1[] = {type_prim(TYPE_I64)};
    GenericInstance *inst1 = generic_instance_new("max", types1, 1);
    generic_registry_add(reg, inst1);
    
    // Find it
    GenericInstance *found = generic_registry_find(reg, "max", types1, 1);
    assert(found == inst1);
    
    // Different type shouldn't match
    Type *types2[] = {type_prim(TYPE_F64)};
    GenericInstance *not_found = generic_registry_find(reg, "max", types2, 1);
    assert(not_found == NULL);
    
    generic_registry_free(reg);
    PASS();
}

// Test 6: Type unification - simple
void test_unify_simple() {
    TEST("Type unification - simple");
    
    SubstitutionMap *subst = subst_map_new();
    
    Type *var_t = type_var("T");
    Type *i64_ty = type_prim(TYPE_I64);
    
    bool result = unify_types(var_t, i64_ty, subst);
    assert(result == true);
    
    // Check substitution
    Type *concrete = subst_map_get(subst, "T");
    assert(concrete != NULL);
    assert(types_equal(concrete, i64_ty));
    
    subst_map_free(subst);
    PASS();
}

// Test 7: Type unification - generic
void test_unify_generic() {
    TEST("Type unification - generic types");
    
    SubstitutionMap *subst = subst_map_new();
    
    // Vec<T> vs Vec<i64>
    Type *var_t = type_var("T");
    Type *vec_t = type_generic("Vec", &var_t, 1);
    
    Type *i64_ty = type_prim(TYPE_I64);
    Type *vec_i64 = type_generic("Vec", &i64_ty, 1);
    
    bool result = unify_types(vec_t, vec_i64, subst);
    assert(result == true);
    
    // T should be bound to i64
    Type *concrete = subst_map_get(subst, "T");
    assert(concrete != NULL);
    assert(types_equal(concrete, i64_ty));
    
    subst_map_free(subst);
    PASS();
}

// Test 8: Type substitution
void test_type_substitution() {
    TEST("Type substitution");
    
    SubstitutionMap *subst = subst_map_new();
    subst_map_add(subst, "T", type_prim(TYPE_I64));
    
    // Substitute T → i64 in Vec<T>
    Type *var_t = type_var("T");
    Type *vec_t = type_generic("Vec", &var_t, 1);
    
    Type *result = substitute_type(vec_t, subst);
    
    // Result should be Vec<i64>
    assert(result->kind == TYPE_GENERIC);
    assert(strcmp(result->data.generic.name, "Vec") == 0);
    assert(result->data.generic.args[0]->kind == TYPE_PRIM);
    
    subst_map_free(subst);
    PASS();
}

// Test 9: Inference from call
void test_infer_from_call() {
    TEST("Infer types from function call");
    
    // fn identity<T>(x: T) -> T
    Type *param_types[] = {type_var("T")};
    
    // identity(42) → T = i64
    Type *arg_types[] = {type_prim(TYPE_I64)};
    
    SubstitutionMap *subst = subst_map_new();
    bool result = infer_generic_types(param_types, 1, arg_types, 1, subst);
    
    assert(result == true);
    
    Type *inferred = subst_map_get(subst, "T");
    assert(inferred != NULL);
    assert(types_equal(inferred, type_prim(TYPE_I64)));
    
    subst_map_free(subst);
    PASS();
}

// Test 10: Result<T, E> inference
void test_result_inference() {
    TEST("Result<T, E> type inference");
    
    SubstitutionMap *subst = subst_map_new();
    
    // Result<T, E>
    Type *var_t = type_var("T");
    Type *var_e = type_var("E");
    Type *type_args1[] = {var_t, var_e};
    Type *result_te = type_generic("Result", type_args1, 2);
    
    // Result<i64, String>
    Type *i64_ty = type_prim(TYPE_I64);
    Type *str_ty = type_prim(TYPE_STRING);
    Type *type_args2[] = {i64_ty, str_ty};
    Type *result_concrete = type_generic("Result", type_args2, 2);
    
    bool success = unify_types(result_te, result_concrete, subst);
    assert(success == true);
    
    // Check T = i64, E = String
    Type *t_concrete = subst_map_get(subst, "T");
    Type *e_concrete = subst_map_get(subst, "E");
    
    assert(types_equal(t_concrete, i64_ty));
    assert(types_equal(e_concrete, str_ty));
    
    subst_map_free(subst);
    PASS();
}

// Main test runner
int main() {
    printf("═══════════════════════════════════════════════\n");
    printf("  Generics Backend Tests\n");
    printf("═══════════════════════════════════════════════\n\n");
    
    test_type_parameter();
    test_name_mangling();
    test_multiple_type_params();
    test_generic_registry();
    test_find_instance();
    test_unify_simple();
    test_unify_generic();
    test_type_substitution();
    test_infer_from_call();
    test_result_inference();
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  ✓ All generics tests passed!\n");
    printf("═══════════════════════════════════════════════\n");
    
    return 0;
}
