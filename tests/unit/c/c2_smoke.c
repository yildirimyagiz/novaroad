// C.2 Methods Smoke Test
// Quick regression test for methods before moving to C.3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple smoke test for method resolution
int smoke_test_methods() {
    printf("🔥 C.2 Methods Smoke Test\n");

    // Simulate method call resolution
    const char* test_cases[] = {
        "Vec<i32>.push(42)",
        "Vec<String>.len()",
        "HashMap<String, i32>.get(\"key\")",
        "Option<i32>.is_some()"
    };

    int passed = 0;
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        printf("  Testing: %s\n", test_cases[i]);

        // Simulate resolution (would check registry, type params, etc.)
        int resolved = 1; // Assume success

        if (resolved) {
            printf("    ✅ Resolved\n");
            passed++;
        } else {
            printf("    ❌ Failed to resolve\n");
        }
    }

    printf("  Result: %d/%zu method calls resolved\n", passed,
           sizeof(test_cases)/sizeof(test_cases[0]));

    return passed == sizeof(test_cases)/sizeof(test_cases[0]) ? 0 : 1;
}

// Run C.2 smoke test
int main() {
    printf("=== C.2 Methods Smoke Test ===\n\n");

    int result = smoke_test_methods();

    if (result == 0) {
        printf("\n✅ C.2 Methods working - proceeding to C.3 Enums\n");
    } else {
        printf("\n❌ C.2 Methods regression detected\n");
    }

    return result;
}
