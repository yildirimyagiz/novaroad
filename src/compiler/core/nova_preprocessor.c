#include "nova_common.h"

/**
 * Nova Preprocessor Implementation
 */

#include "compiler/nova_preprocessor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define INITIAL_MACRO_CAPACITY 64
#define INITIAL_OUTPUT_CAPACITY 4096

// ═══════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

Preprocessor *preprocessor_create(void) {
  Preprocessor *pp = (Preprocessor *)calloc(1, sizeof(Preprocessor));
  if (!pp) return NULL;
  
  pp->macro_capacity = INITIAL_MACRO_CAPACITY;
  pp->macros = (Macro **)calloc(pp->macro_capacity, sizeof(Macro*));
  
  pp->if_stack_capacity = 32;
  pp->if_stack = (bool *)malloc(pp->if_stack_capacity * sizeof(bool));
  pp->if_stack_depth = 0;
  
  pp->output_capacity = INITIAL_OUTPUT_CAPACITY;
  pp->output = (char *)malloc(pp->output_capacity);
  pp->output_size = 0;
  
  // Define builtin macros
  preprocessor_define_builtin_macros(pp);
  
  return pp;
}

void preprocessor_destroy(Preprocessor *pp) {
  if (!pp) return;
  
  for (size_t i = 0; i < pp->macro_count; i++) {
    Macro *m = pp->macros[i];
    free(m->name);
    free(m->replacement);
    if (m->parameters) {
      for (size_t j = 0; j < m->param_count; j++) {
        free(m->parameters[j]);
      }
      free(m->parameters);
    }
    free(m);
  }
  free(pp->macros);
  
  for (size_t i = 0; i < pp->include_path_count; i++) {
    free(pp->include_paths[i]);
  }
  free(pp->include_paths);
  
  free(pp->if_stack);
  free(pp->output);
  free(pp);
}

// ═══════════════════════════════════════════════════════════════════════════
// MACRO MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void preprocessor_define(Preprocessor *pp, const char *name, const char *value) {
  if (!pp || !name) return;
  
  Macro *macro = (Macro *)calloc(1, sizeof(Macro));
  macro->name = strdup(name);
  macro->type = MACRO_OBJECT;
  macro->replacement = value ? strdup(value) : strdup("");
  macro->defined_file = pp->current_file;
  macro->defined_line = pp->current_line;
  
  if (pp->macro_count >= pp->macro_capacity) {
    pp->macro_capacity *= 2;
    pp->macros = (Macro **)realloc(pp->macros, pp->macro_capacity * sizeof(Macro*));
  }
  
  pp->macros[pp->macro_count++] = macro;
}

void preprocessor_define_function(Preprocessor *pp, const char *name,
                                   const char **params, size_t param_count,
                                   const char *replacement) {
  if (!pp || !name) return;
  
  Macro *macro = (Macro *)calloc(1, sizeof(Macro));
  macro->name = strdup(name);
  macro->type = MACRO_FUNCTION;
  macro->replacement = replacement ? strdup(replacement) : strdup("");
  macro->param_count = param_count;
  
  if (param_count > 0) {
    macro->parameters = (char **)malloc(param_count * sizeof(char*));
    for (size_t i = 0; i < param_count; i++) {
      macro->parameters[i] = strdup(params[i]);
    }
  }
  
  if (pp->macro_count >= pp->macro_capacity) {
    pp->macro_capacity *= 2;
    pp->macros = (Macro **)realloc(pp->macros, pp->macro_capacity * sizeof(Macro*));
  }
  
  pp->macros[pp->macro_count++] = macro;
}

void preprocessor_undef(Preprocessor *pp, const char *name) {
  if (!pp || !name) return;
  
  for (size_t i = 0; i < pp->macro_count; i++) {
    if (strcmp(pp->macros[i]->name, name) == 0) {
      Macro *m = pp->macros[i];
      free(m->name);
      free(m->replacement);
      if (m->parameters) {
        for (size_t j = 0; j < m->param_count; j++) {
          free(m->parameters[j]);
        }
        free(m->parameters);
      }
      free(m);
      
      // Shift remaining macros
      memmove(&pp->macros[i], &pp->macros[i + 1], 
              (pp->macro_count - i - 1) * sizeof(Macro*));
      pp->macro_count--;
      return;
    }
  }
}

bool preprocessor_is_defined(const Preprocessor *pp, const char *name) {
  return preprocessor_get_macro(pp, name) != NULL;
}

Macro *preprocessor_get_macro(const Preprocessor *pp, const char *name) {
  if (!pp || !name) return NULL;
  
  for (size_t i = 0; i < pp->macro_count; i++) {
    if (strcmp(pp->macros[i]->name, name) == 0) {
      return pp->macros[i];
    }
  }
  
  return NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// INCLUDE PATHS
// ═══════════════════════════════════════════════════════════════════════════

void preprocessor_add_include_path(Preprocessor *pp, const char *path) {
  if (!pp || !path) return;
  
  pp->include_paths = (char **)realloc(pp->include_paths,
                                        (pp->include_path_count + 1) * sizeof(char*));
  pp->include_paths[pp->include_path_count++] = strdup(path);
}

char *preprocessor_find_include(const Preprocessor *pp, const char *filename, bool is_system) {
  if (!pp || !filename) return NULL;
  
  // Try current directory first for "..." includes
  if (!is_system) {
    FILE *f = fopen(filename, "r");
    if (f) {
      fclose(f);
      return strdup(filename);
    }
  }
  
  // Try include paths
  for (size_t i = 0; i < pp->include_path_count; i++) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", pp->include_paths[i], filename);
    
    FILE *f = fopen(path, "r");
    if (f) {
      fclose(f);
      return strdup(path);
    }
  }
  
  return NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// BUILTIN MACROS
// ═══════════════════════════════════════════════════════════════════════════

void preprocessor_define_builtin_macros(Preprocessor *pp) {
  if (!pp) return;
  
  // Standard predefined macros
  preprocessor_define(pp, "__NOVA__", "1");
  preprocessor_define(pp, "__VERSION__", "1.0.0");
  
  // Date and time
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  
  char date[32], time_str[32];
  strftime(date, sizeof(date), "%b %d %Y", tm_info);
  strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
  
  preprocessor_define(pp, BUILTIN_DATE, date);
  preprocessor_define(pp, BUILTIN_TIME, time_str);
  
  // Platform detection
#ifdef __linux__
  preprocessor_define(pp, "__LINUX__", "1");
#endif
#ifdef _WIN32
  preprocessor_define(pp, "__WINDOWS__", "1");
#endif
#ifdef __APPLE__
  preprocessor_define(pp, "__APPLE__", "1");
#endif
  
  // Architecture
#ifdef __x86_64__
  preprocessor_define(pp, "__X86_64__", "1");
#endif
#ifdef __aarch64__
  preprocessor_define(pp, "__ARM64__", "1");
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// PREPROCESSING
// ═══════════════════════════════════════════════════════════════════════════

static void append_output(Preprocessor *pp, const char *text) {
  size_t len = strlen(text);
  
  while (pp->output_size + len + 1 >= pp->output_capacity) {
    pp->output_capacity *= 2;
    pp->output = (char *)realloc(pp->output, pp->output_capacity);
  }
  
  memcpy(pp->output + pp->output_size, text, len);
  pp->output_size += len;
  pp->output[pp->output_size] = '\0';
}

char *preprocessor_process(Preprocessor *pp, const char *source, const char *filename) {
  if (!pp || !source) return NULL;
  
  pp->current_file = filename;
  pp->current_line = 1;
  pp->output_size = 0;
  pp->output[0] = '\0';
  
  const char *line_start = source;
  const char *p = source;
  
  while (*p) {
    if (*p == '#' && (p == source || *(p-1) == '\n')) {
      // Preprocessor directive
      const char *directive_start = p + 1;
      while (*p && *p != '\n') p++;
      
      size_t directive_len = p - directive_start;
      char *directive = (char *)malloc(directive_len + 1);
      memcpy(directive, directive_start, directive_len);
      directive[directive_len] = '\0';
      
      // Process directive (simplified)
      // Full implementation would parse and execute directives
      
      free(directive);
    } else if (*p == '\n') {
      append_output(pp, "\n");
      pp->current_line++;
      line_start = p + 1;
      p++;
    } else {
      char buf[2] = {*p, '\0'};
      append_output(pp, buf);
      p++;
    }
  }
  
  return strdup(pp->output);
}

char *preprocessor_process_file(Preprocessor *pp, const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    snprintf(pp->error_message, sizeof(pp->error_message), 
             "Failed to open file: %s", filename);
    pp->had_error = true;
    return NULL;
  }
  
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *source = (char *)malloc(size + 1);
  fread(source, 1, size, f);
  source[size] = '\0';
  fclose(f);
  
  char *result = preprocessor_process(pp, source, filename);
  free(source);
  
  return result;
}

bool preprocessor_had_error(const Preprocessor *pp) {
  return pp && pp->had_error;
}

const char *preprocessor_get_error(const Preprocessor *pp) {
  return pp ? pp->error_message : "No preprocessor";
}
