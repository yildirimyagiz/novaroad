/**
 * @file ir_builder.c
 * @brief IR Builder - Converts AST to IR
 */

#include "compiler/ir.h"
#include "compiler/ast.h"
#include "std/alloc.h"

typedef struct {
    nova_ir_module_t *module;
    uint32_t next_reg;
    uint32_t next_label;
} ir_builder_t;

static ir_builder_t *builder_create(nova_ir_module_t *module)
{
    ir_builder_t *builder = nova_alloc(sizeof(ir_builder_t));
    if (!builder) return NULL;
    
    builder->module = module;
    builder->next_reg = 0;
    builder->next_label = 0;
    
    return builder;
}

static uint32_t builder_new_reg(ir_builder_t *builder)
{
    return builder->next_reg++;
}

static nova_ir_instr_t *builder_emit(ir_builder_t *builder, nova_ir_opcode_t opcode,
                                     uint32_t dest, uint32_t src1, uint32_t src2)
{
    nova_ir_instr_t *instr = nova_alloc(sizeof(nova_ir_instr_t));
    if (!instr) return NULL;
    
    instr->opcode = opcode;
    instr->dest = dest;
    instr->src1 = src1;
    instr->src2 = src2;
    instr->next = NULL;
    
    nova_ir_add_instr(builder->module, instr);
    
    return instr;
}

static uint32_t builder_build_expr(ir_builder_t *builder, nova_ast_node_t *node);

static uint32_t builder_build_binop(ir_builder_t *builder, nova_ast_node_t *node)
{
    // Assume binary op has 2 children
    uint32_t left = builder_build_expr(builder, node->children);
    uint32_t right = builder_build_expr(builder, node->children->next);
    
    uint32_t result = builder_new_reg(builder);
    
    // Determine opcode based on operator (simplified)
    builder_emit(builder, IR_ADD, result, left, right);
    
    return result;
}

static uint32_t builder_build_expr(ir_builder_t *builder, nova_ast_node_t *node)
{
    if (!node) return 0;
    
    switch (node->type) {
        case AST_BINARY_OP:
            return builder_build_binop(builder, node);
            
        case AST_LITERAL:
            // Load constant
            return builder_new_reg(builder);
            
        default:
            return 0;
    }
}

void nova_ir_build_from_ast(nova_ir_module_t *module, nova_ast_node_t *ast)
{
    if (!module || !ast) return;
    
    ir_builder_t *builder = builder_create(module);
    if (!builder) return;
    
    // Traverse AST and generate IR
    builder_build_expr(builder, ast);
    
    nova_free(builder);
}
