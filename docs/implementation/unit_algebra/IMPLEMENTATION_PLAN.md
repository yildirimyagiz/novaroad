# 🌟 Unit Algebra Implementation Plan

**Feature**: Compile-time dimensional analysis  
**Status**: In Progress  
**Priority**: P1 (UNIQUE feature!)

---

## 📋 Overview

Unit algebra allows compile-time checking of physical dimensions:

```nova
let mass: qty<f64, kg> = 10.kg;
let accel: qty<f64, m/s²> = 9.81.m/s²;
let force = mass * accel;  // ✓ Type checks to qty<f64, N>

let invalid = 5.kg + 3.m;  // ❌ COMPILE ERROR!
```

---

## 🎯 Implementation Phases

### Phase 1: Type System ✅ DONE
- [x] Add TYPE_QTY to TypeKind
- [x] Add QtyTypeData struct
- [x] Add DimensionExpr struct
- [x] Function declarations

### Phase 2: AST & Parsing ⏳ IN PROGRESS
- [ ] Add AST nodes (QtyTypeNode, QtyLiteralNode)
- [ ] Parse qty<T, dim> syntax
- [ ] Parse unit literals (5.kg)
- [ ] Parse dimension expressions (m/s²)

### Phase 3: Semantic Analysis
- [ ] Type checking for qty types
- [ ] Dimensional arithmetic rules
- [ ] Dimension compatibility checking
- [ ] Error messages

### Phase 4: Code Generation
- [ ] Generate bytecode (zero-cost!)
- [ ] Runtime representation (just T)
- [ ] Test with test_unit_algebra.zn

---

## 📁 Related Files

- **Type System**: `include/nova_types.h` ✅
- **Dimensions**: `include/compiler/dimensions.h` ✅
- **Dimensions Impl**: `src/compiler/dimensions.c` ✅
- **AST**: `include/nova_ast.h` ⏳
- **Parser**: `src/compiler/parser.c` ⏳
- **Semantic**: `src/compiler/semantic.c` ⏳
- **Codegen**: `src/compiler/codegen.c` ⏳
- **Tests**: `tests/unit/zn/test_unit_algebra.zn` ✅

---

## 🔗 See Also

- Main TODO: `../TODO.md`
- Backend Plan: `/Users/yldyagz/novaRoad/nova/UNIT_ALGEBRA_BACKEND_PLAN.md`
- Type Mapping: `/Users/yldyagz/novaRoad/nova/ZN_TO_C_TYPE_MAPPING.md`
