// ╔═══════════════════════════════════════════════════════════════════════════════╗
// ║ Nova Custom MLIR Dialect - TableGen Definition                              ║
// ║ IR-First Design: All advanced type system features as MLIR operations          ║
// ╚═══════════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_DIALECT
#define NOVA_DIALECT

include "mlir/IR/OpBase.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"

// ============================================================================
// NOVA DIALECT DEFINITION
// ============================================================================

def Nova_Dialect : Dialect {
  let name = "nova";
  let summary = "Nova high-level language dialect";
  let description = [{
    The Nova dialect represents high-level constructs from the Nova
    programming language, including:
    - Linear types and affine resource management
    - Effect system with proof levels
    - Ownership and borrowing semantics
    - Lifetime and region analysis
  }];
  let cppNamespace = "::mlir::nova";
}

// ============================================================================
// BASE CLASSES
// ============================================================================

class Nova_Op<string mnemonic, list<Trait> traits = []> :
    Op<Nova_Dialect, mnemonic, traits>;

class Nova_Type<string name, string typeMnemonic> : TypeDef<Nova_Dialect, name> {
  let mnemonic = typeMnemonic;
}

// ============================================================================
// NOVA TYPES WITH VERIFICATION
// ============================================================================

// Linear Type: Must be used exactly once
def LinearType : Nova_Type<"Linear", "linear"> {
  let summary = "Linear type that must be used exactly once";
  let description = [{
    Represents a value that must be consumed exactly once.
    Violations are caught at compile-time via MLIR analysis passes.
  }];
  let parameters = (ins "Type":$baseType);
  let assemblyFormat = "`<` $baseType `>`";
}

// Affine Type: Can be used 0 or more times (duplicable, droppable)
def AffineType : Nova_Type<"Affine", "affine"> {
  let summary = "Affine type that can be freely copied or dropped";
  let parameters = (ins "Type":$baseType);
  let assemblyFormat = "`<` $baseType `>`";
}

// Borrowed Reference Type
def BorrowedType : Nova_Type<"Borrowed", "borrowed"> {
  let summary = "Borrowed reference with lifetime";
  let parameters = (ins 
    "Type":$targetType,
    "StrAttr":$lifetime,
    "BoolAttr":$isMutable
  );
  let assemblyFormat = "`<` $targetType `,` $lifetime `,` $isMutable `>`";
}

// Owned Type: Full ownership semantics
def OwnedType : Nova_Type<"Owned", "owned"> {
  let summary = "Owned value with move semantics";
  let parameters = (ins "Type":$baseType);
  let assemblyFormat = "`<` $baseType `>`";
}

// Effect-Annotated Function Type
def EffectFuncType : Nova_Type<"EffectFunc", "effect_func"> {
  let summary = "Function type with effect annotations";
  let parameters = (ins
    ArrayRefParameter<"Type">:$inputs,
    ArrayRefParameter<"Type">:$outputs,
    "Attribute":$effects
  );
}

// ============================================================================
// ATTRIBUTES
// ============================================================================

def EffectSetAttr : AttrDef<Nova_Dialect, "EffectSet"> {
  let mnemonic = "effects";
  let parameters = (ins
    "uint64_t":$effectBits  // Bitfield: IO, Memory, Throw, Async, Unsafe
  );
  let assemblyFormat = "`<` $effectBits `>`";
}

def ProofLevelAttr : AttrDef<Nova_Dialect, "ProofLevel"> {
  let mnemonic = "proof";
  let parameters = (ins
    "uint32_t":$level
  );
  let assemblyFormat = "`<` $level `>`";
}

// ============================================================================
// OWNERSHIP OPERATIONS
// ============================================================================

def Nova_MoveOp : Nova_Op<"move", [Pure]> {
  let summary = "Transfer ownership of a value";
  let description = [{
    Moves ownership from source to destination.
    Source becomes invalid after this operation.
  }];
  
  let arguments = (ins AnyType:$source);
  let results = (outs AnyType:$dest);
  
  let assemblyFormat = [{
    $source attr-dict `:` type($source) `->` type($dest)
  }];
}

def Nova_BorrowOp : Nova_Op<"borrow", [Pure]> {
  let summary = "Create a borrowed reference";
  let description = [{
    Creates a shared or mutable borrow of a value.
    Enforces borrowing rules via lifetime analysis.
  }];
  
  let arguments = (ins 
    AnyType:$value,
    BoolAttr:$isMutable,
    StrAttr:$lifetime
  );
  let results = (outs BorrowedType:$reference);
  
  let assemblyFormat = [{
    $value `[` $lifetime `]` attr-dict `:` type($value) `->` type($reference)
  }];
}

def Nova_DropOp : Nova_Op<"drop", []> {
  let summary = "Explicitly drop a value (call destructor)";
  let arguments = (ins AnyType:$value);
  let assemblyFormat = "$value attr-dict `:` type($value)";
}

// ============================================================================
// EFFECT SYSTEM OPERATIONS
// ============================================================================

def Nova_WithEffectOp : Nova_Op<"with_effect", []> {
  let summary = "Execute region with specified effects";
  let description = [{
    Demarcates a region that has specific side effects.
    Used for effect tracking and optimization.
  }];
  
  let arguments = (ins EffectSetAttr:$effects);
  let regions = (region SizedRegion<1>:$body);
  
  let assemblyFormat = [{
    $effects $body attr-dict
  }];
}

def Nova_PureCallOp : Nova_Op<"pure_call", [Pure]> {
  let summary = "Call a pure function (no side effects)";
  let arguments = (ins 
    FlatSymbolRefAttr:$callee,
    Variadic<AnyType>:$operands
  );
  let results = (outs Variadic<AnyType>:$results);
  
  let assemblyFormat = [{
    $callee `(` $operands `)` attr-dict `:` functional-type($operands, $results)
  }];
}

def Nova_EffectCallOp : Nova_Op<"effect_call", []> {
  let summary = "Call a function with effects";
  let arguments = (ins 
    FlatSymbolRefAttr:$callee,
    EffectSetAttr:$effects,
    Variadic<AnyType>:$operands
  );
  let results = (outs Variadic<AnyType>:$results);
  
  let assemblyFormat = [{
    $callee `(` $operands `)` `effects` $effects attr-dict `:` functional-type($operands, $results)
  }];
}

// ============================================================================
// LINEAR TYPE OPERATIONS
// ============================================================================

def Nova_LinearAllocOp : Nova_Op<"linear.alloc", []> {
  let summary = "Allocate a linear resource";
  let results = (outs LinearType:$resource);
  let assemblyFormat = "attr-dict `:` type($resource)";
}

def Nova_LinearConsumeOp : Nova_Op<"linear.consume", []> {
  let summary = "Consume a linear resource (use exactly once)";
  let arguments = (ins LinearType:$resource);
  let assemblyFormat = "$resource attr-dict `:` type($resource)";
}

// ============================================================================
// MATHEMATICAL OPERATIONS (with specific types)
// ============================================================================

def Nova_DiffOp : Nova_Op<"diff", [Pure]> {
  let summary = "Compute derivative of an expression";
  let arguments = (ins 
    AnyFloat:$expr,  // Mathematical expressions are typically floating point
    Optional<AnyFloat>:$var
  );
  let results = (outs AnyFloat:$result);
  let assemblyFormat = "diff $expr (`,` $var^)? attr-dict `:` type($expr) `->` type($result)";
}

def Nova_IntegralOp : Nova_Op<"integral", [Pure]> {
  let summary = "Compute definite or indefinite integral";
  let arguments = (ins 
    AnyFloat:$expr,
    Optional<AnyFloat>:$var,
    Optional<AnyFloat>:$lower,
    Optional<AnyFloat>:$upper
  );
  let results = (outs AnyFloat:$result);
  let assemblyFormat = "integral $expr (`,` $var^)? (`from` $lower^ `to` $upper^)? attr-dict `:` type($expr) `->` type($result)";
}

def Nova_SumOp : Nova_Op<"sum", [Pure]> {
  let summary = "Compute summation over a range";
  let arguments = (ins 
    AnyFloat:$expr,
    AnyFloat:$var,
    AnyFloat:$lower,
    AnyFloat:$upper
  );
  let results = (outs AnyFloat:$result);
  let assemblyFormat = "sum $expr `,` $var `from` $lower `to` $upper attr-dict `:` type($expr) `->` type($result)";
}

def Nova_ProdOp : Nova_Op<"prod", [Pure]> {
  let summary = "Compute product over a range";
  let arguments = (ins 
    AnyFloat:$expr,
    AnyFloat:$var,
    AnyFloat:$lower,
    AnyFloat:$upper
  );
  let results = (outs AnyFloat:$result);
  let assemblyFormat = "prod $expr `,` $var `from` $lower `to` $upper attr-dict `:` type($expr) `->` type($result)";
}

def Nova_LimitOp : Nova_Op<"limit", [Pure]> {
  let summary = "Compute limit of an expression";
  let arguments = (ins 
    AnyFloat:$expr,
    AnyFloat:$var,
    AnyFloat:$point
  );
  let results = (outs AnyFloat:$result);
  let assemblyFormat = "limit $expr `as` $var `->` $point attr-dict `:` type($expr) `->` type($result)";
}

def Nova_SolveOp : Nova_Op<"solve", [Pure]> {
  let summary = "Solve equation or system of equations";
  let arguments = (ins 
    Variadic<AnyFloat>:$equations,  // Equations are mathematical expressions
    Variadic<AnyFloat>:$variables   // Variables to solve for
  );
  let results = (outs Variadic<AnyFloat>:$solutions);
  let assemblyFormat = "solve `(` $equations `)` `for` `(` $variables `)` attr-dict `:` functional-type($equations, $solutions)";
}

// ============================================================================
// AI/ML OPERATIONS
// ============================================================================

def Nova_InferOp : Nova_Op<"infer", []> {
  let summary = "Run inference on a machine learning model";
  let arguments = (ins 
    FlatSymbolRefAttr:$model,    // model symbol reference
    AnyType:$input               // input data
  );
  let results = (outs AnyType:$output);
  let assemblyFormat = "infer $model `(` $input `)` attr-dict `:` type($input) `->` type($output)";
}

def Nova_ModelCallOp : Nova_Op<"model_call", []> {
  let summary = "Call a method on a machine learning model";
  let arguments = (ins 
    FlatSymbolRefAttr:$model,     // model symbol reference
    StrAttr:$method,             // method name
    Variadic<AnyType>:$args      // method arguments
  );
  let results = (outs Variadic<AnyType>:$results);
  let assemblyFormat = "model_call $model `.` $method `(` $args `)` attr-dict `:` functional-type($args, $results)";
}

// ============================================================================
// TENSOR OPERATIONS
// ============================================================================

def Nova_TensorIndexOp : Nova_Op<"tensor.index", [Pure]> {
  let summary = "Index into a tensor";
  let arguments = (ins 
    AnyType:$tensor,
    Variadic<AnyType>:$indices
  );
  let results = (outs AnyType:$element);
  let assemblyFormat = "tensor.index $tensor `[` $indices `]` attr-dict `:` type($tensor) `->` type($element)";
}

def Nova_FieldAccessOp : Nova_Op<"field", [Pure]> {
  let summary = "Access a field of a struct";
  let arguments = (ins 
    AnyType:$struct,
    StrAttr:$field_name
  );
  let results = (outs AnyType:$field_value);
  let assemblyFormat = "$struct `.` $field_name attr-dict `:` type($struct) `->` type($field_value)";
}

// ============================================================================
// PIPELINE OPERATIONS
// ============================================================================

def Nova_PipeOp : Nova_Op<"pipe", [Pure]> {
  let summary = "Pipeline operator (forward composition)";
  let arguments = (ins 
    AnyType:$input,
    FlatSymbolRefAttr:$func
  );
  let results = (outs AnyType:$output);
  let assemblyFormat = "$input `|>` $func attr-dict `:` type($input) `->` type($output)";
}

// ============================================================================
// RANGE OPERATIONS
// ============================================================================

def Nova_RangeOp : Nova_Op<"range", [Pure]> {
  let summary = "Create a range expression";
  let arguments = (ins 
    AnyType:$start,
    AnyType:$end,
    Optional<AnyType>:$step
  );
  let results = (outs AnyType:$range);
  let assemblyFormat = "range $start `..` $end (`,` $step^)? attr-dict `:` type($start) `->` type($range)";
}

// ============================================================================
// SCIENTIFIC OPERATIONS
// ============================================================================

def Nova_UnitOp : Nova_Op<"unit", [Pure]> {
  let summary = "Define a physical unit";
  let arguments = (ins 
    StrAttr:$name,
    StrAttr:$definition
  );
  let results = (outs AnyType:$unit_type);
  let assemblyFormat = "unit $name `=` $definition attr-dict";
}

def Nova_MoleculeOp : Nova_Op<"molecule", [Pure]> {
  let summary = "Define a chemical molecule";
  let arguments = (ins 
    StrAttr:$name,
    DictionaryAttr:$composition  // element -> count mapping
  );
  let results = (outs AnyType:$molecule_type);
  let assemblyFormat = "molecule $name `{` $composition `}` attr-dict";
}

def Nova_ReactionOp : Nova_Op<"reaction", []> {
  let summary = "Define a chemical reaction";
  let arguments = (ins 
    StrAttr:$name,
    ArrayAttr:$reactants,
    ArrayAttr:$products
  );
  let results = (outs AnyType:$reaction_type);
  let assemblyFormat = "reaction $name `{` $reactants `=>` $products `}` attr-dict";
}

def Nova_SequenceOp : Nova_Op<"sequence", [Pure]> {
  let summary = "Define a biological sequence";
  let arguments = (ins 
    StrAttr:$seq_kind,   // "DNA" | "RNA" | "Protein"
    StrAttr:$name,
    StrAttr:$data
  );
  let results = (outs AnyType:$sequence_type);
  let assemblyFormat = "sequence $seq_kind $name `=` $data attr-dict";
}

// ============================================================================
// LIFETIME OPERATIONS
// ============================================================================

def Nova_LifetimeBeginOp : Nova_Op<"lifetime.begin", []> {
  let summary = "Mark the beginning of a lifetime region";
  let arguments = (ins StrAttr:$lifetime);
  let assemblyFormat = "$lifetime attr-dict";
}

def Nova_LifetimeEndOp : Nova_Op<"lifetime.end", []> {
  let summary = "Mark the end of a lifetime region";
  let arguments = (ins StrAttr:$lifetime);
  let assemblyFormat = "$lifetime attr-dict";
}

#endif // NOVA_DIALECT
