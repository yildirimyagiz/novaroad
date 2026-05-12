#ifndef NOVA_FLOW_H
#define NOVA_FLOW_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Flow analysis context
typedef struct nova_flow_context {
    int dummy;
} nova_flow_context_t;

nova_flow_context_t *nova_flow_context_create(void);
void nova_flow_context_destroy(nova_flow_context_t *ctx);

/** Analyze information flow for a statement */
int nova_flow_analyze_stmt(nova_flow_context_t *ctx, nova_stmt_t *stmt);

/** Check if a flow is valid */
bool nova_flow_is_valid(nova_flow_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_FLOW_H */
