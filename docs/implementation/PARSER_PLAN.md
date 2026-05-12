# 🚀 Parser Integration Plan

**Date**: 2026-02-26  
**Status**: Phase 1 Complete, Phase 2 Ready to Start  
**Estimated Time**: 4-6 hours

---

## 📋 Current State

### ✅ Foundation Complete
- Type system (nova_types.h) ✅
- AST nodes (nova_ast.h) ✅
- Type creators (ast.c) ✅
- AST constructors (ast.c) ✅
- Tokens (lexer.h, lexer.c) ✅

### 📁 Parser File
- **File**: `src/compiler/parser.c`
- **Size**: 2,557 lines
- **Functions**: parse_type(), parse_expression(), parse_function_declaration()

---

## 🎯 Implementation Tasks

### Task 1: Parse Qty Types (1-2 hours)
**Function**: `parse_type()` at line 164

**Add to parse_type()**:
```c
// After existing type parsing...
case TOKEN_KEYWORD_QTY: {
    // Parse: qty<T, dim>
    advance(parser);  // consume 'qty'
    expect(parser, TOKEN_LT);
    
    ASTNode *base_type = parse_type(parser);
    expect(parser, TOKEN_COMMA);
    
    DimensionExpr *dim = parse_dimension_expr(parser);
    expect(parser, TOKEN_GT);
    
    return ast_create_qty_type(base_type, dim);
}
```

**New Helper Function**:
```c
static DimensionExpr *parse_dimension_expr(nova_parser_t *parser) {
    // Parse: kg, m/s², kg·m/s²
    // For MVP: Just parse identifier and map to dimension
    
    if (match(parser, TOKEN_IDENT)) {
        const char *unit = parser->previous.start;
        
        // Simple mapping (expand later)
        if (strcmp(unit, "kg") == 0) {
            return dim_create_base(1, 0, 0, 0, 0, 0, 0);  // Mass
        } else if (strcmp(unit, "m") == 0) {
            return dim_create_base(0, 1, 0, 0, 0, 0, 0);  // Length
        } else if (strcmp(unit, "s") == 0) {
            return dim_create_base(0, 0, 1, 0, 0, 0, 0);  // Time
        }
        // ... more units
    }
    
    error(parser, "Expected dimension expression");
    return NULL;
}
```

---

### Task 2: Parse Unit Literals (1 hour)
**Function**: `parse_primary()` or `parse_expression()`

**Add to expression parsing**:
```c
case TOKEN_LIT_UNIT: {
    // Parse: 5.kg, 9.81.m/s²
    // Lexer should recognize pattern: NUMBER DOT IDENTIFIER
    
    double value = parser->previous.value.floating;
    DimensionExpr *dim = parse_dimension_expr(parser);
    
    return ast_create_qty_literal(value, dim);
}
```

**Note**: May need lexer enhancement to recognize `5.kg` pattern

---

### Task 3: Parse Async Functions (1 hour)
**Function**: `parse_function_declaration()`

**Modify function parsing**:
```c
static nova_stmt_t *parse_function_declaration(nova_parser_t *parser) {
    bool is_async = false;
    
    // Check for 'async' modifier
    if (match(parser, TOKEN_KEYWORD_ASYNC)) {
        is_async = true;
    }
    
    expect(parser, TOKEN_KEYWORD_FN);
    
    // ... rest of function parsing
    
    if (is_async) {
        // Wrap return type in Task<T>
        return_type = ast_create_flow_type(FLOW_TASK, return_type);
    }
    
    return ast_create_async_fn(name, params, param_count, return_type, body);
}
```

---

### Task 4: Parse Await Expressions (30 min)
**Function**: `parse_expression()`

**Add to expression parsing**:
```c
case TOKEN_KEYWORD_AWAIT: {
    advance(parser);  // consume 'await'
    
    ASTNode *expr = parse_expression(parser);
    return ast_create_await_expr(expr);
}
```

---

### Task 5: Parse Flow Types (30 min)
**Function**: `parse_type()`

**Add to parse_type()**:
```c
case TOKEN_KEYWORD_FLOW: {
    // Parse: flow<Signal<T>>, flow<Stream<T>>
    advance(parser);
    expect(parser, TOKEN_LT);
    
    // Determine flow kind from next token or type
    FlowTypeKind kind = FLOW_SIGNAL;  // Default
    
    ASTNode *inner = parse_type(parser);
    expect(parser, TOKEN_GT);
    
    return ast_create_flow_type(kind, inner);
}
```

---

### Task 6: Parse Effect Types (30 min)
**Function**: `parse_type()`

**Add to parse_type()**:
```c
case TOKEN_KEYWORD_EFFECT: {
    // Parse: effect<IO<T>>, effect<Async<T>>
    advance(parser);
    expect(parser, TOKEN_LT);
    
    // Determine effect kind
    EffectTypeKind kind = EFFECT_IO;  // Default
    
    ASTNode *inner = parse_type(parser);
    expect(parser, TOKEN_GT);
    
    return ast_create_effect_type(kind, inner);
}
```

---

## 📊 Estimated Timeline

| Task | Time | Difficulty |
|------|------|------------|
| 1. Parse Qty Types | 1-2h | Medium |
| 2. Parse Unit Literals | 1h | Medium |
| 3. Parse Async Functions | 1h | Easy |
| 4. Parse Await Expressions | 30m | Easy |
| 5. Parse Flow Types | 30m | Easy |
| 6. Parse Effect Types | 30m | Easy |
| **Total** | **4-6h** | |

---

## 🎯 Testing Strategy

### Test 1: Qty Types
```nova
let mass: qty<f64, kg> = 10.kg;
```
Should parse without errors.

### Test 2: Async
```nova
async fn fetch() -> Data {
    await http_get("url")
}
```
Should parse as async function with await expression.

### Test 3: Flow Types
```nova
let signal: flow<Signal<i64>> = ...;
```
Should parse flow type.

---

## 🚧 Known Challenges

### 1. Unit Literal Lexing
Current lexer may not recognize `5.kg` as a single token.

**Solutions**:
- A) Modify lexer to recognize pattern
- B) Parse as `5.0` DOT `kg` and combine in parser

**Recommendation**: Use solution B (easier)

### 2. Dimension Expression Parsing
Need to parse complex expressions like `m/s²`, `kg·m/s²`.

**Solutions**:
- A) Full expression parser (complex)
- B) Simple identifier lookup for MVP (easy)

**Recommendation**: Use solution B for MVP

### 3. Parser.c Size
File is 2,557 lines, complex codebase.

**Solutions**:
- Careful incremental changes
- Test after each addition
- Don't break existing functionality

---

## 💡 Implementation Strategy

### Incremental Approach
1. Start with simplest (Task 4: await)
2. Move to async functions (Task 3)
3. Then qty types (Task 1)
4. Finally unit literals (Task 2)

### Testing
- Add one feature at a time
- Test compilation after each
- Verify AST generation

---

## 🔗 Files to Modify

```
src/compiler/parser.c       (main work)
  - parse_type()           (+40 lines for qty/flow/effect)
  - parse_expression()     (+10 lines for await)
  - parse_function_declaration() (+20 lines for async)
  - parse_dimension_expr() (+30 lines new function)

Total estimated: ~100 lines of parser code
```

---

## 📝 Next Steps

**For Next Session**:

1. **Start with Await** (easiest)
   - Add case to parse_expression()
   - Test: `await expr`

2. **Add Async Functions**
   - Modify parse_function_declaration()
   - Test: `async fn foo() { ... }`

3. **Add Qty Types**
   - Add case to parse_type()
   - Add parse_dimension_expr()
   - Test: `qty<f64, kg>`

4. **Add Unit Literals** (if time)
   - Modify expression parsing
   - Test: `5.kg`

**Estimated time for first session**: 2-3 hours

---

**Ready to start when you are!** 🚀
