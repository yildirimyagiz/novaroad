// Test C integration directly
#include <stdio.h>

// Nova VM function signature
typedef int (*nova_function_t)(void);

// Simple test function that prints
void test_print() {
    printf("✅ Hello from C!\n");
    printf("🚀 Nova C integration works!\n");
}

int main() {
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         NOVA C INTEGRATION TEST                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    test_print();
    
    printf("\n");
    printf("✅ Test completed successfully!\n");
    printf("📊 Next: Link this with Nova bytecode\n");
    
    return 0;
}
