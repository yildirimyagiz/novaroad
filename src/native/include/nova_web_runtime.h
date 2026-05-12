/**
 * Nova Web UI Runtime Library
 * Provides runtime support for JSX/DOM operations and CSS processing
 */

#ifndef NOVA_WEB_RUNTIME_H
#define NOVA_WEB_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// DOM ELEMENT STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct NovaAttribute {
  char *name;
  char *value;
  struct NovaAttribute *next;
} NovaAttribute;

typedef struct NovaElement {
  char *tag_name;
  NovaAttribute *attributes;
  struct NovaElement **children;
  size_t child_count;
  size_t child_capacity;
  char *text_content; // For text nodes
  void *style_object; // CSS style object
} NovaElement;

typedef struct {
  char *content;
  // Future: parsed CSS rules, selector matching, etc.
} NovaStyleObject;

// ═══════════════════════════════════════════════════════════════════════════
// RUNTIME API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create a new DOM element with the given tag name
 * @param tag_name The HTML tag name (e.g., "div", "span")
 * @return Pointer to the created element
 */
void *nova_create_element(const char *tag_name);

/**
 * Set an attribute on a DOM element
 * @param element The element to modify
 * @param name The attribute name
 * @param value The attribute value
 */
void nova_set_attribute(void *element, const char *name, const char *value);

/**
 * Append a child element to a parent element
 * @param parent The parent element
 * @param child The child element to append
 */
void nova_append_child(void *parent, void *child);

/**
 * Process CSS template string and return a style object
 * @param css_content The CSS content string
 * @return Pointer to a style object
 */
void *nova_process_css(const char *css_content);

/**
 * Apply a style object to an element
 * @param element The element to style
 * @param style_object The style object to apply
 */
void nova_apply_style(void *element, void *style_object);

/**
 * Render an element tree to HTML string
 * @param element The root element to render
 * @return Dynamically allocated HTML string (caller must free)
 */
char *nova_render_to_html(void *element);

/**
 * Destroy an element and free all associated memory
 * @param element The element to destroy
 */
void nova_destroy_element(void *element);

/**
 * Destroy a style object and free memory
 * @param style_object The style object to destroy
 */
void nova_destroy_style(void *style_object);

// ═══════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Get an attribute value from an element
 * @param element The element
 * @param name The attribute name
 * @return The attribute value, or NULL if not found
 */
const char *nova_get_attribute(void *element, const char *name);

/**
 * Print element tree for debugging
 * @param element The element to print
 * @param indent Indentation level
 */
void nova_debug_print_element(void *element, int indent);

#endif // NOVA_WEB_RUNTIME_H
