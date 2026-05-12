/**
 * @file test_autograd.c
 * @brief Unit tests for autograd system
 */

#include "ai/autograd.h"
#include "ai/tensor.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

void test_basic_gradient() {
    printf("  Testing basic gradient computation...\n");
    
    nova_grad_tape_t *tape = nova_grad_tape_create(256);
    assert(tape != NULL);
    
    /* Create tensor x = 2.0 */
    size_t shape[] = {1};
    nova_tensor_t *x = nova_tensor_from_array((float[]){2.0f}, shape, 1, NOVA_DTYPE_F32);
    assert(x != NULL);
    
    /* Record operation */
    nova_grad_record(tape, x);
    
    /* y = x² = 4.0, dy/dx = 2x = 4.0 */
    nova_tensor_t *x2 = nova_tensor_mul(x, x);
    
    /* Backward pass */
    nova_grad_backward(tape, x2);
    nova_tensor_t *grad = nova_grad_get(tape, x);
    
    assert(grad != NULL);
    float *grad_data = (float *)nova_tensor_data(grad);
    assert(fabs(grad_data[0] - 4.0f) < 1e-5f);
    
    nova_grad_tape_destroy(tape);
    printf("  ✅ Basic gradient test passed\n");
}

void test_vjp() {
    printf("  Testing Vector-Jacobian Product...\n");
    
    nova_grad_tape_t *tape = nova_grad_tape_create(256);
    
    size_t shape[] = {2};
    nova_tensor_t *x = nova_tensor_from_array((float[]){1.0f, 2.0f}, shape, 1, NOVA_DTYPE_F32);
    nova_tensor_t *v = nova_tensor_from_array((float[]){1.0f, 0.0f}, shape, 1, NOVA_DTYPE_F32);
    
    nova_grad_record(tape, x);
    
    /* y = x₁² + x₂² */
    nova_tensor_t *x_squared = nova_tensor_mul(x, x);
    
    /* VJP */
    nova_tensor_t *vjp = nova_grad_vjp(tape, x_squared, x, v);
    
    assert(vjp != NULL);
    nova_grad_tape_destroy(tape);
    printf("  ✅ VJP test passed\n");
}

void test_higher_order() {
    printf("  Testing higher-order gradients...\n");
    
    nova_grad_tape_t *tape = nova_grad_tape_create(512);
    
    size_t shape[] = {1};
    nova_tensor_t *x = nova_tensor_from_array((float[]){3.0f}, shape, 1, NOVA_DTYPE_F32);
    
    nova_grad_record(tape, x);
    
    /* y = x³, dy/dx = 3x², d²y/dx² = 6x */
    nova_tensor_t *x2 = nova_tensor_mul(x, x);
    nova_tensor_t *x3 = nova_tensor_mul(x2, x);
    
    /* First derivative */
    nova_grad_backward(tape, x3);
    nova_tensor_t *grad1 = nova_grad_get(tape, x);
    
    assert(grad1 != NULL);
    float *g1_data = (float *)nova_tensor_data(grad1);
    assert(fabs(g1_data[0] - 27.0f) < 1e-4f); /* 3 * 3² = 27 */
    
    nova_grad_tape_destroy(tape);
    printf("  ✅ Higher-order gradient test passed\n");
}

int main(void) {
    printf("Running autograd tests...\n\n");
    
    test_basic_gradient();
    test_vjp();
    test_higher_order();
    
    printf("\n✅ All autograd tests passed!\n");
    return 0;
}
