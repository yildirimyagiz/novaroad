# Nova Compiler: Unit Algebra Code Generation Analysis

## Summary
The Nova compiler has **AST structures defined for unit algebra** (`EXPR_UNIT_LITERAL`, `TYPE_QTY`) but **unit literal code generation is NOT YET IMPLEMENTED** in the bytecode generator. The parser has infrastructure for parsing unit types, but the codegen lacks the corresponding case handlers.

---

## 1. AST Definitions for Unit Algebra

### File: `include/compiler/ast.h`

#### Expression Kind (Line 90)
```c
EXPR_UNIT_LITERAL,  // Unit literal: 5.kg, 9.81.m/s
```
**Location**: Line 90 in enum `nova_expr_kind_t`

#### Unit Literal Data Structure (Lines 141-145)
```c
struct {
    double value;      // for EXPR_UNIT_LITERAL
    char *unit;        // unit string (e.g., "kg", "m/s")
    void *dimension;   // nova_dimension_t pointer
} unit_literal;
```
**Location**: Lines 141-145 in `nova_expr_t` union

#### Type System for Quantities (Line 50)
```c
TYPE_QTY      // qty<T, dim> for unit algebra
```
**Location**: Line 50 in enum `nova_type_kind_t`

#### nova_type_qty Structure (Lines 69-73)
```c
struct {
    struct nova_type *inner_type;
    char *unit_expr;
    void *dimension;  // nova_dimension_t*
} qty;
```
**Location**: Lines 69-73 in `nova_type_t` union

#### Type Constructor (Line 320)
```c
nova_type_t *nova_type_qty(nova_type_t *base_type, const char *unit_expr);  // qty<T, dim>
```
**Location**: Line 320 - Function declaration for creating qty types

---

## 2. Codegen Missing Cases

### File: `src/compiler/codegen.c`

#### Switch Statement in generate_expression (Line 334)
**Location**: Lines 329-714 - `static bool generate_expression(nova_codegen_t *codegen, nova_expr_t *expr)`

#### Implemented Cases:
- **Line 335-340**: `EXPR_INT` → emits `OP_CONSTANT` with integer value
- **Line 342-346**: `EXPR_FLOAT` → emits `OP_CONSTANT` with double value
- **Line 348-353**: `EXPR_STR` → emits `OP_CONSTANT` with string value
- **Line 355-358**: `EXPR_BOOL` → emits `OP_TRUE` or `OP_FALSE`
- **Line 360-423**: `EXPR_CALL` 
- **Line 425-442**: `EXPR_IDENT`
- **Line 444-463**: `EXPR_ASSIGN`
- **Line 465-486**: `EXPR_ADDR_OF`
- **Line 488-497**: `EXPR_DEREF`
- **Line 499-567**: `EXPR_BINARY`
- **Line 569-577**: `EXPR_ARRAY_LIT`
- **Line 579-586**: `EXPR_INDEX`
- **Line 588-593**: `EXPR_STRING_LEN`
- **Line 595-609**: `EXPR_NAMESPACED_ACCESS`
- **Line 611-673**: `EXPR_MATCH`
- **Line 675-707**: `EXPR_ENUM_VARIANT`

#### **MISSING Cases** (Default at Line 709-711):
- `EXPR_UNIT_LITERAL` - **NOT IMPLEMENTED** (falls through to default)
- `EXPR_FIELD_ACCESS` - NOT IMPLEMENTED
- `EXPR_STRUCT_INIT` - NOT IMPLEMENTED
- `EXPR_STRING_SLICE` - NOT IMPLEMENTED
- `EXPR_STRING_CONCAT` - NOT IMPLEMENTED
- `EXPR_CAST` - NOT IMPLEMENTED
- `EXPR_HEAP_NEW` - NOT IMPLEMENTED

**Default Case (Lines 709-711)**:
```c
default:
    // fprintf(stderr, "CODEGEN: Unsupported expression kind %d\n", expr->kind);
    return false;
```

---

## 3. EXPR_INT and EXPR_FLOAT Code Generation

### EXPR_INT (Lines 335-340)
```c
case EXPR_INT:
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    int constant =
        chunk_add_constant(codegen->chunk, value_number((double) expr->data.lit_int));
    chunk_write(codegen->chunk, (uint8_t) constant, expr->span.line);
    break;
```

**Bytecode emitted**:
1. `OP_CONSTANT` (0x00) - Load constant opcode
2. Constant pool index (1 byte)

**Example**: Integer `42` → `[OP_CONSTANT, 0] [push 42.0 from constant pool]`

### EXPR_FLOAT (Lines 342-346)
```c
case EXPR_FLOAT:
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    constant = chunk_add_constant(codegen->chunk, value_number(expr->data.lit_float));
    chunk_write(codegen->chunk, (uint8_t) constant, expr->span.line);
    break;
```

**Bytecode emitted**:
1. `OP_CONSTANT` (0x00) - Load constant opcode
2. Constant pool index (1 byte)

**Example**: Float `3.14` → `[OP_CONSTANT, 0] [push 3.14 from constant pool]`

---

## 4. Available Bytecode Opcodes

### File: `include/backend/opcode.h`

#### Constants (0x00 - 0x0F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_CONSTANT` | 0x00 | Load constant from constant pool: `OP_CONSTANT <index>` |
| `OP_NULL` | 0x01 | Load null value |
| `OP_TRUE` | 0x02 | Load boolean true |
| `OP_FALSE` | 0x03 | Load boolean false |

#### Arithmetic (0x10 - 0x2F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_ADD` | 0x10 | Add two values: a + b |
| `OP_SUBTRACT` | 0x11 | Subtract: a - b |
| `OP_MULTIPLY` | 0x12 | Multiply: a * b |
| `OP_DIVIDE` | 0x13 | Divide: a / b |
| `OP_NEGATE` | 0x14 | Negate: -a |
| `OP_MODULO` | 0x15 | Modulo: a % b |

#### Comparison (0x30 - 0x3F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_EQUAL` | 0x30 | Equal: a == b |
| `OP_NOT_EQUAL` | 0x31 | Not equal: a != b |
| `OP_LESS` | 0x32 | Less than: a < b |
| `OP_LESS_EQUAL` | 0x33 | Less or equal: a <= b |
| `OP_GREATER` | 0x34 | Greater than: a > b |
| `OP_GREATER_EQUAL` | 0x35 | Greater or equal: a >= b |

#### Logical (0x40 - 0x4F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_NOT` | 0x40 | Logical NOT: !a |
| `OP_AND` | 0x41 | Logical AND: a && b |
| `OP_OR` | 0x42 | Logical OR: a \\|\\| b |

#### Control Flow (0x50 - 0x6F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_JUMP` | 0x50 | Jump to offset: pc += offset |
| `OP_JUMP_IF_FALSE` | 0x51 | Conditional jump if false |
| `OP_JUMP_IF_TRUE` | 0x52 | Conditional jump if true |
| `OP_LOOP` | 0x53 | Loop: pc -= offset (backward jump) |
| `OP_CALL` | 0x54 | Call function |
| `OP_RETURN` | 0x55 | Return from function |

#### Variables (0x70 - 0x8F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_DEFINE_GLOBAL` | 0x70 | Define global variable |
| `OP_GET_GLOBAL` | 0x71 | Get global variable |
| `OP_SET_GLOBAL` | 0x72 | Set global variable |
| `OP_GET_LOCAL` | 0x73 | Get local variable |
| `OP_SET_LOCAL` | 0x74 | Set local variable |
| `OP_GET_UPVALUE` | 0x75 | Get upvalue |
| `OP_SET_UPVALUE` | 0x76 | Set upvalue |

#### Closures (0x90 - 0x9F)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_CLOSURE` | 0x90 | Create closure from function |
| `OP_CLOSE_UPVALUE` | 0x91 | Close upvalue |

#### Classes/Objects (0xA0 - 0xBF)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_CLASS` | 0xA0 | Create class |
| `OP_GET_PROPERTY` | 0xA1 | Get property: obj.prop |
| `OP_SET_PROPERTY` | 0xA2 | Set property: obj.prop = value |
| `OP_GET_METHOD` | 0xA3 | Get method |
| `OP_INVOKE` | 0xA4 | Invoke method |
| `OP_INHERIT` | 0xA5 | Inherit from superclass |

#### Enum Operations (0xB0 - 0xBF)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_ENUM` | 0xB0 | Create enum variant: `OP_ENUM <tag> <field_count>` |
| `OP_GET_TAG` | 0xB1 | Get enum tag |
| `OP_GET_FIELD` | 0xB2 | Get enum field |

#### Memory & Stack (0xC0 - 0xCF)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_POP` | 0xC0 | Pop value from stack |
| `OP_DUP` | 0xC1 | Duplicate top of stack |
| `OP_PRINT` | 0xC2 | Print value (debugging) |

#### Pointer Operations (0xD0 - 0xDF)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_ADDR_OF` | 0xD0 | Address-of: push address of variable |
| `OP_DEREF` | 0xD1 | Dereference: load value from pointer |
| `OP_STORE_PTR` | 0xD2 | Store through pointer: *ptr = value |

#### Array Operations (0xE0 - 0xEF)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_ARRAY_LIT` | 0xE0 | Create array from stack |
| `OP_INDEX_GET` | 0xE1 | Index into array: push object[index] |
| `OP_INDEX_SET` | 0xE2 | Set array element |
| `OP_ARRAY_LEN` | 0xE3 | Get array length |

#### Special (0xF0 - 0xFF)
| Opcode | Value | Description |
|--------|-------|-------------|
| `OP_EOF` | 0xFF | End of bytecode marker |

---

## 5. Parser Unit Type Handling

### File: `src/compiler/parser.c`

#### Dimension Parsing (Lines 931-972)
**Location**: Around line 931 in type parsing

The parser has infrastructure to parse unit types in the form `qty<base_type, dimension>`:
```c
// Parse dimension expression (kg, m/s, kg·m/s², etc.)
// Collect all tokens until we hit '>'
char dim_buffer[256];
int dim_idx = 0;

// Read first unit identifier
if (!match(parser, TOKEN_IDENT)) {
    nova_type_free(scalar_type);
    return NULL; // Expected dimension name
}
```

**Supported syntax**:
- Single unit: `qty<f64, kg>`
- Division: `qty<f64, m/s>` (parses `/` operator)
- Multiplication: `qty<f64, kg·m>` (parses `*` operator)
- Powers: `m2` → `m^2` (parses superscript numbers)

#### Parser Token (Line 109)
```c
case TOKEN_KEYWORD_UNIT:
```
Parser recognizes `unit` as a keyword but context suggests it's for type parsing.

---

## 6. What's Missing for Unit Algebra Codegen

### No Bytecode Opcodes for Units
The `opcode.h` does NOT define:
- `OP_UNIT_LITERAL` - for creating unit values
- `OP_UNIT_CONVERT` - for unit conversions
- `OP_IN` - for unit conversions (e.g., `x in meters`)
- No unit-specific arithmetic operations

### No Code Generation for EXPR_UNIT_LITERAL
**codegen.c** line 709-711 returns `false` for unhandled expression types:
```c
default:
    // fprintf(stderr, "CODEGEN: Unsupported expression kind %d\n", expr->kind);
    return false;
```

### Implementation Strategy Needed
To implement unit algebra code generation, you would need:

1. **Define new opcodes** (in `include/backend/opcode.h`):
   ```c
   OP_UNIT_LITERAL = 0x16  // Create unit quantity: value + unit metadata
   OP_UNIT_CONVERT = 0x17  // Convert between unit systems
   OP_UNIT_SCALE = 0x18    // Apply scale factor
   ```

2. **Add codegen case** (in `src/compiler/codegen.c` line ~709):
   ```c
   case EXPR_UNIT_LITERAL: {
       // Generate value
       if (!generate_expression(codegen, /* numeric part */))
           return false;
       
       // Emit unit metadata
       chunk_write_opcode(codegen->chunk, OP_UNIT_LITERAL, expr->span.line);
       // Write unit string index and dimension
       break;
   }
   ```

3. **Runtime support** (in VM): Handle unit arithmetic and conversions at runtime

---

## File Summary Table

| File | Location | Status | Details |
|------|----------|--------|---------|
| `include/compiler/ast.h` | Lines 50, 69-73, 90, 141-145, 320 | ✅ Defined | AST nodes for `EXPR_UNIT_LITERAL`, `TYPE_QTY` |
| `src/compiler/codegen.c` | Lines 329-714 | ❌ Missing | No `EXPR_UNIT_LITERAL` case handler |
| `include/backend/opcode.h` | Lines 28-217 | ⚠️ Partial | No unit-specific opcodes (`OP_UNIT_*`) |
| `src/compiler/parser.c` | Lines ~931, 109 | ✅ Partial | Type parsing for `qty<T, dim>` |

---

## Grep Results Summary

### No EXPR_UNIT references in codegen.c:
```
$ grep -n 'EXPR_UNIT\|unit_lit\|unit_conv\|OP_UNIT' src/compiler/codegen.c
(no output - not implemented)
```

### Parser recognizes unit keyword:
```
$ grep -n 'EXPR_UNIT\|unit_lit\|unit_conv\|UNIT\|OP_UNIT' src/compiler/parser.c
109:    case TOKEN_KEYWORD_UNIT:
```

---

## Conclusion

The Nova compiler has the **AST infrastructure for unit algebra** but **code generation is not implemented**. To complete unit algebra support:

1. ✅ AST structures exist for `EXPR_UNIT_LITERAL` and `TYPE_QTY`
2. ✅ Parser can parse unit type syntax (`qty<f64, m/s>`)
3. ❌ **No bytecode opcodes for unit operations**
4. ❌ **No codegen case for `EXPR_UNIT_LITERAL`**
5. ❌ **No runtime VM support for unit semantics**

This is a placeholder for future implementation.
