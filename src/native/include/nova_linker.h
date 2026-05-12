/**
 * Nova Linker
 * Links object files and resolves symbols to create executables
 */

#ifndef NOVA_LINKER_H
#define NOVA_LINKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// SYMBOL TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  SYMBOL_LOCAL,      // Local to object file
  SYMBOL_GLOBAL,     // Globally visible
  SYMBOL_WEAK,       // Weak symbol (can be overridden)
  SYMBOL_EXTERNAL,   // Defined in another file
  SYMBOL_UNDEFINED,  // Not yet resolved
} SymbolLinkage;

typedef enum {
  SYMBOL_TYPE_FUNCTION,
  SYMBOL_TYPE_DATA,
  SYMBOL_TYPE_TLS,      // Thread-local storage
  SYMBOL_TYPE_SECTION,
} SymbolType;

typedef struct {
  char *name;
  SymbolLinkage linkage;
  SymbolType type;
  
  uint64_t address;      // Virtual address (0 if unresolved)
  uint64_t size;
  
  const char *section;   // Section name (.text, .data, etc.)
  size_t object_index;   // Which object file defines this
  
  bool is_defined;
  bool is_used;
} LinkSymbol;

// ═══════════════════════════════════════════════════════════════════════════
// RELOCATION
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  RELOC_ABSOLUTE,    // Absolute address
  RELOC_RELATIVE,    // PC-relative
  RELOC_PLT,         // Procedure linkage table
  RELOC_GOT,         // Global offset table
} RelocationType;

typedef struct {
  uint64_t offset;           // Offset in section
  RelocationType type;
  const char *symbol_name;
  int64_t addend;
} Relocation;

// ═══════════════════════════════════════════════════════════════════════════
// OBJECT FILE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *name;
  
  // Code sections
  uint8_t *text_data;
  size_t text_size;
  
  uint8_t *rodata_data;
  size_t rodata_size;
  
  uint8_t *data_data;
  size_t data_size;
  
  uint8_t *bss_data;
  size_t bss_size;
  
  // Symbols
  LinkSymbol *symbols;
  size_t symbol_count;
  
  // Relocations
  Relocation *relocations;
  size_t relocation_count;
} ObjectFile;

// ═══════════════════════════════════════════════════════════════════════════
// LINKER CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  ObjectFile *objects;
  size_t object_count;
  size_t object_capacity;
  
  // Global symbol table
  LinkSymbol **symbol_table;
  size_t symbol_capacity;
  size_t symbol_count;
  
  // Linked sections
  uint8_t *text_section;
  size_t text_size;
  uint64_t text_base;
  
  uint8_t *rodata_section;
  size_t rodata_size;
  uint64_t rodata_base;
  
  uint8_t *data_section;
  size_t data_size;
  uint64_t data_base;
  
  uint8_t *bss_section;
  size_t bss_size;
  uint64_t bss_base;
  
  // Entry point
  const char *entry_symbol;
  uint64_t entry_address;
  
  // Errors
  bool had_error;
  char error_message[512];
} Linker;

// ═══════════════════════════════════════════════════════════════════════════
// LINKER API
// ═══════════════════════════════════════════════════════════════════════════

// Linker lifecycle
Linker *linker_create(void);
void linker_destroy(Linker *linker);

// Add object files
bool linker_add_object(Linker *linker, ObjectFile *obj);
bool linker_add_object_file(Linker *linker, const char *filename);

// Link process
bool linker_link(Linker *linker);
bool linker_resolve_symbols(Linker *linker);
bool linker_relocate(Linker *linker);

// Output
bool linker_emit_executable(Linker *linker, const char *output_path);
bool linker_emit_shared_library(Linker *linker, const char *output_path);

// Error handling
bool linker_had_error(const Linker *linker);
const char *linker_get_error(const Linker *linker);

// ═══════════════════════════════════════════════════════════════════════════
// OBJECT FILE OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

ObjectFile *object_file_create(const char *name);
void object_file_destroy(ObjectFile *obj);
bool object_file_load(const char *filename, ObjectFile **out);
bool object_file_save(const ObjectFile *obj, const char *filename);

// Add sections
void object_file_add_text(ObjectFile *obj, const uint8_t *data, size_t size);
void object_file_add_rodata(ObjectFile *obj, const uint8_t *data, size_t size);
void object_file_add_data(ObjectFile *obj, const uint8_t *data, size_t size);
void object_file_add_bss(ObjectFile *obj, size_t size);

// Add symbols
void object_file_add_symbol(ObjectFile *obj, LinkSymbol *symbol);
LinkSymbol *object_file_find_symbol(const ObjectFile *obj, const char *name);

// Add relocations
void object_file_add_relocation(ObjectFile *obj, Relocation *reloc);

// ═══════════════════════════════════════════════════════════════════════════
// SYMBOL OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

LinkSymbol *link_symbol_create(const char *name, SymbolLinkage linkage, SymbolType type);
void link_symbol_destroy(LinkSymbol *symbol);
LinkSymbol *link_symbol_copy(const LinkSymbol *symbol);

#endif // NOVA_LINKER_H
