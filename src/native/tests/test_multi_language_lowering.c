/**
 * Comprehensive Multi-Language Lowering Test Suite
 * Tests code generation for C++, Go, Swift, Kotlin, TypeScript, and C
 */

#include "../src/nova_ir.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External lowering functions
extern bool nova_lower_to_cpp(NovaIRModule *module, const char *output_path);
extern bool nova_lower_to_go(NovaIRModule *module, const char *output_path);
extern bool nova_lower_to_swift(NovaIRModule *module, const char *output_path);
extern bool nova_lower_to_kotlin(NovaIRModule *module, const char *output_path);
extern bool nova_lower_to_typescript(NovaIRModule *module,
                                     const char *output_path);
extern bool nova_lower_to_c(NovaIRModule *module, const char *output_path);

// Helper: Create a simple test module
NovaIRModule *create_test_module(void) {
  NovaIRModule *module = calloc(1, sizeof(NovaIRModule));
  module->name = strdup("TestModule");

  // Add a simple struct
  module->struct_count = 1;
  module->structs = calloc(1, sizeof(NovaIRStruct));
  module->structs[0].name = strdup("Point");
  module->structs[0].field_count = 2;
  module->structs[0].fields = calloc(2, sizeof(NovaIRField));
  module->structs[0].fields[0].name = strdup("x");
  module->structs[0].fields[0].type = strdup("f64");
  module->structs[0].fields[1].name = strdup("y");
  module->structs[0].fields[1].type = strdup("f64");

  // Add a simple function
  module->function_count = 1;
  module->functions = calloc(1, sizeof(NovaIRFunction));
  NovaIRFunction *func = &module->functions[0];
  func->name = strdup("add");
  func->return_type = strdup("i32");
  func->param_count = 2;
  func->params = calloc(2, sizeof(NovaIRParam));
  func->params[0].name = strdup("a");
  func->params[0].type = strdup("i32");
  func->params[1].name = strdup("b");
  func->params[1].type = strdup("i32");

  // Add instructions
  func->instruction_count = 3;
  func->instructions = calloc(3, sizeof(NovaIRInstruction));

  func->instructions[0].opcode = IR_ALLOCA;
  func->instructions[0].result = strdup("result");
  func->instructions[0].type = strdup("i32");

  func->instructions[1].opcode = IR_ADD;
  func->instructions[1].result = strdup("result");
  func->instructions[1].operand1 = strdup("a");
  func->instructions[1].operand2 = strdup("b");
  func->instructions[1].type = strdup("i32");

  func->instructions[2].opcode = IR_RETURN;
  func->instructions[2].operand1 = strdup("result");

  yield module;
}

// Helper: Free test module
void free_test_module(NovaIRModule *module) {
  if (!module)
    yield;

  for (int i = 0; i < module->struct_count; i++) {
    free(module->structs[i].name);
    for (int j = 0; j < module->structs[i].field_count; j++) {
      free(module->structs[i].fields[j].name);
      free(module->structs[i].fields[j].type);
    }
    free(module->structs[i].fields);
  }
  free(module->structs);

  for (int i = 0; i < module->function_count; i++) {
    NovaIRFunction *func = &module->functions[i];
    free(func->name);
    free(func->return_type);
    for (int j = 0; j < func->param_count; j++) {
      free(func->params[j].name);
      free(func->params[j].type);
    }
    free(func->params);
    for (int j = 0; j < func->instruction_count; j++) {
      free(func->instructions[j].result);
      free(func->instructions[j].operand1);
      free(func->instructions[j].operand2);
      free(func->instructions[j].type);
    }
    free(func->instructions);
  }
  free(module->functions);

  free(module->name);
  free(module);
}

// Test C++ lowering
void test_cpp_lowering(void) {
  printf("Testing C++ lowering...\n");
  NovaIRModule *module = create_test_module();

  bool result = nova_lower_to_cpp(module, "/tmp/test_output.cpp");
  assert(result && "C++ lowering should succeed");

  // Verify output file exists
  FILE *f = fopen("/tmp/test_output.cpp", "r");
  assert(f && "C++ output file should exist");

  // Check for expected content
  char buffer[1024];
  bool found_namespace = false;
  bool found_struct = false;
  bool found_function = false;

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "namespace"))
      found_namespace = true;
    if (strstr(buffer, "struct Point"))
      found_struct = true;
    if (strstr(buffer, "int32_t add"))
      found_function = true;
  }
  fclose(f);

  assert(found_namespace && "Should contain namespace");
  assert(found_struct && "Should contain Point struct");
  assert(found_function && "Should contain add function");

  free_test_module(module);
  printf("✓ C++ lowering test passed\n");
}

// Test Go lowering
void test_go_lowering(void) {
  printf("Testing Go lowering...\n");
  NovaIRModule *module = create_test_module();

  bool result = nova_lower_to_go(module, "/tmp/test_output.go");
  assert(result && "Go lowering should succeed");

  FILE *f = fopen("/tmp/test_output.go", "r");
  assert(f && "Go output file should exist");

  char buffer[1024];
  bool found_package = false;
  bool found_struct = false;
  bool found_function = false;

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "package"))
      found_package = true;
    if (strstr(buffer, "type Point struct"))
      found_struct = true;
    if (strstr(buffer, "func add"))
      found_function = true;
  }
  fclose(f);

  assert(found_package && "Should contain package declaration");
  assert(found_struct && "Should contain Point struct");
  assert(found_function && "Should contain add function");

  free_test_module(module);
  printf("✓ Go lowering test passed\n");
}

// Test Swift lowering
void test_swift_lowering(void) {
  printf("Testing Swift lowering...\n");
  NovaIRModule *module = create_test_module();

  bool result = nova_lower_to_swift(module, "/tmp/test_output.swift");
  assert(result && "Swift lowering should succeed");

  FILE *f = fopen("/tmp/test_output.swift", "r");
  assert(f && "Swift output file should exist");

  char buffer[1024];
  bool found_import = false;
  bool found_struct = false;
  bool found_function = false;

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "import Foundation"))
      found_import = true;
    if (strstr(buffer, "struct Point"))
      found_struct = true;
    if (strstr(buffer, "func add"))
      found_function = true;
  }
  fclose(f);

  assert(found_import && "Should contain import statement");
  assert(found_struct && "Should contain Point struct");
  assert(found_function && "Should contain add function");

  free_test_module(module);
  printf("✓ Swift lowering test passed\n");
}

// Test Kotlin lowering
void test_kotlin_lowering(void) {
  printf("Testing Kotlin lowering...\n");
  NovaIRModule *module = create_test_module();

  bool result = nova_lower_to_kotlin(module, "/tmp/test_output.kt");
  assert(result && "Kotlin lowering should succeed");

  FILE *f = fopen("/tmp/test_output.kt", "r");
  assert(f && "Kotlin output file should exist");

  char buffer[1024];
  bool found_package = false;
  bool found_dataclass = false;
  bool found_function = false;

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "package"))
      found_package = true;
    if (strstr(buffer, "data class Point"))
      found_dataclass = true;
    if (strstr(buffer, "fun add"))
      found_function = true;
  }
  fclose(f);

  assert(found_package && "Should contain package declaration");
  assert(found_dataclass && "Should contain Point data class");
  assert(found_function && "Should contain add function");

  free_test_module(module);
  printf("✓ Kotlin lowering test passed\n");
}

// Test TypeScript lowering
void test_typescript_lowering(void) {
  printf("Testing TypeScript lowering...\n");
  NovaIRModule *module = create_test_module();

  bool result = nova_lower_to_typescript(module, "/tmp/test_output.ts");
  assert(result && "TypeScript lowering should succeed");

  FILE *f = fopen("/tmp/test_output.ts", "r");
  assert(f && "TypeScript output file should exist");

  char buffer[1024];
  bool found_strict = false;
  bool found_interface = false;
  bool found_function = false;

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "use strict"))
      found_strict = true;
    if (strstr(buffer, "interface Point"))
      found_interface = true;
    if (strstr(buffer, "function add"))
      found_function = true;
  }
  fclose(f);

  assert(found_strict && "Should contain 'use strict'");
  assert(found_interface && "Should contain Point interface");
  assert(found_function && "Should contain add function");

  free_test_module(module);
  printf("✓ TypeScript lowering test passed\n");
}

// Test C lowering
void test_c_lowering(void) {
  printf("Testing C lowering...\n");
  NovaIRModule *module = create_test_module();

  bool result = nova_lower_to_c(module, "/tmp/test_output.h");
  assert(result && "C lowering should succeed");

  FILE *f = fopen("/tmp/test_output.h", "r");
  assert(f && "C output file should exist");

  char buffer[1024];
  bool found_guard = false;
  bool found_typedef = false;
  bool found_function = false;

  while (fgets(buffer, sizeof(buffer), f)) {
    if (strstr(buffer, "#ifndef"))
      found_guard = true;
    if (strstr(buffer, "typedef struct"))
      found_typedef = true;
    if (strstr(buffer, "int32_t add"))
      found_function = true;
  }
  fclose(f);

  assert(found_guard && "Should contain header guard");
  assert(found_typedef && "Should contain Point typedef");
  assert(found_function && "Should contain add function");

  free_test_module(module);
  printf("✓ C lowering test passed\n");
}

int main(void) {
  printf("=== Multi-Language Lowering Test Suite ===\n\n");

  test_cpp_lowering();
  test_go_lowering();
  test_swift_lowering();
  test_kotlin_lowering();
  test_typescript_lowering();
  test_c_lowering();

  printf("\n=== All tests passed! ===\n");
  yield 0;
}
