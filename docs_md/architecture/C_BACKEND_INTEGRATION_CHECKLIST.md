# 🔧 C Backend Integration Checklist

**Date**: 2025-02-26  
**Task**: Integrate pattern matching into existing C codebase

---

## 📋 Integration Points

### 1. AST Integration (include/nova_ast.h)

**Current Status**: Check if EXPR_MATCH exists

```bash
grep "EXPR_MATCH" include/nova_ast.h
```

**Needed:**
- [ ] Add EXPR_MATCH to ExprKind enum
- [ ] Add MatchExpr to Expr union
- [ ] Include nova_pattern.h

### 2. Parser Integration (src/compiler/parser.c)

**Current Status**: Check parse_expression

```bash
grep "parse_match\|EXPR_MATCH" src/compiler/parser.c
```

**Needed:**
- [ ] Add match case to parse_primary_expression()
- [ ] Link parser_match.c functions
- [ ] Handle TOKEN_MATCH

### 3. Semantic Analysis (src/compiler/semantic.c)

**Current Status**: Check semantic_analyze_expr

```bash
grep "EXPR_MATCH\|match" src/compiler/semantic.c
```

**Needed:**
- [ ] Add EXPR_MATCH case to semantic_analyze_expr()
- [ ] Call match_analyze()
- [ ] Link pattern_semantic.c

### 4. Code Generation (src/compiler/codegen.c)

**Current Status**: Check generate_expression

```bash
grep "EXPR_MATCH\|generate_match" src/compiler/codegen.c
```

**Needed:**
- [ ] Add EXPR_MATCH case to generate_expression()
- [ ] Call codegen_match_expression()
- [ ] Link pattern_codegen.c

### 5. VM Integration (src/backend/vm.c)

**Current Status**: Check VM dispatch

```bash
grep "OP_DUP\|OP_PATTERN" src/backend/vm.c
```

**Needed:**
- [ ] Add pattern opcodes to VM dispatch
- [ ] Include opcode_pattern.h
- [ ] Call vm_execute_pattern_opcode()

### 6. Build System (CMakeLists.txt or Makefile)

**Needed:**
- [ ] Add pattern.c
- [ ] Add parser_match.c
- [ ] Add pattern_semantic.c
- [ ] Add exhaustiveness.c
- [ ] Add pattern_codegen.c
- [ ] Add vm_pattern.c

---

## 🔍 Quick Check Commands

```bash
# Check AST
grep -n "typedef enum.*ExprKind" include/nova_ast.h

# Check if pattern files are compiled
ls -la src/compiler/pattern*.c

# Check if VM has pattern opcodes
grep "case OP_DUP:" src/backend/vm.c

# Check build files
grep "pattern" CMakeLists.txt Makefile 2>/dev/null
```

---

## ✅ Action Plan

1. **Verify AST** - Add EXPR_MATCH if missing
2. **Link Parser** - Integrate parser_match.c
3. **Link Semantic** - Integrate pattern_semantic.c
4. **Link Codegen** - Integrate pattern_codegen.c
5. **Link VM** - Add pattern opcode handlers
6. **Update Build** - Add all new files to build system
7. **Test** - Compile and run tests

Let's check each one!
