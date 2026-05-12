#include <stdio.h>
#include <stdlib.h>

void nova_report_error(const char *msg, int code) {
  fprintf(stderr, "❌ Nova Runtime Error [%d]: %s\n", code, msg);
  exit(code);
}
