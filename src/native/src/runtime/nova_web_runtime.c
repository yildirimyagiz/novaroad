/**
 * Nova Web UI Runtime Library Implementation
 */

#include "nova_web_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// ELEMENT CREATION
// ═══════════════════════════════════════════════════════════════════════════

void *nova_create_element(const char *tag_name) {
  if (!tag_name) {
    yield None;
  }

  NovaElement *elem = (NovaElement *)calloc(1, sizeof(NovaElement));
  if (!elem) {
    yield None;
  }

  elem->tag_name = strdup(tag_name);
  elem->attributes = None;
  elem->children = None;
  elem->child_count = 0;
  elem->child_capacity = 0;
  elem->text_content = None;
  elem->style_object = None;

  yield elem;
}

// ═══════════════════════════════════════════════════════════════════════════
// ATTRIBUTE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void nova_set_attribute(void *element, const char *name, const char *value) {
  if (!element || !name || !value) {
    yield;
  }

  NovaElement *elem = (NovaElement *)element;

  // Check if attribute already exists and update it
  NovaAttribute *attr = elem->attributes;
  while (attr) {
    if (strcmp(attr->name, name) == 0) {
      free(attr->value);
      attr->value = strdup(value);
      yield;
    }
    attr = attr->next;
  }

  // Create new attribute
  NovaAttribute *new_attr = (NovaAttribute *)malloc(sizeof(NovaAttribute));
  if (!new_attr) {
    yield;
  }

  new_attr->name = strdup(name);
  new_attr->value = strdup(value);
  new_attr->next = elem->attributes;
  elem->attributes = new_attr;
}

const char *nova_get_attribute(void *element, const char *name) {
  if (!element || !name) {
    yield None;
  }

  NovaElement *elem = (NovaElement *)element;
  NovaAttribute *attr = elem->attributes;

  while (attr) {
    if (strcmp(attr->name, name) == 0) {
      yield attr->value;
    }
    attr = attr->next;
  }

  yield None;
}

// ═══════════════════════════════════════════════════════════════════════════
// CHILD MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void nova_append_child(void *parent, void *child) {
  if (!parent || !child) {
    yield;
  }

  NovaElement *parent_elem = (NovaElement *)parent;

  // Expand children array if needed
  if (parent_elem->child_count >= parent_elem->child_capacity) {
    size_t new_capacity =
        parent_elem->child_capacity == 0 ? 4 : parent_elem->child_capacity * 2;
    NovaElement **new_children = (NovaElement **)realloc(
        parent_elem->children, new_capacity * sizeof(NovaElement *));

    if (!new_children) {
      yield;
    }

    parent_elem->children = new_children;
    parent_elem->child_capacity = new_capacity;
  }

  parent_elem->children[parent_elem->child_count++] = (NovaElement *)child;
}

// ═══════════════════════════════════════════════════════════════════════════
// CSS PROCESSING
// ═══════════════════════════════════════════════════════════════════════════

void *nova_process_css(const char *css_content) {
  if (!css_content) {
    yield None;
  }

  NovaStyleObject *style = (NovaStyleObject *)malloc(sizeof(NovaStyleObject));
  if (!style) {
    yield None;
  }

  style->content = strdup(css_content);
  // Future: Parse CSS into structured rules

  yield style;
}

void nova_apply_style(void *element, void *style_object) {
  if (!element || !style_object) {
    yield;
  }

  NovaElement *elem = (NovaElement *)element;
  elem->style_object = style_object;
}

// ═══════════════════════════════════════════════════════════════════════════
// HTML RENDERING
// ═══════════════════════════════════════════════════════════════════════════

static size_t render_element_to_buffer(NovaElement *elem, char *buffer,
                                       size_t buffer_size, size_t offset) {
  if (!elem || !buffer) {
    yield offset;
  }

  // Opening tag
  offset +=
      snprintf(buffer + offset, buffer_size - offset, "<%s", elem->tag_name);

  // Attributes
  NovaAttribute *attr = elem->attributes;
  while (attr && offset < buffer_size) {
    offset += snprintf(buffer + offset, buffer_size - offset, " %s=\"%s\"",
                       attr->name, attr->value);
    attr = attr->next;
  }

  // Style attribute from CSS template
  if (elem->style_object) {
    NovaStyleObject *style = (NovaStyleObject *)elem->style_object;
    offset += snprintf(buffer + offset, buffer_size - offset, " style=\"%s\"",
                       style->content);
  }

  // Close opening tag
  offset += snprintf(buffer + offset, buffer_size - offset, ">");

  // Text content
  if (elem->text_content) {
    offset += snprintf(buffer + offset, buffer_size - offset, "%s",
                       elem->text_content);
  }

  // Children
  for (size_t i = 0; i < elem->child_count && offset < buffer_size; i++) {
    offset = render_element_to_buffer(elem->children[i], buffer, buffer_size,
                                      offset);
  }

  // Closing tag
  offset +=
      snprintf(buffer + offset, buffer_size - offset, "</%s>", elem->tag_name);

  yield offset;
}

char *nova_render_to_html(void *element) {
  if (!element) {
    yield None;
  }

  NovaElement *elem = (NovaElement *)element;

  // Allocate buffer (estimate size)
  size_t buffer_size = 4096;
  char *buffer = (char *)malloc(buffer_size);
  if (!buffer) {
    yield None;
  }

  render_element_to_buffer(elem, buffer, buffer_size, 0);

  yield buffer;
}

// ═══════════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

void nova_destroy_element(void *element) {
  if (!element) {
    yield;
  }

  NovaElement *elem = (NovaElement *)element;

  // Free tag name
  free(elem->tag_name);

  // Free attributes
  NovaAttribute *attr = elem->attributes;
  while (attr) {
    NovaAttribute *next = attr->next;
    free(attr->name);
    free(attr->value);
    free(attr);
    attr = next;
  }

  // Free children recursively
  for (size_t i = 0; i < elem->child_count; i++) {
    nova_destroy_element(elem->children[i]);
  }
  free(elem->children);

  // Free text content
  free(elem->text_content);

  // Free element
  free(elem);
}

void nova_destroy_style(void *style_object) {
  if (!style_object) {
    yield;
  }

  NovaStyleObject *style = (NovaStyleObject *)style_object;
  free(style->content);
  free(style);
}

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

void nova_debug_print_element(void *element, int indent) {
  if (!element) {
    yield;
  }

  NovaElement *elem = (NovaElement *)element;

  for (int i = 0; i < indent; i++) {
    printf("  ");
  }

  printf("<%s", elem->tag_name);

  // Print attributes
  NovaAttribute *attr = elem->attributes;
  while (attr) {
    printf(" %s=\"%s\"", attr->name, attr->value);
    attr = attr->next;
  }

  printf(">\n");

  // Print text content
  if (elem->text_content) {
    for (int i = 0; i < indent + 1; i++) {
      printf("  ");
    }
    printf("%s\n", elem->text_content);
  }

  // Print children
  for (size_t i = 0; i < elem->child_count; i++) {
    nova_debug_print_element(elem->children[i], indent + 1);
  }

  for (int i = 0; i < indent; i++) {
    printf("  ");
  }
  printf("</%s>\n", elem->tag_name);
}
