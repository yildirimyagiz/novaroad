#include <stdbool.h>

/**
 * src/compiler/module_registry.c
 * ──────────────────────────────────────────────────────────────────────────────
 * Module registry implementation for Nova's import/export system.
 */

#include "compiler/ast.h"
#include "compiler/lexer.h"
#include "compiler/module_registry.h"
#include "compiler/parser.h"
#include "compiler/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Registry lifecycle
 * ──────────────────────────────────────────────────────── */

nova_module_registry_t *nova_module_registry_create(void) {
  nova_module_registry_t *reg = calloc(1, sizeof(nova_module_registry_t));
  if (!reg)
    return NULL;
  reg->search_path = strdup(".");
  return reg;
}

static void export_free(nova_export_t *e) {
  while (e) {
    nova_export_t *next = e->next;
    free(e->name);
    nova_type_free(e->type);
    free(e);
    e = next;
  }
}

static void module_free(nova_module_t *m) {
  while (m) {
    nova_module_t *next = m->next;
    free(m->name);
    free(m->filepath);
    export_free(m->exports);
    free(m);
    m = next;
  }
}

void nova_module_registry_destroy(nova_module_registry_t *reg) {
  if (!reg)
    return;
  module_free(reg->modules);
  free(reg->search_path);
  free(reg);
}

/* ── Module registration
 * ─────────────────────────────────────────────────────── */

nova_module_t *nova_module_registry_add(nova_module_registry_t *reg,
                                        const char *name,
                                        const char *filepath) {
  if (!reg || !name)
    return NULL;

  /* Avoid duplicates */
  nova_module_t *existing = nova_module_registry_find(reg, name);
  if (existing)
    return existing;

  nova_module_t *m = calloc(1, sizeof(nova_module_t));
  if (!m)
    return NULL;
  m->name = strdup(name);
  m->filepath = filepath ? strdup(filepath) : NULL;
  m->state = NOVA_MODULE_PENDING;

  /* Prepend to linked list */
  m->next = reg->modules;
  reg->modules = m;
  reg->count++;
  return m;
}

nova_module_t *nova_module_registry_find(nova_module_registry_t *reg,
                                         const char *name) {
  if (!reg || !name)
    return NULL;
  for (nova_module_t *m = reg->modules; m; m = m->next) {
    if (strcmp(m->name, name) == 0)
      return m;
  }
  return NULL;
}

void nova_module_registry_set_search_path(nova_module_registry_t *reg,
                                          const char *path) {
  if (!reg)
    return;
  free(reg->search_path);
  reg->search_path = path ? strdup(path) : strdup(".");
}

/* ── Module discovery
 * ────────────────────────────────────────────────────────── */

nova_module_t *nova_module_registry_discover(nova_module_registry_t *reg,
                                             const char *name) {
  if (!reg || !name)
    return NULL;

  /* Already registered? */
  nova_module_t *existing = nova_module_registry_find(reg, name);
  if (existing)
    return existing;

  /* Search each directory in search_path */
  char *path_copy = strdup(reg->search_path ? reg->search_path : ".");
  /* Hierarchy: replace :: with / */
  const char *effective_name = name;
  if (strncmp(name, "crate::", 7) == 0) {
    effective_name = name + 7;
  }
  char *sanitized_name = strdup(effective_name);
  for (char *p = sanitized_name; *p; p++) {
    if (*p == ':' && *(p + 1) == ':') {
      *p = '/';
      memmove(p + 1, p + 2, strlen(p + 2) + 1);
    }
  }

  char *dir = strtok(path_copy, ":");
  char candidate[4096];
  /* Support both file modules and directory modules (mod.zn/mod.nova) */
  const char *formats[] = {"%s/%s.nova", "%s/%s.zn", "%s/%s/mod.nova",
                           "%s/%s/mod.zn", NULL};

  nova_module_t *found = NULL;

  while (dir && !found) {
    for (int i = 0; formats[i] && !found; i++) {
      snprintf(candidate, sizeof(candidate), formats[i], dir, sanitized_name);
      FILE *f = fopen(candidate, "r");
      if (f) {
        printf("nova: module '%s' found at '%s'\n", name, candidate);
        fclose(f);
        found = nova_module_registry_add(reg, name, candidate);
      }
    }
    dir = strtok(NULL, ":");
  }
  free(sanitized_name);
  free(path_copy);

  /* Note: Error reporting moved to resolve_import to support fallbacks */
  return found;
}

/* ── Export management
 * ───────────────────────────────────────────────────────── */

void nova_module_add_export(nova_module_t *module, const char *name,
                            nova_type_t *type, bool is_fn) {
  if (!module || !name)
    return;

  nova_export_t *e = calloc(1, sizeof(nova_export_t));
  if (!e)
    return;
  e->name = strdup(name);
  e->type = nova_type_clone(type);
  e->is_fn = is_fn;

  /* Prepend */
  e->next = module->exports;
  module->exports = e;
  module->export_count++;
}

static void register_exports(nova_module_t *module, nova_stmt_t *s) {
  if (!s)
    return;

  if (s->kind == STMT_BLOCK) {
    for (size_t i = 0; i < s->data.block.count; i++) {
      register_exports(module, s->data.block.statements[i]);
    }
    return;
  }

  if (s->kind == STMT_FN && s->data.fn_stmt.is_public) {
    /* Build TYPE_FN for the exported function */
    nova_type_t *fn_type = calloc(1, sizeof(nova_type_t));
    if (!fn_type)
      return;
    fn_type->kind = TYPE_FN;
    fn_type->data.fn.param_count = s->data.fn_stmt.param_count;
    fn_type->data.fn.return_type =
        s->data.fn_stmt.return_type
            ? nova_type_clone(s->data.fn_stmt.return_type)
            : nova_type_void();
    /* param types */
    if (s->data.fn_stmt.param_count > 0) {
      fn_type->data.fn.params =
          calloc(s->data.fn_stmt.param_count, sizeof(nova_type_t *));
      for (size_t j = 0; j < s->data.fn_stmt.param_count; j++) {
        fn_type->data.fn.params[j] =
            s->data.fn_stmt.params[j]->type
                ? nova_type_clone(s->data.fn_stmt.params[j]->type)
                : nova_type_i32();
      }
    } else {
      fn_type->data.fn.params = NULL;
    }
    nova_module_add_export(module, s->data.fn_stmt.name, fn_type, true);
    nova_type_free(fn_type);
  } else if (s->kind == STMT_STRUCT_DECL && s->data.struct_decl) {
    /* Export shape/struct as TYPE_DATA (all are public in module context) */
    nova_type_t *struct_type = calloc(1, sizeof(nova_type_t));
    if (struct_type) {
      struct_type->kind = TYPE_DATA;
      struct_type->data.name =
          strdup(s->data.struct_decl->name ? s->data.struct_decl->name : "");
      nova_module_add_export(module, s->data.struct_decl->name, struct_type,
                             false);
      nova_type_free(struct_type);
    }
  } else if (s->kind == STMT_ENUM_DECL) {
    /* Export cases/enum as TYPE_DATA */
    nova_type_t *enum_type = calloc(1, sizeof(nova_type_t));
    if (enum_type) {
      enum_type->kind = TYPE_DATA;
      enum_type->data.name =
          strdup(s->data.enum_decl.name ? s->data.enum_decl.name : "");
      nova_module_add_export(module, s->data.enum_decl.name, enum_type, false);
      nova_type_free(enum_type);
    }
  } else if (s->kind == STMT_VAR_DECL && s->data.var_decl.is_public) {
    /* Export pub var/let */
    nova_type_t *var_type = s->data.var_decl.type
                                ? nova_type_clone(s->data.var_decl.type)
                                : nova_type_i64();
    nova_module_add_export(module, s->data.var_decl.name, var_type, true);
    nova_type_free(var_type);
  }
}

/* ── Module compilation
 * ──────────────────────────────────────────────────────── */

bool nova_module_registry_compile(nova_module_registry_t *reg,
                                  nova_module_t *module) {
  if (!module)
    return false;

  printf("nova: compiling module '%s' from '%s'\n", module->name,
         module->filepath);
  if (module->state == NOVA_MODULE_COMPILED)
    return true;
  if (module->state == NOVA_MODULE_ERROR)
    return false;

  if (!module->filepath) {
    fprintf(stderr, "nova: module '%s' has no file path\n", module->name);
    module->state = NOVA_MODULE_ERROR;
    return false;
  }

  /* Read source */
  FILE *f = fopen(module->filepath, "r");
  if (!f) {
    fprintf(stderr, "nova: cannot open module file '%s'\n", module->filepath);
    module->state = NOVA_MODULE_ERROR;
    return false;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  char *source = malloc(fsize + 1);
  if (!source) {
    fclose(f);
    module->state = NOVA_MODULE_ERROR;
    return false;
  }
  fread(source, 1, fsize, f);
  source[fsize] = '\0';
  fclose(f);

  /* Parse */
  nova_lexer_t *lexer = nova_lexer_create(source);
  nova_parser_t *parser = nova_parser_create(lexer);
  size_t stmt_count = 0;
  nova_stmt_t **stmts = nova_parser_parse_statements(parser, &stmt_count);

  if (!stmts) {
    const char *err = nova_parser_get_error(parser);
    if (err) {
      fprintf(stderr, "nova: parser error in '%s': %s\n", module->filepath,
              err);
    } else {
      fprintf(stderr, "nova: unknown parser error in '%s'\n", module->filepath);
    }
    nova_parser_destroy(parser);
    nova_lexer_destroy(lexer);
    free(source);
    module->state = NOVA_MODULE_ERROR;
    return false;
  }

  nova_parser_destroy(parser);
  nova_lexer_destroy(lexer);
  free(source);

  /* Walk top-level statements and register exported symbols */
  for (size_t i = 0; i < stmt_count; i++) {
    register_exports(module, stmts[i]);
  }

  /* Free stmt array */
  for (size_t i = 0; i < stmt_count; i++) {
    if (stmts[i])
      nova_stmt_free(stmts[i]);
  }
  free(stmts);

  module->state = NOVA_MODULE_COMPILED;
  return true;
}

/* ── Import resolution
 * ───────────────────────────────────────────────────────── */

/*
 * nova_module_registry_resolve_import:
 * Injects exported symbols from module_name into the caller's symbol table.
 *
 * symbol_table_add_fn is a function pointer of type:
 *   void (*)(void *table, const char *name, const nova_type_t *type, bool
 * is_mutable)
 */
bool nova_module_registry_resolve_import(
    nova_module_registry_t *reg, void *symbol_table, void *symbol_table_add_fn,
    const char *module_name, const char **symbol_names, size_t symbol_count) {
  if (!reg || !module_name)
    return false;

  /* Find or discover module */
  nova_module_t *mod = nova_module_registry_find(reg, module_name);
  if (!mod) {
    mod = nova_module_registry_discover(reg, module_name);
    if (!mod && symbol_count == 0) {
      /* Fallback: try splitting last component (e.g. core::lexer::Lexer ->
       * core::lexer + Lexer) */
      char *last_colon = strrchr(module_name, ':');
      if (last_colon && last_colon > module_name && *(last_colon - 1) == ':') {
        char *parent_module =
            strndup(module_name, last_colon - 1 - module_name);
        const char *member_name = last_colon + 1;
        const char *member_names[] = {member_name};
        bool result = nova_module_registry_resolve_import(
            reg, symbol_table, symbol_table_add_fn, parent_module, member_names,
            1);
        free(parent_module);
        return result;
      }
    }
    if (!mod) {
      // Tracing for nova_module_registry_discover
      // if (!quiet) { // Assuming 'quiet' is a parameter or global flag
      if (mod) {
        printf("nova: module '%s' discovered at '%s'\n", module_name,
               mod->filepath);
      } else {
        //   printf("nova: module '%s' not found in search path\n",
        //   module_name);
      }
      // }
      fprintf(stderr, "nova: module '%s' not found in search path '%s'\n",
              module_name, reg->search_path ? reg->search_path : ".");
      return false;
    }
  }

  /* Compile if needed */
  if (mod->state == NOVA_MODULE_PENDING) {
    if (!nova_module_registry_compile(reg, mod))
      return false;
  }
  if (mod->state != NOVA_MODULE_COMPILED)
    return false;

  /* Type-safe function pointer cast */
  typedef void (*add_fn_t)(void *, const char *, const nova_type_t *, bool);
  add_fn_t add_fn = (add_fn_t)symbol_table_add_fn;

  bool all_found = true;

  if (symbol_count == 0 || (symbol_count == 1 && symbol_names &&
                            strcmp(symbol_names[0], "*") == 0)) {
    /* Wildcard import: inject all exports */
    for (nova_export_t *e = mod->exports; e; e = e->next) {
      if (add_fn)
        add_fn(symbol_table, e->name, e->type, false);
    }
  } else {
    /* Selective import */
    for (size_t i = 0; i < symbol_count; i++) {
      bool found = false;
      for (nova_export_t *e = mod->exports; e; e = e->next) {
        if (strcmp(e->name, symbol_names[i]) == 0) {
          if (add_fn)
            add_fn(symbol_table, e->name, e->type, false);
          found = true;
          break;
        }
      }
      if (!found) {
        fprintf(stderr, "nova: module '%s' does not export '%s'\n", module_name,
                symbol_names[i]);
        all_found = false;
      }
    }
  }

  return all_found;
}
