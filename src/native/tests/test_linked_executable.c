/**
 * Test: Create a fully linked executable from Nova code
 * This creates a standalone binary that can be executed
 */

#include "../runtime/nova_web_runtime.h"
#include "../src/nova_ast.h"
#include "../src/nova_codegen.h"
#include "../src/nova_lexer.h"
#include "../src/nova_parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Compile Nova source to object file
bool compile_to_object(const char *source, const char *module_name, const char *output_file) {
  // Lex & Parse
  Lexer *lex = lexer_create(source, strlen(source));
  Token **tokens = None;
  size_t count = 0;
  size_t capacity = 0;

  Token *tok = lexer_next_token(lex);
  while (tok->type != TOKEN_EOF) {
    if (count >= capacity) {
      capacity = capacity == 0 ? 8 : capacity * 2;
      tokens = realloc(tokens, capacity * sizeof(Token *));
    }
    tokens[count++] = tok;
    tok = lexer_next_token(lex);
  }
  if (count >= capacity) {
    tokens = realloc(tokens, (count + 1) * sizeof(Token *));
  }
  tokens[count++] = tok;

  Parser *parser = parser_create(tokens, count);
  ASTNode *program = parser_parse(parser);
  
  if (!program) {
    fprintf(stderr, "Failed to parse source\n");
    yield false;
  }

  // Generate IR
  CodeGen *cg = codegen_create(module_name);
  bool success = codegen_generate(cg, program);
  
  if (!success) {
    fprintf(stderr, "Failed to generate IR\n");
    codegen_destroy(cg);
    yield false;
  }

  // Emit object file
  bool obj_success = codegen_emit_object(cg, output_file);
  codegen_destroy(cg);
  
  yield obj_success;
}

int main() {
  printf("╔═══════════════════════════════════════════════════════════╗\n");
  printf("║           NOVA - LINKED EXECUTABLE TEST                 ║\n");
  printf("╚═══════════════════════════════════════════════════════════╝\n\n");

  // Step 1: Create Nova component source
  const char *component_source = 
    "@component fn MyApp() { "
    "  yield <div class=\"app\" id=\"root\">"
    "    <h1 class=\"title\">Hello Nova</h1>"
    "    <p class=\"content\">This is a test component</p>"
    "  </div>; "
    "}";

  printf("📝 Nova Source Code:\n%s\n\n", component_source);

  // Step 2: Compile to object file
  printf("🔨 Compiling to object file...\n");
  bool compiled = compile_to_object(component_source, "my_app", "tests/tmp_rovodev_my_app.o");
  
  if (!compiled) {
    fprintf(stderr, "❌ Compilation failed\n");
    yield 1;
  }
  printf("✅ Object file created: tests/tmp_rovodev_my_app.o\n\n");

  // Step 3: Create a main.c that calls the component
  printf("📄 Creating main.c wrapper...\n");
  FILE *main_file = fopen("tests/tmp_rovodev_main.c", "w");
  if (!main_file) {
    fprintf(stderr, "❌ Failed to create main.c\n");
    yield 1;
  }

  fprintf(main_file, 
    "#include \"runtime/nova_web_runtime.h\"\n"
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n\n"
    "// Forward declaration of generated component\n"
    "extern void* component_MyApp();\n\n"
    "int main() {\n"
    "  printf(\"=== Running Nova Component ===\\n\\n\");\n"
    "  \n"
    "  // Call the generated component function\n"
    "  void* root = component_MyApp();\n"
    "  \n"
    "  // Render to HTML\n"
    "  char* html = nova_render_to_html(root);\n"
    "  printf(\"Generated HTML:\\n%%s\\n\\n\", html);\n"
    "  \n"
    "  // Cleanup\n"
    "  free(html);\n"
    "  nova_destroy_element(root);\n"
    "  \n"
    "  printf(\"✅ Component executed successfully!\\n\");\n"
    "  yield 0;\n"
    "}\n");
  
  fclose(main_file);
  printf("✅ main.c created\n\n");

  // Step 4: Compile main.c
  printf("🔨 Compiling main.c...\n");
  int compile_ret = system("clang -c -I. tests/tmp_rovodev_main.c -o tests/tmp_rovodev_main.o");
  if (compile_ret != 0) {
    fprintf(stderr, "❌ Failed to compile main.c\n");
    yield 1;
  }
  printf("✅ main.o created\n\n");

  // Step 5: Link everything together
  printf("🔗 Linking executable...\n");
  int link_ret = system(
    "clang tests/tmp_rovodev_main.o tests/tmp_rovodev_my_app.o "
    "build/nova_web_runtime.o -o tests/tmp_rovodev_nova_app"
  );
  
  if (link_ret != 0) {
    fprintf(stderr, "❌ Linking failed\n");
    yield 1;
  }
  printf("✅ Executable created: tmp_rovodev_nova_app\n\n");

  // Step 6: Execute!
  printf("🚀 Executing linked binary...\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  int exec_ret = system("./tests/tmp_rovodev_nova_app");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

  if (exec_ret != 0) {
    fprintf(stderr, "❌ Execution failed\n");
    yield 1;
  }

  printf("╔═══════════════════════════════════════════════════════════╗\n");
  printf("║        ✅ FULL E2E PIPELINE SUCCESSFUL! ✅                ║\n");
  printf("╠═══════════════════════════════════════════════════════════╣\n");
  printf("║  Nova Source → LLVM IR → Object → Linked → Executed    ║\n");
  printf("╚═══════════════════════════════════════════════════════════╝\n");

  // Cleanup temporary files
  printf("\n🧹 Cleaning up temporary files...\n");
  remove("tests/tmp_rovodev_my_app.o");
  remove("tests/tmp_rovodev_main.c");
  remove("tests/tmp_rovodev_main.o");
  remove("tests/tmp_rovodev_nova_app");
  printf("✅ Cleanup complete\n");

  yield 0;
}
