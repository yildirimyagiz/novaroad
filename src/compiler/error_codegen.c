#include <stdbool.h>

/**
 * @file error_codegen.c
 * @brief Error handling code generation
 */

#include "compiler/codegen.h"
#include "compiler/error.h"
#include "backend/chunk.h"
#include "backend/opcode.h"

// ══════════════════════════════════════════════════════════════════════════════
// OPCODES FOR ERROR HANDLING
// ══════════════════════════════════════════════════════════════════════════════

#define OP_RESULT_OK    0x70  // Create Result::Ok(value)
#define OP_RESULT_ERR   0x71  // Create Result::Err(error)
#define OP_RESULT_CHECK 0x72  // Check if Result is Ok
#define OP_RESULT_UNWRAP 0x73 // Unwrap Result (panic on Err)
#define OP_TRY_BEGIN    0x74  // Begin try block
#define OP_TRY_END      0x75  // End try block
#define OP_CATCH        0x76  // Catch handler
#define OP_PANIC        0x77  // Panic (abort execution)

// ══════════════════════════════════════════════════════════════════════════════
// TRY/CATCH CODE GENERATION
// ══════════════════════════════════════════════════════════════════════════════

bool codegen_try_expression(Codegen *cg, TryExpr *try_expr, int line)
{
    Chunk *chunk = cg->chunk;
    
    // 1. Set up exception handler
    chunk_write_opcode(chunk, OP_TRY_BEGIN, line);
    int handler_offset_pos = chunk->count;
    chunk_write(chunk, 0xff, line);  // Placeholder for handler offset
    chunk_write(chunk, 0xff, line);
    
    // 2. Generate try block
    if (!generate_statement(cg, try_expr->try_block)) {
        return false;
    }
    
    // 3. End try block (no exception)
    chunk_write_opcode(chunk, OP_TRY_END, line);
    
    // Jump over catch handlers
    chunk_write_opcode(chunk, OP_JUMP, line);
    int skip_handlers_pos = chunk->count;
    chunk_write(chunk, 0xff, line);
    chunk_write(chunk, 0xff, line);
    
    // 4. Generate catch handlers
    int handler_start = chunk->count;
    
    // Patch handler offset
    chunk->code[handler_offset_pos] = (handler_start >> 8) & 0xff;
    chunk->code[handler_offset_pos + 1] = handler_start & 0xff;
    
    for (size_t i = 0; i < try_expr->handler_count; i++) {
        CatchHandler *handler = &try_expr->handlers[i];
        
        // Test if error matches handler type
        if (handler->error_type) {
            chunk_write_opcode(chunk, OP_DUP, line);  // Dup error
            // Load expected error type
            int type_const = chunk_add_constant(chunk, TYPE_VAL(handler->error_type));
            chunk_write_constant_long(chunk, type_const, line);
            chunk_write_opcode(chunk, OP_TYPE_CHECK, line);
            
            // Jump to next handler if not match
            chunk_write_opcode(chunk, OP_JUMP_IF_FALSE, line);
            int next_handler_pos = chunk->count;
            chunk_write(chunk, 0xff, line);
            chunk_write(chunk, 0xff, line);
        }
        
        // Pop test result
        chunk_write_opcode(chunk, OP_POP, line);
        
        // Bind error variable
        if (handler->error_var) {
            int slot = symbol_table_add_local(cg->symbols, handler->error_var);
            chunk_write_opcode(chunk, OP_SET_LOCAL, line);
            chunk_write(chunk, slot, line);
        } else {
            chunk_write_opcode(chunk, OP_POP, line);  // Discard error
        }
        
        // Generate handler body
        if (!generate_statement(cg, handler->handler)) {
            return false;
        }
        
        // Jump to end
        chunk_write_opcode(chunk, OP_JUMP, line);
        int end_jump_pos = chunk->count;
        chunk_write(chunk, 0xff, line);
        chunk_write(chunk, 0xff, line);
        
        // Patch next handler jump if exists
        if (handler->error_type && i < try_expr->handler_count - 1) {
            // Patch to next handler
        }
    }
    
    // 5. Generate finally block if present
    if (try_expr->finally_block) {
        if (!generate_statement(cg, try_expr->finally_block)) {
            return false;
        }
    }
    
    // Patch skip handlers jump
    int try_end = chunk->count;
    chunk->code[skip_handlers_pos] = (try_end >> 8) & 0xff;
    chunk->code[skip_handlers_pos + 1] = try_end & 0xff;
    
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// ERROR PROPAGATION (?) CODE GENERATION
// ══════════════════════════════════════════════════════════════════════════════

bool codegen_propagate_expression(Codegen *cg, PropagateExpr *prop, int line)
{
    Chunk *chunk = cg->chunk;
    
    // Desugar: expr? => match expr { Ok(v) => v, Err(e) => return Err(e) }
    
    // 1. Evaluate inner expression (Result<T, E>)
    if (!generate_expression(cg, prop->inner)) {
        return false;
    }
    
    // 2. Check if Result is Ok
    chunk_write_opcode(chunk, OP_DUP, line);
    chunk_write_opcode(chunk, OP_RESULT_CHECK, line);
    
    // 3. If Ok, unwrap and continue
    chunk_write_opcode(chunk, OP_JUMP_IF_FALSE, line);
    int err_branch_pos = chunk->count;
    chunk_write(chunk, 0xff, line);
    chunk_write(chunk, 0xff, line);
    
    // Ok branch: unwrap value
    chunk_write_opcode(chunk, OP_RESULT_UNWRAP, line);
    chunk_write_opcode(chunk, OP_JUMP, line);
    int continue_pos = chunk->count;
    chunk_write(chunk, 0xff, line);
    chunk_write(chunk, 0xff, line);
    
    // 4. If Err, propagate (early return)
    int err_branch = chunk->count;
    chunk->code[err_branch_pos] = (err_branch >> 8) & 0xff;
    chunk->code[err_branch_pos + 1] = err_branch & 0xff;
    
    // Extract error from Result::Err
    chunk_write_opcode(chunk, OP_RESULT_UNWRAP, line);  // Get error value
    
    // Wrap in Err and return
    chunk_write_opcode(chunk, OP_RESULT_ERR, line);
    chunk_write_opcode(chunk, OP_RETURN, line);
    
    // 5. Continue point
    int continue_offset = chunk->count;
    chunk->code[continue_pos] = (continue_offset >> 8) & 0xff;
    chunk->code[continue_pos + 1] = continue_offset & 0xff;
    
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// RESULT CONSTRUCTOR CODE GENERATION
// ══════════════════════════════════════════════════════════════════════════════

bool codegen_result_ok(Codegen *cg, Expr *value, int line)
{
    Chunk *chunk = cg->chunk;
    
    // Evaluate value
    if (!generate_expression(cg, value)) {
        return false;
    }
    
    // Wrap in Ok
    chunk_write_opcode(chunk, OP_RESULT_OK, line);
    
    return true;
}

bool codegen_result_err(Codegen *cg, Expr *error, int line)
{
    Chunk *chunk = cg->chunk;
    
    // Evaluate error
    if (!generate_expression(cg, error)) {
        return false;
    }
    
    // Wrap in Err
    chunk_write_opcode(chunk, OP_RESULT_ERR, line);
    
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// PANIC CODE GENERATION
// ══════════════════════════════════════════════════════════════════════════════

bool codegen_panic_expression(Codegen *cg, PanicExpr *panic, int line)
{
    Chunk *chunk = cg->chunk;
    
    // Evaluate message
    if (panic->message_expr) {
        if (!generate_expression(cg, panic->message_expr)) {
            return false;
        }
    } else {
        // Static message
        int msg_const = chunk_add_constant(chunk, STRING_VAL(panic->message));
        chunk_write_constant_long(chunk, msg_const, line);
    }
    
    // Emit panic opcode (aborts execution)
    chunk_write_opcode(chunk, OP_PANIC, line);
    
    return true;
}
