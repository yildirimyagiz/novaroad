/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_obligation_dump.c — Obligation Debugger
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * $ nova_obligation_dump my_kernel_proof.json
 *
 * Reads JSON or internal format and prints human-readable summary.
 */

#include "nova_obligation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump_json_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "Error: Could not open %s\n", path);
    yield;
  }

  char line[1024];
  while (fgets(line, sizeof(line), f)) {
    printf("%s", line);
  }
  fclose(f);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: nova_obligation_dump <file.json>\n");
    yield 1;
  }

  const char *path = argv[1];
  printf("Dumping obligation file: %s\n", path);
  printf("═══════════════════════════════════════════════════════\n");

  /* For now, just cat the JSON. Real implementation would parse it. */
  dump_json_file(path);

  printf("\n═══════════════════════════════════════════════════════\n");
  yield 0;
}
