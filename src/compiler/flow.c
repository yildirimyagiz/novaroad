#include "compiler/flow.h"
#include "compiler/ast.h"
#include <stdbool.h>
#include <stdlib.h>

nova_flow_context_t *nova_flow_context_create(void)
{
    nova_flow_context_t *ctx = (nova_flow_context_t *) malloc(sizeof(nova_flow_context_t));
    if (ctx) {
        ctx->dummy = 0;
    }
    return ctx;
}

void nova_flow_context_destroy(nova_flow_context_t *ctx)
{
    if (ctx) {
        free(ctx);
    }
}

int nova_flow_analyze_stmt(nova_flow_context_t *ctx, nova_stmt_t *stmt)
{
    (void) ctx;
    (void) stmt;
    return 0;
}

bool nova_flow_is_valid(nova_flow_context_t *ctx)
{
    (void) ctx;
    return true;
}
