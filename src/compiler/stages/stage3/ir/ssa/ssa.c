#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../frontend/core/ast_mod/ast.h"
#include "../ir.h"

void ir_print_function(IRFunction *f) {
    printf("  [IR Viz] Function: %s\
", f->name);
    BasicBlock *bb = f->en
<truncated 8292 bytes>