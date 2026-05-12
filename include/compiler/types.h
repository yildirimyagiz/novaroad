/**
 * @file types.h
 * @brief Type system (inference, checking, borrow checking)
 */

#ifndef NOVA_COMPILER_TYPES_H
#define NOVA_COMPILER_TYPES_H


#include "ast.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Types are defined in ast.h (included above)
nova_type_t *nova_type_primitive(nova_type_kind_t kind);
bool nova_type_equals(nova_type_t *a, nova_type_t *b);
const char *nova_type_to_string(nova_type_t *type);

/* Type checking */
int nova_type_check(struct nova_stmt *ast);
nova_type_t *nova_type_infer(struct nova_expr *expr);

/* Borrow checking */
int nova_borrow_check(struct nova_stmt *ast);

/* Nova-specific type constructors */

/**
 * Create flow type: flow<T>
 * @param element_type Type of elements in the flow
 * @return Flow type
 */
nova_type_t *nova_type_flow(nova_flow_kind_t flow_kind, nova_type_t *inner);

/**
 * Create tensor type with shape: tensor<[batch, height, width, channels], f32>
 * @param shape_names Named dimensions (can be NULL)
 * @param shape_sizes Dimension sizes
 * @param num_dims Number of dimensions
 * @param dtype Data type (f32, f64, i32, etc.)
 * @return Tensor type
 */
nova_type_t *nova_type_tensor(const char **shape_names, size_t *shape_sizes, size_t num_dims,
                              nova_type_t *dtype);

/**
 * Create dimensional type with units: 5.kg, 3.m/s²
 * @param base_type Numeric base type
 * @param unit_string Unit specification ("kg", "m/s²", etc.)
 * @return Dimensional type
 */
nova_type_t *nova_type_dimensional(nova_type_t *base_type, const char *unit_string);

/**
 * Create contract type with pre/post conditions
 * @param base_type Underlying type
 * @param requires Precondition expressions
 * @param ensures Postcondition expressions
 * @param num_requires Number of preconditions
 * @param num_ensures Number of postconditions
 * @return Contract type
 */
nova_type_t *nova_type_contract(nova_type_t *base_type, struct nova_expr **requires,
                                size_t num_requires, struct nova_expr **ensures,
                                size_t num_ensures);

/**
 * Create effect type: func() -> i32 with [io, async]
 * @param base_type Function type
 * @param effects Effect annotations
 * @param num_effects Number of effects
 * @return Effect type
 */
nova_type_t *nova_type_with_effects(nova_type_t *base_type, const char **effects,
                                    size_t num_effects);

/**
 * Create union type: A | B
 * @param types Array of types to union
 * @param num_types Number of types
 * @return Union type
 */
nova_type_t *nova_type_union(nova_type_t **types, size_t num_types);

/**
 * Create intersection type: A & B
 * @param types Array of types to intersect
 * @param num_types Number of types
 * @return Intersection type
 */
nova_type_t *nova_type_intersection(nova_type_t **types, size_t num_types);

/**
 * Create space type: space<3D, physics>
 * @param dimension_spec Dimension specification ("2D", "3D", "4D")
 * @param domain Domain specification ("physics", "graphics", etc.)
 * @return Space type
 */
nova_type_t *nova_type_space(const char *dimension_spec, const char *domain);

/* Type checking for Nova-specific features */

/**
 * Check dimensional compatibility (can't add kg + meter)
 */
bool nova_type_dimensional_compatible(nova_type_t *a, nova_type_t *b);

/**
 * Infer result type of dimensional operation
 * Example: kg * m/s² = Newton (N)
 */
nova_type_t *nova_type_dimensional_result(nova_type_t *a, nova_type_t *b, const char *op);

/**
 * Check tensor shape compatibility for operations
 */
bool nova_type_tensor_shape_compatible(nova_type_t *a, nova_type_t *b, const char *op);

/**
 * Verify contract conditions are satisfiable
 */
bool nova_type_contract_satisfiable(nova_type_t *contract_type);

/**
 * Check if type has specific effect
 */
bool nova_type_has_effect(nova_type_t *type, const char *effect);

#ifdef __cplusplus
}
#endif
#endif /* NOVA_TYPES_H */
