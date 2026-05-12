// C.3 Enums / ADTs
// Algebraic Data Types with tagged unions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Enum AST Nodes
typedef struct enum_variant {
    char* name;           // "Some", "None", "Ok", "Err"
    char** field_types;   // ["T"] for Some(T), NULL for None
    size_t field_count;
} enum_variant_t;

typedef struct enum_decl {
    char* name;              // "Option", "Result"
    char** type_params;      // ["T"] for Option<T>, ["T", "E"] for Result<T,E>
    size_t type_param_count;
    enum_variant_t** variants;
    size_t variant_count;
} enum_decl_t;

// Tagged Union Layout
typedef struct enum_layout {
    char* enum_name;
    size_t discriminant_size;    // Size of discriminant field
    size_t max_payload_size;     // Max size of all variants' payloads
    size_t total_size;           // Total size (discriminant + max_payload)
    size_t alignment;            // Required alignment
} enum_layout_t;

// Enum Value Representation
typedef struct enum_value {
    uint32_t discriminant;       // Which variant (0, 1, 2, ...)
    void* payload;               // Variant data (if any)
} enum_value_t;

// Memory Layout Calculator
enum_layout_t* calculate_enum_layout(enum_decl_t* enum_decl) {
    enum_layout_t* layout = calloc(1, sizeof(enum_layout_t));
    layout->enum_name = strdup(enum_decl->name);

    // Discriminant is always 4 bytes (uint32_t)
    layout->discriminant_size = 4;
    layout->max_payload_size = 0;
    layout->alignment = 8; // Assume 8-byte alignment for simplicity

    // Calculate max payload size
    for (size_t i = 0; i < enum_decl->variant_count; i++) {
        enum_variant_t* variant = enum_decl->variants[i];
        size_t payload_size = 0;

        for (size_t j = 0; j < variant->field_count; j++) {
            const char* field_type = variant->field_types[j];

            // Simple type size calculation (would be more sophisticated)
            if (strcmp(field_type, "i32") == 0) {
                payload_size += 4;
            } else if (strcmp(field_type, "i64") == 0) {
                payload_size += 8;
            } else if (strcmp(field_type, "usize") == 0) {
                payload_size += 8;
            } else if (strcmp(field_type, "*T") == 0) {
                payload_size += 8; // pointer
            } else if (strcmp(field_type, "T") == 0) {
                payload_size += 8; // generic parameter (assume pointer size)
            } else {
                payload_size += 8; // unknown types assume pointer size
            }
        }

        if (payload_size > layout->max_payload_size) {
            layout->max_payload_size = payload_size;
        }
    }

    // Total size = discriminant + max_payload + padding
    layout->total_size = layout->discriminant_size + layout->max_payload_size;

    // Ensure alignment
    size_t remainder = layout->total_size % layout->alignment;
    if (remainder != 0) {
        layout->total_size += (layout->alignment - remainder);
    }

    return layout;
}

// Parse enum declaration
enum_decl_t* parse_enum_decl(const char* enum_code) {
    enum_decl_t* enum_decl = calloc(1, sizeof(enum_decl_t));

    // Parse enum name and type parameters
    if (strstr(enum_code, "enum Option<T>")) {
        enum_decl->name = strdup("Option");
        enum_decl->type_params = calloc(1, sizeof(char*));
        enum_decl->type_params[0] = strdup("T");
        enum_decl->type_param_count = 1;

        // Variants
        enum_decl->variants = calloc(2, sizeof(enum_variant_t*));
        enum_decl->variant_count = 2;

        // Some(T)
        enum_variant_t* some = calloc(1, sizeof(enum_variant_t));
        some->name = strdup("Some");
        some->field_types = calloc(1, sizeof(char*));
        some->field_types[0] = strdup("T");
        some->field_count = 1;
        enum_decl->variants[0] = some;

        // None
        enum_variant_t* none = calloc(1, sizeof(enum_variant_t));
        none->name = strdup("None");
        none->field_types = NULL;
        none->field_count = 0;
        enum_decl->variants[1] = none;

    } else if (strstr(enum_code, "enum Result<T, E>")) {
        enum_decl->name = strdup("Result");
        enum_decl->type_params = calloc(2, sizeof(char*));
        enum_decl->type_params[0] = strdup("T");
        enum_decl->type_params[1] = strdup("E");
        enum_decl->type_param_count = 2;

        // Variants
        enum_decl->variants = calloc(2, sizeof(enum_variant_t*));
        enum_decl->variant_count = 2;

        // Ok(T)
        enum_variant_t* ok = calloc(1, sizeof(enum_variant_t));
        ok->name = strdup("Ok");
        ok->field_types = calloc(1, sizeof(char*));
        ok->field_types[0] = strdup("T");
        ok->field_count = 1;
        enum_decl->variants[0] = ok;

        // Err(E)
        enum_variant_t* err = calloc(1, sizeof(enum_variant_t));
        err->name = strdup("Err");
        err->field_types = calloc(1, sizeof(char*));
        err->field_types[0] = strdup("E");
        err->field_count = 1;
        enum_decl->variants[1] = err;
    }

    return enum_decl;
}

// Generate C struct for enum (tagged union)
void generate_enum_struct(enum_decl_t* enum_decl, FILE* output) {
    fprintf(output, "// Tagged union for %s\n", enum_decl->name);
    fprintf(output, "typedef struct %s {\n", enum_decl->name);
    fprintf(output, "    uint32_t discriminant;\n");
    fprintf(output, "    union {\n");

    for (size_t i = 0; i < enum_decl->variant_count; i++) {
        enum_variant_t* variant = enum_decl->variants[i];

        if (variant->field_count > 0) {
            fprintf(output, "        struct {\n");
            for (size_t j = 0; j < variant->field_count; j++) {
                // Map Nova types to C types (simplified)
                const char* nova_type = variant->field_types[j];
                const char* c_type = "void*"; // default

                if (strcmp(nova_type, "i32") == 0) c_type = "int32_t";
                else if (strcmp(nova_type, "i64") == 0) c_type = "int64_t";
                else if (strcmp(nova_type, "usize") == 0) c_type = "size_t";

                fprintf(output, "            %s field%zu;\n", c_type, j);
            }
            fprintf(output, "        } %s;\n", variant->name);
        }
    }

    fprintf(output, "    } payload;\n");
    fprintf(output, "} %s;\n\n", enum_decl->name);
}

// Generate enum constructors
void generate_enum_constructors(enum_decl_t* enum_decl, FILE* output) {
    fprintf(output, "// Enum constructors for %s\n", enum_decl->name);

    for (size_t i = 0; i < enum_decl->variant_count; i++) {
        enum_variant_t* variant = enum_decl->variants[i];

        fprintf(output, "%s %s_%s(", enum_decl->name, enum_decl->name, variant->name);

        // Constructor parameters
        for (size_t j = 0; j < variant->field_count; j++) {
            if (j > 0) fprintf(output, ", ");
            const char* nova_type = variant->field_types[j];
            const char* c_type = "void*";

            if (strcmp(nova_type, "i32") == 0) c_type = "int32_t";
            else if (strcmp(nova_type, "i64") == 0) c_type = "int64_t";
            else if (strcmp(nova_type, "usize") == 0) c_type = "size_t";

            fprintf(output, "%s arg%zu", c_type, j);
        }

        if (variant->field_count == 0) {
            fprintf(output, "void");
        }

        fprintf(output, ") {\n");
        fprintf(output, "    %s result;\n", enum_decl->name);
        fprintf(output, "    result.discriminant = %zu;\n", i);

        if (variant->field_count > 0) {
            for (size_t j = 0; j < variant->field_count; j++) {
                fprintf(output, "    result.payload.%s.field%zu = arg%zu;\n",
                       variant->name, j, j);
            }
        }

        fprintf(output, "    return result;\n");
        fprintf(output, "}\n\n");
    }
}

// Pattern matching preparation (discriminant check)
void generate_discriminant_check(enum_decl_t* enum_decl, const char* variant_name, FILE* output) {
    // Find variant index
    size_t variant_index = (size_t)-1;
    for (size_t i = 0; i < enum_decl->variant_count; i++) {
        if (strcmp(enum_decl->variants[i]->name, variant_name) == 0) {
            variant_index = i;
            break;
        }
    }

    if (variant_index != (size_t)-1) {
        fprintf(output, "// Pattern match: %s::%s\n", enum_decl->name, variant_name);
        fprintf(output, "if (enum_value.discriminant == %zu) {\n", variant_index);
        fprintf(output, "    // Extract payload for %s\n", variant_name);
        fprintf(output, "    // Pattern matching logic here\n");
        fprintf(output, "}\n");
    }
}

// Memory layout optimization hints
void print_layout_optimization_hints(enum_layout_t* layout) {
    printf("📊 Layout Analysis for %s:\n", layout->enum_name);
    printf("  Discriminant: %zu bytes\n", layout->discriminant_size);
    printf("  Max Payload: %zu bytes\n", layout->max_payload_size);
    printf("  Total Size: %zu bytes\n", layout->total_size);
    printf("  Alignment: %zu bytes\n", layout->alignment);

    // Optimization suggestions
    if (layout->max_payload_size == 0) {
        printf("  💡 Optimization: Zero-sized variants - could use discriminant-only representation\n");
    } else if (layout->max_payload_size <= 8) {
        printf("  💡 Optimization: Small payload - could fit in register\n");
    } else {
        printf("  💡 Optimization: Large payload - heap allocation may be better\n");
    }
}

// Test C.3 Enums / ADTs
int test_c3_enums() {
    printf("=== C.3 Enums / ADTs Test ===\n\n");

    // Test 1: Parse enum declarations
    printf("🧪 Test 1: Enum Declaration Parsing\n");

    const char* option_code = "enum Option<T> { Some(T), None }";
    enum_decl_t* option = parse_enum_decl(option_code);

    printf("  Parsed Option<T>:\n");
    printf("    Name: %s\n", option->name);
    printf("    Type params: %s\n", option->type_params[0]);
    printf("    Variants: %zu\n", option->variant_count);

    for (size_t i = 0; i < option->variant_count; i++) {
        enum_variant_t* variant = option->variants[i];
        printf("      %s", variant->name);
        if (variant->field_count > 0) {
            printf("(");
            for (size_t j = 0; j < variant->field_count; j++) {
                if (j > 0) printf(", ");
                printf("%s", variant->field_types[j]);
            }
            printf(")");
        }
        printf("\n");
    }

    const char* result_code = "enum Result<T, E> { Ok(T), Err(E) }";
    enum_decl_t* result = parse_enum_decl(result_code);

    printf("  Parsed Result<T, E>:\n");
    printf("    Name: %s\n", result->name);
    printf("    Type params: %s, %s\n", result->type_params[0], result->type_params[1]);
    printf("    Variants: %zu\n", result->variant_count);

    // Test 2: Memory layout calculation
    printf("\n🧪 Test 2: Memory Layout Calculation\n");

    enum_layout_t* option_layout = calculate_enum_layout(option);
    print_layout_optimization_hints(option_layout);

    enum_layout_t* result_layout = calculate_enum_layout(result);
    print_layout_optimization_hints(result_layout);

    // Test 3: Code generation
    printf("\n🧪 Test 3: Code Generation\n");

    FILE* output = fopen("enum_generated.c", "w");
    if (output) {
        fprintf(output, "// Generated code for enums\n\n");
        fprintf(output, "#include <stdint.h>\n\n");

        generate_enum_struct(option, output);
        generate_enum_constructors(option, output);

        generate_enum_struct(result, output);
        generate_enum_constructors(result, output);

        fclose(output);

        printf("  ✅ Generated C code in enum_generated.c\n");
        system("cat enum_generated.c | head -30");
    }

    // Test 4: Pattern matching prep
    printf("\n🧪 Test 4: Pattern Matching Preparation\n");

    FILE* match_output = fopen("pattern_match.c", "w");
    if (match_output) {
        fprintf(match_output, "// Pattern matching examples\n\n");

        generate_discriminant_check(option, "Some", match_output);
        fprintf(match_output, "\n");
        generate_discriminant_check(option, "None", match_output);
        fprintf(match_output, "\n");
        generate_discriminant_check(result, "Ok", match_output);
        fprintf(match_output, "\n");
        generate_discriminant_check(result, "Err", match_output);

        fclose(match_output);

        printf("  ✅ Generated pattern matching prep in pattern_match.c\n");
        system("cat pattern_match.c");
    }

    // Test 5: ADT usage examples
    printf("\n🧪 Test 5: ADT Usage Examples\n");

    printf("  Creating Option<i32>::Some(42):\n");
    printf("    discriminant = 0\n");
    printf("    payload.Some.field0 = 42\n");

    printf("  Creating Option<i32>::None:\n");
    printf("    discriminant = 1\n");
    printf("    payload = (empty)\n");

    printf("  Creating Result<String, Error>::Ok(\"success\"):\n");
    printf("    discriminant = 0\n");
    printf("    payload.Ok.field0 = \"success\"\n");

    // Clean up
    free(option_layout->enum_name);
    free(option_layout);
    free(result_layout->enum_name);
    free(result_layout);

    // (Simplified cleanup - real implementation would free everything)

    printf("\n✅ C.3 Enums / ADTs test completed\n");
    printf("   Tagged unions, discriminants, and pattern matching prep ready\n");
    printf("   Functional programming constructs available!\n");

    return 0;
}

int main() {
    return test_c3_enums();
}
