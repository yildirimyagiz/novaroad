/**
 * Nova Linker Implementation
 */

#include "compiler/nova_linker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// HASH TABLE FOR SYMBOLS
// ═══════════════════════════════════════════════════════════════════════════

#define SYMBOL_TABLE_SIZE 1024

static size_t hash_string(const char *str) {
  size_t hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash % SYMBOL_TABLE_SIZE;
}

// ═══════════════════════════════════════════════════════════════════════════
// LINKER LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

Linker *linker_create(void) {
  Linker *linker = (Linker *)calloc(1, sizeof(Linker));
  if (!linker) return NULL;
  
  linker->object_capacity = 16;
  linker->objects = (ObjectFile *)calloc(linker->object_capacity, sizeof(ObjectFile));
  
  linker->symbol_capacity = SYMBOL_TABLE_SIZE;
  linker->symbol_table = (LinkSymbol **)calloc(linker->symbol_capacity, sizeof(LinkSymbol*));
  
  linker->entry_symbol = "main";
  linker->text_base = 0x400000;  // Default base address (typical for x86-64)
  linker->had_error = false;
  
  return linker;
}

void linker_destroy(Linker *linker) {
  if (!linker) return;
  
  for (size_t i = 0; i < linker->object_count; i++) {
    object_file_destroy(&linker->objects[i]);
  }
  free(linker->objects);
  
  for (size_t i = 0; i < linker->symbol_capacity; i++) {
    LinkSymbol *sym = linker->symbol_table[i];
    while (sym) {
      LinkSymbol *next = (LinkSymbol *)sym->section;  // Abuse section pointer for chaining
      link_symbol_destroy(sym);
      sym = next;
    }
  }
  free(linker->symbol_table);
  
  free(linker->text_section);
  free(linker->rodata_section);
  free(linker->data_section);
  free(linker->bss_section);
  
  free(linker);
}

// ═══════════════════════════════════════════════════════════════════════════
// ERROR HANDLING
// ═══════════════════════════════════════════════════════════════════════════

static void linker_error(Linker *linker, const char *message) {
  if (!linker->had_error) {
    snprintf(linker->error_message, sizeof(linker->error_message), "%s", message);
    linker->had_error = true;
  }
}

bool linker_had_error(const Linker *linker) {
  return linker && linker->had_error;
}

const char *linker_get_error(const Linker *linker) {
  return linker ? linker->error_message : "No linker";
}

// ═══════════════════════════════════════════════════════════════════════════
// ADD OBJECT FILES
// ═══════════════════════════════════════════════════════════════════════════

bool linker_add_object(Linker *linker, ObjectFile *obj) {
  if (!linker || !obj) return false;
  
  if (linker->object_count >= linker->object_capacity) {
    linker->object_capacity *= 2;
    linker->objects = (ObjectFile *)realloc(linker->objects, 
                                             linker->object_capacity * sizeof(ObjectFile));
  }
  
  linker->objects[linker->object_count++] = *obj;
  return true;
}

bool linker_add_object_file(Linker *linker, const char *filename) {
  if (!linker || !filename) return false;
  
  ObjectFile *obj = NULL;
  if (!object_file_load(filename, &obj)) {
    linker_error(linker, "Failed to load object file");
    return false;
  }
  
  bool result = linker_add_object(linker, obj);
  free(obj);  // Copy was made
  return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// SYMBOL RESOLUTION
// ═══════════════════════════════════════════════════════════════════════════

static LinkSymbol *linker_lookup_symbol(Linker *linker, const char *name) {
  size_t hash = hash_string(name);
  LinkSymbol *sym = linker->symbol_table[hash];
  
  while (sym) {
    if (strcmp(sym->name, name) == 0) {
      return sym;
    }
    sym = (LinkSymbol *)sym->section;  // Next in chain
  }
  
  return NULL;
}

static bool linker_insert_symbol(Linker *linker, LinkSymbol *symbol) {
  size_t hash = hash_string(symbol->name);
  
  // Check for conflicts
  LinkSymbol *existing = linker_lookup_symbol(linker, symbol->name);
  if (existing) {
    // Both defined and not weak -> error
    if (existing->is_defined && symbol->is_defined &&
        existing->linkage != SYMBOL_WEAK && symbol->linkage != SYMBOL_WEAK) {
      char err[256];
      snprintf(err, sizeof(err), "Multiple definition of symbol '%s'", symbol->name);
      linker_error(linker, err);
      return false;
    }
    
    // Prefer defined over undefined
    if (symbol->is_defined && !existing->is_defined) {
      // Replace
      link_symbol_destroy(existing);
      linker->symbol_table[hash] = symbol;
    }
    
    return true;
  }
  
  // Insert new symbol
  symbol->section = (char *)linker->symbol_table[hash];  // Chain
  linker->symbol_table[hash] = symbol;
  linker->symbol_count++;
  
  return true;
}

bool linker_resolve_symbols(Linker *linker) {
  if (!linker) return false;
  
  // Pass 1: Collect all symbols
  for (size_t i = 0; i < linker->object_count; i++) {
    ObjectFile *obj = &linker->objects[i];
    
    for (size_t j = 0; j < obj->symbol_count; j++) {
      LinkSymbol *sym = link_symbol_copy(&obj->symbols[j]);
      sym->object_index = i;
      
      if (!linker_insert_symbol(linker, sym)) {
        return false;
      }
    }
  }
  
  // Pass 2: Check for undefined symbols
  for (size_t i = 0; i < linker->symbol_capacity; i++) {
    LinkSymbol *sym = linker->symbol_table[i];
    while (sym) {
      if (!sym->is_defined && sym->linkage != SYMBOL_WEAK) {
        char err[256];
        snprintf(err, sizeof(err), "Undefined symbol '%s'", sym->name);
        linker_error(linker, err);
        return false;
      }
      sym = (LinkSymbol *)sym->section;
    }
  }
  
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION LAYOUT
// ═══════════════════════════════════════════════════════════════════════════

static bool linker_layout_sections(Linker *linker) {
  if (!linker) return false;
  
  // Calculate total sizes
  size_t total_text = 0, total_rodata = 0, total_data = 0, total_bss = 0;
  
  for (size_t i = 0; i < linker->object_count; i++) {
    total_text += linker->objects[i].text_size;
    total_rodata += linker->objects[i].rodata_size;
    total_data += linker->objects[i].data_size;
    total_bss += linker->objects[i].bss_size;
  }
  
  // Allocate sections
  linker->text_section = (uint8_t *)malloc(total_text);
  linker->text_size = total_text;
  
  linker->rodata_section = (uint8_t *)malloc(total_rodata);
  linker->rodata_size = total_rodata;
  
  linker->data_section = (uint8_t *)malloc(total_data);
  linker->data_size = total_data;
  
  linker->bss_section = (uint8_t *)calloc(total_bss, 1);
  linker->bss_size = total_bss;
  
  // Assign addresses
  linker->rodata_base = linker->text_base + linker->text_size;
  linker->data_base = linker->rodata_base + linker->rodata_size;
  linker->bss_base = linker->data_base + linker->data_size;
  
  // Copy sections and update symbol addresses
  size_t text_offset = 0, rodata_offset = 0, data_offset = 0, bss_offset = 0;
  
  for (size_t i = 0; i < linker->object_count; i++) {
    ObjectFile *obj = &linker->objects[i];
    
    // Copy .text
    if (obj->text_size > 0) {
      memcpy(linker->text_section + text_offset, obj->text_data, obj->text_size);
      
      // Update symbol addresses
      for (size_t j = 0; j < obj->symbol_count; j++) {
        if (strcmp(obj->symbols[j].section, ".text") == 0) {
          LinkSymbol *global_sym = linker_lookup_symbol(linker, obj->symbols[j].name);
          if (global_sym) {
            global_sym->address = linker->text_base + text_offset + obj->symbols[j].address;
          }
        }
      }
      
      text_offset += obj->text_size;
    }
    
    // Copy .rodata
    if (obj->rodata_size > 0) {
      memcpy(linker->rodata_section + rodata_offset, obj->rodata_data, obj->rodata_size);
      rodata_offset += obj->rodata_size;
    }
    
    // Copy .data
    if (obj->data_size > 0) {
      memcpy(linker->data_section + data_offset, obj->data_data, obj->data_size);
      data_offset += obj->data_size;
    }
    
    // BSS
    bss_offset += obj->bss_size;
  }
  
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// RELOCATION
// ═══════════════════════════════════════════════════════════════════════════

bool linker_relocate(Linker *linker) {
  if (!linker) return false;
  
  for (size_t i = 0; i < linker->object_count; i++) {
    ObjectFile *obj = &linker->objects[i];
    
    for (size_t j = 0; j < obj->relocation_count; j++) {
      Relocation *reloc = &obj->relocations[j];
      
      // Find symbol
      LinkSymbol *sym = linker_lookup_symbol(linker, reloc->symbol_name);
      if (!sym) {
        char err[256];
        snprintf(err, sizeof(err), "Relocation references undefined symbol '%s'", 
                 reloc->symbol_name);
        linker_error(linker, err);
        return false;
      }
      
      // Calculate relocation value
      uint64_t value = sym->address + reloc->addend;
      
      // Apply relocation based on type
      uint8_t *patch_location = linker->text_section + reloc->offset;
      
      switch (reloc->type) {
        case RELOC_ABSOLUTE:
          *(uint64_t *)patch_location = value;
          break;
          
        case RELOC_RELATIVE: {
          uint64_t pc = linker->text_base + reloc->offset;
          int32_t relative = (int32_t)(value - pc - 4);
          *(int32_t *)patch_location = relative;
          break;
        }
          
        default:
          linker_error(linker, "Unsupported relocation type");
          return false;
      }
    }
  }
  
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// LINKING
// ═══════════════════════════════════════════════════════════════════════════

bool linker_link(Linker *linker) {
  if (!linker) return false;
  
  // Resolve symbols
  if (!linker_resolve_symbols(linker)) {
    return false;
  }
  
  // Layout sections
  if (!linker_layout_sections(linker)) {
    return false;
  }
  
  // Apply relocations
  if (!linker_relocate(linker)) {
    return false;
  }
  
  // Find entry point
  LinkSymbol *entry = linker_lookup_symbol(linker, linker->entry_symbol);
  if (!entry) {
    char err[256];
    snprintf(err, sizeof(err), "Entry point '%s' not found", linker->entry_symbol);
    linker_error(linker, err);
    return false;
  }
  linker->entry_address = entry->address;
  
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// OUTPUT
// ═══════════════════════════════════════════════════════════════════════════

bool linker_emit_executable(Linker *linker, const char *output_path) {
  if (!linker || !output_path) return false;
  
  FILE *f = fopen(output_path, "wb");
  if (!f) {
    linker_error(linker, "Failed to open output file");
    return false;
  }
  
  // Simple executable format (custom Nova format)
  // Header: magic, entry, section sizes
  fwrite("NOVA", 4, 1, f);
  fwrite(&linker->entry_address, sizeof(uint64_t), 1, f);
  fwrite(&linker->text_size, sizeof(size_t), 1, f);
  fwrite(&linker->rodata_size, sizeof(size_t), 1, f);
  fwrite(&linker->data_size, sizeof(size_t), 1, f);
  fwrite(&linker->bss_size, sizeof(size_t), 1, f);
  
  // Sections
  fwrite(linker->text_section, 1, linker->text_size, f);
  fwrite(linker->rodata_section, 1, linker->rodata_size, f);
  fwrite(linker->data_section, 1, linker->data_size, f);
  
  fclose(f);
  return true;
}

bool linker_emit_shared_library(Linker *linker, const char *output_path) {
  // TODO: Implement shared library generation
  linker_error(linker, "Shared library generation not yet implemented");
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// OBJECT FILE OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

ObjectFile *object_file_create(const char *name) {
  ObjectFile *obj = (ObjectFile *)calloc(1, sizeof(ObjectFile));
  if (!obj) return NULL;
  
  obj->name = name ? strdup(name) : NULL;
  return obj;
}

void object_file_destroy(ObjectFile *obj) {
  if (!obj) return;
  
  free(obj->name);
  free(obj->text_data);
  free(obj->rodata_data);
  free(obj->data_data);
  free(obj->bss_data);
  
  for (size_t i = 0; i < obj->symbol_count; i++) {
    link_symbol_destroy(&obj->symbols[i]);
  }
  free(obj->symbols);
  free(obj->relocations);
}

bool object_file_load(const char *filename, ObjectFile **out) {
  FILE *f = fopen(filename, "rb");
  if (!f) return false;
  
  ObjectFile *obj = object_file_create(filename);
  
  // Read sections (simplified format)
  fread(&obj->text_size, sizeof(size_t), 1, f);
  obj->text_data = (uint8_t *)malloc(obj->text_size);
  fread(obj->text_data, 1, obj->text_size, f);
  
  fread(&obj->symbol_count, sizeof(size_t), 1, f);
  obj->symbols = (LinkSymbol *)calloc(obj->symbol_count, sizeof(LinkSymbol));
  // Read symbols (simplified)
  
  fclose(f);
  *out = obj;
  return true;
}

bool object_file_save(const ObjectFile *obj, const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (!f) return false;
  
  fwrite(&obj->text_size, sizeof(size_t), 1, f);
  fwrite(obj->text_data, 1, obj->text_size, f);
  
  fwrite(&obj->symbol_count, sizeof(size_t), 1, f);
  // Write symbols
  
  fclose(f);
  return true;
}

void object_file_add_text(ObjectFile *obj, const uint8_t *data, size_t size) {
  if (!obj) return;
  obj->text_data = (uint8_t *)malloc(size);
  memcpy(obj->text_data, data, size);
  obj->text_size = size;
}

void object_file_add_symbol(ObjectFile *obj, LinkSymbol *symbol) {
  if (!obj || !symbol) return;
  
  obj->symbols = (LinkSymbol *)realloc(obj->symbols, 
                                        (obj->symbol_count + 1) * sizeof(LinkSymbol));
  obj->symbols[obj->symbol_count++] = *symbol;
}

void object_file_add_relocation(ObjectFile *obj, Relocation *reloc) {
  if (!obj || !reloc) return;
  
  obj->relocations = (Relocation *)realloc(obj->relocations,
                                            (obj->relocation_count + 1) * sizeof(Relocation));
  obj->relocations[obj->relocation_count++] = *reloc;
}

// ═══════════════════════════════════════════════════════════════════════════
// SYMBOL OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

LinkSymbol *link_symbol_create(const char *name, SymbolLinkage linkage, SymbolType type) {
  LinkSymbol *sym = (LinkSymbol *)calloc(1, sizeof(LinkSymbol));
  if (!sym) return NULL;
  
  sym->name = name ? strdup(name) : NULL;
  sym->linkage = linkage;
  sym->type = type;
  sym->is_defined = false;
  sym->is_used = false;
  
  return sym;
}

void link_symbol_destroy(LinkSymbol *symbol) {
  if (!symbol) return;
  free(symbol->name);
}

LinkSymbol *link_symbol_copy(const LinkSymbol *symbol) {
  if (!symbol) return NULL;
  
  LinkSymbol *copy = (LinkSymbol *)malloc(sizeof(LinkSymbol));
  memcpy(copy, symbol, sizeof(LinkSymbol));
  copy->name = symbol->name ? strdup(symbol->name) : NULL;
  
  return copy;
}
