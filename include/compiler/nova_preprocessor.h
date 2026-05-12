/**
 * Nova Preprocessor & Macro System
 * C-style preprocessor with macro expansion
 */

#ifndef NOVA_PREPROCESSOR_H
#define NOVA_PREPROCESSOR_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// MACRO DEFINITION
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  MACRO_OBJECT,      // #define PI 3.14
  MACRO_FUNCTION,    // #define MAX(a,b) ((a)>(b)?(a):(b))
  MACRO_BUILTIN,     // __FILE__, __LINE__, etc.
} MacroType;

typedef struct {
  char *name;
  MacroType type;
  
  // For object-like macros
  char *replacement;
  
  // For function-like macros
  char **parameters;
  size_t param_count;
  bool is_variadic;  // ... support
  
  // Metadata
  const char *defined_file;
  size_t defined_line;
  bool is_predefined;
} Macro;

// ═══════════════════════════════════════════════════════════════════════════
// PREPROCESSOR CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // Macro registry
  Macro **macros;
  size_t macro_count;
  size_t macro_capacity;
  
  // Include paths
  char **include_paths;
  size_t include_path_count;
  
  // Conditional compilation stack
  bool *if_stack;
  size_t if_stack_depth;
  size_t if_stack_capacity;
  
  // Current state
  const char *current_file;
  size_t current_line;
  
  // Output
  char *output;
  size_t output_size;
  size_t output_capacity;
  
  // Error tracking
  bool had_error;
  char error_message[512];
} Preprocessor;

// ═══════════════════════════════════════════════════════════════════════════
// PREPROCESSOR API
// ═══════════════════════════════════════════════════════════════════════════

// Lifecycle
Preprocessor *preprocessor_create(void);
void preprocessor_destroy(Preprocessor *pp);

// Macro management
void preprocessor_define(Preprocessor *pp, const char *name, const char *value);
void preprocessor_define_function(Preprocessor *pp, const char *name, 
                                   const char **params, size_t param_count,
                                   const char *replacement);
void preprocessor_undef(Preprocessor *pp, const char *name);
bool preprocessor_is_defined(const Preprocessor *pp, const char *name);
Macro *preprocessor_get_macro(const Preprocessor *pp, const char *name);

// Include paths
void preprocessor_add_include_path(Preprocessor *pp, const char *path);
char *preprocessor_find_include(const Preprocessor *pp, const char *filename, bool is_system);

// Preprocessing
char *preprocessor_process(Preprocessor *pp, const char *source, const char *filename);
char *preprocessor_process_file(Preprocessor *pp, const char *filename);

// Predefined macros
void preprocessor_define_builtin_macros(Preprocessor *pp);

// Error handling
bool preprocessor_had_error(const Preprocessor *pp);
const char *preprocessor_get_error(const Preprocessor *pp);

// ═══════════════════════════════════════════════════════════════════════════
// MACRO EXPANSION
// ═══════════════════════════════════════════════════════════════════════════

char *macro_expand(Preprocessor *pp, const char *text);
char *macro_expand_function(Preprocessor *pp, Macro *macro, char **args, size_t arg_count);

// ═══════════════════════════════════════════════════════════════════════════
// DIRECTIVES
// ═══════════════════════════════════════════════════════════════════════════

// #define, #undef
bool process_define(Preprocessor *pp, const char *line);
bool process_undef(Preprocessor *pp, const char *line);

// #include
bool process_include(Preprocessor *pp, const char *line);

// #if, #ifdef, #ifndef, #elif, #else, #endif
bool process_if(Preprocessor *pp, const char *condition);
bool process_ifdef(Preprocessor *pp, const char *name);
bool process_ifndef(Preprocessor *pp, const char *name);
bool process_elif(Preprocessor *pp, const char *condition);
bool process_else(Preprocessor *pp);
bool process_endif(Preprocessor *pp);

// #error, #warning, #pragma
bool process_error(Preprocessor *pp, const char *message);
bool process_warning(Preprocessor *pp, const char *message);
bool process_pragma(Preprocessor *pp, const char *directive);

// ═══════════════════════════════════════════════════════════════════════════
// CONDITIONAL COMPILATION
// ═══════════════════════════════════════════════════════════════════════════

bool evaluate_condition(Preprocessor *pp, const char *condition);
bool should_skip_block(const Preprocessor *pp);

// ═══════════════════════════════════════════════════════════════════════════
// BUILTIN MACROS
// ═══════════════════════════════════════════════════════════════════════════

#define BUILTIN_FILE       "__FILE__"
#define BUILTIN_LINE       "__LINE__"
#define BUILTIN_DATE       "__DATE__"
#define BUILTIN_TIME       "__TIME__"
#define BUILTIN_TIMESTAMP  "__TIMESTAMP__"
#define BUILTIN_COUNTER    "__COUNTER__"

#endif // NOVA_PREPROCESSOR_H
