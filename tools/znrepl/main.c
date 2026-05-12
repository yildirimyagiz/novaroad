#include "../../include/backend/bytecode.h"
#include "../../include/backend/chunk.h"
#include "../../include/backend/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Nova REPL/VM v0.1.0\n");
    printf("Usage: znrepl <file.nbc> [-d]\n");
    return 0;
  }

  bool disassemble = false;
  const char *filename = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0)
      disassemble = true;
    else
      filename = argv[i];
  }

  if (!filename) {
    fprintf(stderr, "Error: No input file specified\n");
    return 1;
  }

  Chunk chunk;
  chunk_init(&chunk);
  if (!chunk_load(&chunk, filename)) {
    fprintf(stderr, "Error: Could not load bytecode from %s\n", filename);
    return 1;
  }

  if (disassemble) {
    bytecode_disassemble_chunk(&chunk, filename);
  }

  nova_vm_t *vm = nova_vm_create();
  NovaInterpretResult result = nova_vm_interpret(vm, &chunk);

  if (result != INTERPRET_OK) {
    fprintf(stderr, "Runtime Error: %s\n", nova_vm_get_error(vm));
  }

  nova_vm_destroy(vm);
  chunk_free(&chunk);

  return (result == INTERPRET_OK) ? 0 : 1;
}
