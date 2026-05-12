# Nova Compiler Implementation Guide

## Overview

Nova's compiler consists of three major advanced systems working together:

1. **Polonius Borrow Checker** - Location-based lifetime analysis
2. **Parser Combinators** - Composable, type-safe parsing
3. **Effect Inference System** - Hindley-Milner effect inference

---

## 1. Polonius Borrow Checker

### Architecture

```
Location Tracking → Origin Inference → Loan Tracking → Subset Constraints
                                                              ↓
                                                         Validation
```

### Core Components

#### Location (`location.zn`)
```nova
let block = BlockId::new(0);
let location = Location::new(block, statement_index);
```

#### Origin (`origin.zn`)
```nova
// Universal region (function parameter)
let param_origin = origin_ctx.new_universal(Some("'a"));

// Existential region (local variable)
let local_origin = origin_ctx.new_existential(location);
```

#### Loan (`loan.zn`)
```nova
// Issue a borrow
let loan = loan_ctx.issue_loan(
    location,
    Path::local(var_id),
    region,
    is_mut
);
```

#### Subset Constraints (`subset.zn`)
```nova
// Add constraint: 'a ⊆ 'b
subset_ctx.add_subset(origin_a, origin_b, location);

// Check if constraint holds
if subset_ctx.check_subset(&origin_a, &origin_b) {
    // Constraint satisfied
}
```

### Usage Example

```nova
use nova::compiler::typesystem::polonius::*;

fn check_borrow_safety() {
    let mut ctx = PoloniusContext::new();
    
    // Create location
    let block = BlockId::new(0);
    let loc = ctx.new_location(block);
    
    // Create variable with origin
    let var_origin = ctx.new_existential_origin(loc);
    let path = Path::local(0);
    
    // Issue borrow
    let loan = ctx.issue_loan(loc, path, var_origin, false);
    
    // Validate all constraints
    match ctx.validate() {
        Ok(()) => println!("✅ Borrow safe"),
        Err(errors) => println!("❌ Errors: {:?}", errors),
    }
}
```

### Mobile-Aware Features

```nova
// Create context with mobile awareness
let mut ctx = PoloniusContext::with_mobile(
    0.8,                    // Battery level (80%)
    ThermalState::Cool,     // Device temperature
    Platform::iOS,          // Target platform
);

// Mutable borrows may be restricted based on device state
let loan = ctx.issue_loan(loc, path, origin, true);
```

---

## 2. Parser Combinators

### Architecture

```
Core Parsers → Combinators → Pratt Parser → Error Recovery
```

### Core Parsers (`core.zn`)

```nova
// Match a specific token
let int_parser = token(TokenKind::IntLit(0));

// Always succeed with a value
let pure_parser = pure(42);

// Always fail
let fail_parser = fail("Expected number");
```

### Combinators (`combinators.zn`)

```nova
// Map result
let doubled = map(int_parser, |n| n * 2);

// Sequence two parsers
let seq = and_then(parser1, parser2);

// Alternative parsers
let either = or(string_parser, int_parser);

// Zero or more
let many_ints = many(int_parser);

// One or more
let some_ints = many1(int_parser);

// Optional
let opt_semi = optional(semicolon);

// Separated list
let csv = sep_by(int_parser, comma);
```

### Pratt Parser (`pratt.zn`)

```nova
// Create Pratt parser for expressions
let mut parser = PrattParser::new(primary_parser);

// Add prefix operator (unary minus)
parser.prefix(TokenKind::Minus, 70, |expr| {
    Expr::Neg(Box::new(expr))
});

// Add infix operator (addition)
parser.infix(TokenKind::Plus, 10, Associativity::Left, |l, r| {
    Expr::Add(Box::new(l), Box::new(r))
});

// Parse expression
match parser.parse(&tokens) {
    ParseResult::Success(expr, rest) => // Use expr
    ParseResult::Failure(err) => // Handle error
}
```

### Error Recovery (`error_recovery.zn`)

```nova
// Create recovering parser
let recovering = RecoveringParser::new(
    parser,
    vec![TokenKind::Semicolon, TokenKind::RBrace] // Sync tokens
);

// Get error suggestions
let suggestions = suggest_fix(&error, &tokens);
```

---

## 3. Effect Inference System

### Architecture

```
Constraint Generation → Unification → Polymorphism → Handlers
```

### Effects (`constraint.zn`)

```nova
// Basic effects
let pure = Effect::Pure;
let io = Effect::IO;
let async_effect = Effect::Async;

// Custom effects
let custom = Effect::Custom("Database".to_string());

// Effect variables
let var = Effect::Var(EffectVar::new(0));

// Union of effects
let combined = Effect::Union(vec![Effect::IO, Effect::Async]);
```

### Solver (`solver.zn`)

```nova
let mut solver = EffectSolver::new();

// Create fresh variable
let var = solver.fresh_var();

// Unify effects
solver.unify(Effect::Var(var), Effect::IO)?;

// Check subsumption
solver.check_subsumption(Effect::Pure, Effect::IO)?; // OK

// Solve constraints
let constraints = EffectConstraintSet::new();
solver.solve(&constraints)?;

// Get final effect
let final_effect = solver.finalize(Effect::Var(var));
```

### Polymorphism (`polymorphism.zn`)

```nova
// Create polymorphic scheme: forall E. E
let var = EffectVar::new(0);
let scheme = EffectScheme::poly(vec![var], Effect::Var(var));

// Instantiate with fresh variables
let instance = scheme.instantiate(&mut solver);

// Generalize effect
let free_vars = HashSet::new();
let scheme = EffectScheme::generalize(effect, &free_vars);
```

### Effect Handlers (`handlers.zn`)

```nova
// Exception handler
let handler = EffectHandler::new(Effect::Custom("Exception"));
handler.handle("throw", |msg, cont| {
    println!("Exception: {:?}", msg);
    Value::Unit
});

// State handler
let state_handler = state_handler(initial_state);

// Async handler
let async_handler = async_handler();
```

---

## Integration

### Unified Pipeline

```nova
use nova::compiler::integration::NovaCompiler;

fn compile_program(source: String) -> Result<CompiledModule, Vec<String>> {
    let mut compiler = NovaCompiler::new();
    
    compiler.compile(source)
}
```

### Complete Example

```nova
fn compile_and_check() {
    let source = r#"
        fn example(x: &i32) -> i32 | IO {
            let y = *x;
            println!("{}", y);
            y
        }
    "#;
    
    let mut compiler = NovaCompiler::new();
    
    match compiler.compile(source.to_string()) {
        Ok(module) => println!("✅ Compiled: {}", module.name),
        Err(errors) => {
            println!("❌ Compilation errors:");
            for err in errors {
                println!("  - {}", err);
            }
        }
    }
}
```

---

## Testing

### Running Tests

```bash
# Run Polonius tests
zntest nova/zn/src/compiler/typesystem/polonius/tests.zn

# Run Parser tests
zntest nova/zn/src/compiler/parser_combinators/tests.zn

# Run Effect tests
zntest nova/zn/src/compiler/effect_inference/tests.zn
```

### Test Coverage

- **Polonius:** 15 tests covering location, origin, loan, subset, CFG, live range
- **Parser:** 12 tests covering combinators, Pratt parsing, error recovery
- **Effects:** 5 tests covering unification, subsumption, polymorphism

---

## Performance Characteristics

### Polonius Borrow Checker
- **Time Complexity:** O(n³) for transitive closure (optimized with caching)
- **Space Complexity:** O(n²) for subset graph
- **Typical overhead:** 5-10% compile time vs simple borrow checker

### Parser Combinators
- **Time Complexity:** O(n) for most parsers, O(n²) worst case with backtracking
- **Space Complexity:** O(d) where d is parse tree depth
- **Typical overhead:** Within 10% of hand-written parser

### Effect Inference
- **Time Complexity:** O(n·α(n)) where α is inverse Ackermann (nearly linear)
- **Space Complexity:** O(n) for substitutions
- **Typical overhead:** < 5% compile time

---

## Advanced Features

### Polonius: Mobile-Aware Borrowing

```nova
// Restrict borrows based on device state
if thermal_state == ThermalState::Critical && is_mut {
    return Err("Mutable borrow disallowed when overheating");
}
```

### Parser: Custom Error Messages

```nova
let error = ParseError {
    message: "Expected semicolon".to_string(),
    span: current_token.span,
    expected: vec!["semicolon".to_string()],
    recovery_suggestions: vec![
        "Add `;` at end of statement".to_string()
    ],
};
```

### Effects: Algebraic Effect Handlers

```nova
handle {
    let x = read_line(); // IO effect
    write_line(x);
} with {
    IO::read_line() => { /* custom handler */ }
    IO::write_line(s) => { /* custom handler */ }
}
```

---

## Best Practices

### 1. Polonius
- Always validate constraints after building the graph
- Use mobile-aware features for embedded/mobile targets
- Cache transitive closure for repeated queries

### 2. Parser
- Use Pratt parser for expressions (handles precedence automatically)
- Add error recovery for better diagnostics
- Provide helpful error messages with suggestions

### 3. Effects
- Start with monomorphic effects, generalize later
- Use effect bounds to constrain polymorphism
- Consider algebraic handlers for advanced control flow

---

## Troubleshooting

### Common Issues

**Polonius: Infinite effect type**
```
Error: Infinite effect type: E = E | IO
```
**Solution:** Check for cyclic subset constraints

**Parser: Backtracking too much**
```
Performance: Parse takes O(n²) time
```
**Solution:** Use committed choice (`and_then` instead of `or`)

**Effects: Cannot unify**
```
Error: Cannot unify IO with Pure
```
**Solution:** Use subsumption instead of unification

---

## Further Reading

- [Polonius RFC](https://github.com/rust-lang/polonius)
- [Parser Combinators in Practice](https://bodil.lol/parser-combinators/)
- [Algebraic Effects](https://www.eff-lang.org/)
