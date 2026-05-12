# 🎯 Pattern Matching Implementation - 1 Week Plan

**Start Date**: 2025-02-26  
**Duration**: 7 days  
**Status**: In Progress

---

## 📋 Implementation Checklist

### Day 1-2: AST & Parser Integration
- [ ] Add `EXPR_MATCH` to AST node types
- [ ] Define Pattern AST nodes
- [ ] Define MatchArm structure
- [ ] Add match keyword parsing to parser
- [ ] Add pattern parsing (literal, wildcard, binding, tuple, variant)
- [ ] Add guard expression parsing
- [ ] Unit tests for parsing

### Day 3-4: Semantic Analysis
- [ ] Pattern type checking
- [ ] Exhaustiveness checking
- [ ] Reachability analysis (unreachable patterns)
- [ ] Pattern binding to symbol table
- [ ] Type refinement for patterns
- [ ] Unit tests for semantics

### Day 5-6: Code Generation
- [ ] Match expression bytecode generation
- [ ] Pattern test code generation
- [ ] Pattern binding code generation
- [ ] Jump table optimization
- [ ] Add new opcodes to VM
- [ ] Integration tests

### Day 7: Testing & Polish
- [ ] Comprehensive test suite
- [ ] Edge cases
- [ ] Performance tests
- [ ] Documentation
- [ ] Examples

---

## 📁 Files to Modify/Create

### AST Changes
```c
// include/nova_ast.h
typedef enum {
    // ... existing ...
    EXPR_MATCH,
} ExprKind;

typedef struct {
    Expr *scrutinee;
    MatchArm *arms;
    size_t arm_count;
} MatchExpr;

typedef struct {
    Pattern *pattern;
    Expr *guard;      // Optional
    Expr *body;
} MatchArm;

typedef enum {
    PATTERN_WILDCARD,
    PATTERN_LITERAL,
    PATTERN_BINDING,
    PATTERN_TUPLE,
    PATTERN_VARIANT,
    PATTERN_OR,
} PatternKind;
```

### Parser Changes
```c
// src/compiler/parser.c
Expr *parse_match_expr();
Pattern *parse_pattern();
MatchArm *parse_match_arm();
```

### Codegen Changes
```c
// src/compiler/codegen.c
bool generate_match(nova_codegen_t *codegen, MatchExpr *expr);
bool generate_pattern_test(nova_codegen_t *codegen, Pattern *pattern);
bool generate_pattern_bindings(nova_codegen_t *codegen, Pattern *pattern);
```

### VM Changes
```c
// include/backend/opcode.h
#define OP_DUP          0x20
#define OP_PATTERN_EQ   0x50
```

---

## 🔧 Implementation Steps

### Step 1: AST Definition
Create complete AST nodes for match expressions and patterns.

### Step 2: Parser Integration
Add match expression parsing to main expression parser.

### Step 3: Semantic Analysis
- Type check patterns against scrutinee type
- Check exhaustiveness
- Detect unreachable patterns

### Step 4: Codegen
Generate bytecode for:
```
LOAD scrutinee
ARM1:
  DUP
  TEST_PATTERN1
  JUMP_IF_FALSE ARM2
  BIND_VARS
  EVAL_BODY
  JUMP END
ARM2:
  DUP
  TEST_PATTERN2
  ...
END:
  POP
```

### Step 5: VM Support
Add opcodes for pattern testing and execution.

---

## 📝 Test Cases

### Basic Tests
```nova
match x {
    0 => "zero",
    1 => "one",
    _ => "other",
}
```

### Tuple Patterns
```nova
match point {
    (0, 0) => "origin",
    (x, 0) => "x-axis",
    (0, y) => "y-axis",
    (x, y) => "general",
}
```

### Variant Patterns
```nova
match opt {
    Some(x) => x,
    None => 0,
}
```

### Guard Expressions
```nova
match x {
    n if n < 0 => "negative",
    n if n == 0 => "zero",
    _ => "positive",
}
```

### Or Patterns
```nova
match x {
    1 | 3 | 5 | 7 | 9 => "odd",
    0 | 2 | 4 | 6 | 8 => "even",
    _ => "other",
}
```

### Nested Patterns
```nova
match result {
    Ok(Some(x)) => x,
    Ok(None) => 0,
    Err(_) => -1,
}
```

---

## 🎯 Success Criteria

- [ ] All basic patterns work (literal, wildcard, binding)
- [ ] Tuple patterns work
- [ ] Variant patterns work (Option, Result)
- [ ] Guard expressions work
- [ ] Or patterns work
- [ ] Exhaustiveness checking works
- [ ] Unreachable pattern detection works
- [ ] Performance is acceptable
- [ ] All tests pass

---

## 📊 Progress Tracking

| Day | Task | Status | Notes |
|-----|------|--------|-------|
| 1 | AST definition | ⏳ | Starting now |
| 2 | Parser integration | ⏳ | |
| 3 | Semantic analysis | ⏳ | |
| 4 | Exhaustiveness | ⏳ | |
| 5 | Codegen | ⏳ | |
| 6 | VM support | ⏳ | |
| 7 | Testing | ⏳ | |

---

## 🚀 Let's Start!

**Current Phase**: Day 1 - AST Definition

Ready to implement? Let's do this! 💪
