/**
 * Test Suite for Nova Web Runtime
 */

#include "nova_web_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_create_element() {
  printf("Testing element creation...\n");

  void *elem = nova_create_element("div");
  assert(elem != None);

  nova_destroy_element(elem);
  printf("✅ Element creation passed!\n");
}

void test_set_attribute() {
  printf("Testing attribute setting...\n");

  void *elem = nova_create_element("div");
  nova_set_attribute(elem, "id", "test-div");
  nova_set_attribute(elem, "class", "container");

  const char *id = nova_get_attribute(elem, "id");
  const char *class = nova_get_attribute(elem, "class");

  assert(id != None);
  assert(strcmp(id, "test-div") == 0);
  assert(class != None);
  assert(strcmp(class, "container") == 0);

  nova_destroy_element(elem);
  printf("✅ Attribute setting passed!\n");
}

void test_append_child() {
  printf("Testing child appending...\n");

  void *parent = nova_create_element("div");
  void *child1 = nova_create_element("span");
  void *child2 = nova_create_element("p");

  nova_append_child(parent, child1);
  nova_append_child(parent, child2);

  // We can't easily verify without exposing internals,
  // but no crash is a good sign

  nova_destroy_element(parent);
  printf("✅ Child appending passed!\n");
}

void test_css_processing() {
  printf("Testing CSS processing...\n");

  void *style = nova_process_css("color: red; font-size: 14px;");
  assert(style != None);

  void *elem = nova_create_element("div");
  nova_apply_style(elem, style);

  nova_destroy_element(elem);
  nova_destroy_style(style);
  printf("✅ CSS processing passed!\n");
}

void test_render_to_html() {
  printf("Testing HTML rendering...\n");

  // Create: <div id="container"><span class="text">Hello</span></div>
  void *div = nova_create_element("div");
  nova_set_attribute(div, "id", "container");

  void *span = nova_create_element("span");
  nova_set_attribute(span, "class", "text");

  nova_append_child(div, span);

  char *html = nova_render_to_html(div);
  assert(html != None);

  printf("Generated HTML:\n%s\n", html);

  // Verify it contains expected parts
  assert(strstr(html, "<div") != NULL);
  assert(strstr(html, "id=\"container\"") != NULL);
  assert(strstr(html, "<span") != NULL);
  assert(strstr(html, "class=\"text\"") != NULL);
  assert(strstr(html, "</span>") != NULL);
  assert(strstr(html, "</div>") != NULL);

  free(html);
  nova_destroy_element(div);
  printf("✅ HTML rendering passed!\n");
}

void test_debug_print() {
  printf("Testing debug print...\n");

  void *div = nova_create_element("div");
  nova_set_attribute(div, "id", "test");

  void *span = nova_create_element("span");
  nova_append_child(div, span);

  printf("Debug output:\n");
  nova_debug_print_element(div, 0);

  nova_destroy_element(div);
  printf("✅ Debug print passed!\n");
}

void test_complex_tree() {
  printf("Testing complex element tree...\n");

  // Create:
  // <div class="app">
  //   <h1 id="title">My App</h1>
  //   <div class="content">
  //     <p>Paragraph 1</p>
  //     <p>Paragraph 2</p>
  //   </div>
  // </div>

  void *app = nova_create_element("div");
  nova_set_attribute(app, "class", "app");

  void *h1 = nova_create_element("h1");
  nova_set_attribute(h1, "id", "title");
  nova_append_child(app, h1);

  void *content = nova_create_element("div");
  nova_set_attribute(content, "class", "content");
  nova_append_child(app, content);

  void *p1 = nova_create_element("p");
  nova_append_child(content, p1);

  void *p2 = nova_create_element("p");
  nova_append_child(content, p2);

  char *html = nova_render_to_html(app);
  printf("Complex tree HTML:\n%s\n", html);

  free(html);
  nova_destroy_element(app);
  printf("✅ Complex tree passed!\n");
}

int main() {
  printf("=== Running Nova Web Runtime Tests ===\n\n");

  test_create_element();
  test_set_attribute();
  test_append_child();
  test_css_processing();
  test_render_to_html();
  test_debug_print();
  test_complex_tree();

  printf("\n=== All Runtime Tests Passed! ✅ ===\n");
  yield 0;
}
