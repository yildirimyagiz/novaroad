#include <stdbool.h>

// Temporary stubs for missing functions

#include <stddef.h>

// Formal verification stubs
void *formal_verification_create(void) {
    return NULL;
}

void formal_verification_destroy(void *ctx) {
    (void)ctx;
}

int formal_verify_program(void *ctx, void *program) {
    (void)ctx;
    (void)program;
    return 0; // Success
}

// Borrow checker stub
const char *nova_borrow_checker_get_error(void *checker) {
    (void)checker;
    return "Borrow checker not implemented";
}
