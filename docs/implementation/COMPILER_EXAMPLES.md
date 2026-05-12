# Nova Compiler - Code Examples

## Complete Working Examples

### Example 1: Basic Borrow Checking

```nova
use nova::compiler::typesystem::polonius::*;

fn main() {
    let mut ctx = PoloniusContext::new();
    
    // Scenario: Borrow a variable
    let block = BlockId::new(0);
    let loc1 = ctx.new_location(block);
    let loc2 = ctx.new_location(block);
    
    // Variable x created
    let x_origin = ctx.new_existential_origin(loc1);
    let x_path = Path::local(0);
    
    // Immutable borrow: let y = &x
    let y_loan = ctx.issue_loan(loc1, x_path.clone(), x_origin.clone(), false);
    
    // Another immutable borrow: let z = &x (OK)
    let z_loan = ctx.issue_loan(loc2, x_path, x_origin, false);
    
    // Check for conflicts
    let conflicts = ctx.find_conflicts(y_loan);
    assert!(conflicts.is_empty()); // No conflicts - two immutable borrows OK
    
    println!("✅ Borrow checking passed!");
}
```

### Example 2: Mutable Borrow Conflicts

```nova
fn test_mut_conflict() {
    let mut ctx = PoloniusContext::new();
    let block = BlockId::new(0);
    let loc1 = ctx.new_location(block);
    let loc2 = ctx.new_location(block);
    
    let x_origin = ctx.new_existential_origin(loc1);
    let x_path = Path::local(0);
    
    // Mutable borrow: let y = &mut x
    let y_loan = ctx.issue_loan(loc1, x_path.clone(), x_origin.clone(), true);
    
    // Another mutable borrow: let z = &mut x (ERROR)
    let z_loan = ctx.issue_loan(loc2, x_path, x_origin, true);
    
    // Check conflicts
    let conflicts = ctx.find_conflicts(y_loan);
    assert!(!conflicts.is_empty()); // Conflict detected!
    
    println!("❌ Mutable aliasing detected (as expected)");
}
```

### Example 3: Lifetime Constraints

```nova
fn test_lifetime_outlives() {
    let mut ctx = PoloniusContext::new();
    let block = BlockId::new(0);
    let loc = ctx.new_location(block);
    
    // Function signature: fn foo<'a, 'b>(x: &'a i32, y: &'b i32) -> &'a i32
    // Constraint: 'a ⊆ 'b (return value lives as long as 'a)
    
    let lifetime_a = ctx.new_universal_origin(Some("'a".to_string()));
    let lifetime_b = ctx.new_universal_origin(Some("'b".to_string()));
    
    // Return value borrows from 'a, so 'a must outlive return
    ctx.add_subset(lifetime_a.clone(), lifetime_b.clone(), loc);
    
    // Check constraint
    if ctx.check_subset(&lifetime_a, &lifetime_b) {
        println!("✅ Lifetime constraint satisfied");
    }
}
```

### Example 4: Expression Parsing with Pratt

```nova
use nova::compiler::parser_combinators::*;

fn parse_math_expression() {
    // Create Pratt parser
    let mut parser = PrattParser::new(Box::new(|input| {
        match &input[0].kind {
            TokenKind::IntLit(n) => ParseResult::Success(Expr::Num(*n), &input[1..]),
            _ => ParseResult::Failure(/* error */),
        }
    }));
    
    // Configure operators
    parser.infix(TokenKind::Plus, 10, Associativity::Left, |l, r| {
        Expr::Add(Box::new(l), Box::new(r))
    });
    
    parser.infix(TokenKind::Star, 20, Associativity::Left, |l, r| {
        Expr::Mul(Box::new(l), Box::new(r))
    });
    
    // Parse: 2 + 3 * 4
    let tokens = vec![
        Token { kind: TokenKind::IntLit(2), /* ... */ },
        Token { kind: TokenKind::Plus, /* ... */ },
        Token { kind: TokenKind::IntLit(3), /* ... */ },
        Token { kind: TokenKind::Star, /* ... */ },
        Token { kind: TokenKind::IntLit(4), /* ... */ },
    ];
    
    match parser.parse(&tokens) {
        ParseResult::Success(expr, _) => {
            // Result: Add(2, Mul(3, 4)) - correct precedence!
            println!("✅ Parsed: {:?}", expr);
        }
        ParseResult::Failure(err) => println!("❌ {}", err.message),
    }
}
```

### Example 5: Effect Inference

```nova
use nova::compiler::effect_inference::*;

fn infer_function_effects() {
    let mut solver = EffectSolver::new();
    
    // Function: fn read_config() -> String
    // Infer effect for read_config
    let read_effect = solver.fresh_var();
    
    // Constraint: read_config performs IO
    solver.unify(Effect::Var(read_effect), Effect::IO).unwrap();
    
    // Function: fn process(s: String) -> Result<Data>
    let process_effect = solver.fresh_var();
    
    // Constraint: process is pure
    solver.unify(Effect::Var(process_effect), Effect::Pure).unwrap();
    
    // Function: fn main() calls both
    let main_effect = solver.fresh_var();
    
    // main's effect = read_effect ∪ process_effect
    let combined = Effect::Var(read_effect).union(Effect::Var(process_effect));
    solver.unify(Effect::Var(main_effect), combined).unwrap();
    
    // Finalize
    let result = solver.finalize(Effect::Var(main_effect));
    println!("main() effect: {:?}", result); // IO (since Pure ∪ IO = IO)
}
```

### Example 6: Effect Polymorphism

```nova
fn effect_polymorphic_map() {
    let mut solver = EffectSolver::new();
    
    // map<T, U, E>(f: T -> U | E, list: [T]) -> [U] | E
    let effect_param = solver.fresh_var();
    let scheme = EffectScheme::poly(
        vec![effect_param],
        Effect::Var(effect_param)
    );
    
    // Use map with pure function
    let pure_instance = scheme.instantiate(&mut solver);
    solver.unify(pure_instance, Effect::Pure).unwrap();
    
    // Use map with IO function
    let io_instance = scheme.instantiate(&mut solver);
    solver.unify(io_instance, Effect::IO).unwrap();
    
    println!("✅ Effect polymorphism works!");
}
```

### Example 7: Complete Pipeline

```nova
use nova::compiler::integration::*;

fn compile_complete_program() {
    let source = r#"
        fn factorial(n: i32) -> i32 | Pure {
            if n <= 1 {
                1
            } else {
                n * factorial(n - 1)
            }
        }
        
        fn main() | IO {
            let result = factorial(5);
            println!("5! = {}", result);
        }
    "#;
    
    let mut compiler = NovaCompiler::new();
    
    match compiler.compile(source.to_string()) {
        Ok(module) => {
            println!("✅ Compilation successful!");
            println!("   Module: {}", module.name);
            println!("   - Borrow checking: passed");
            println!("   - Effect inference: passed");
            println!("   - All constraints satisfied");
        }
        Err(errors) => {
            println!("❌ Compilation failed:");
            for (i, err) in errors.iter().enumerate() {
                println!("   {}. {}", i + 1, err);
            }
        }
    }
}
```

### Example 8: Mobile-Aware Compilation

```nova
fn compile_for_mobile() {
    use nova::compiler::typesystem::polonius::{ThermalState, Platform};
    
    // Create mobile-aware context
    let mut ctx = PoloniusContext::with_mobile(
        0.2,                    // Low battery (20%)
        ThermalState::Hot,      // Device is hot
        Platform::Android,      // Target Android
    );
    
    let block = BlockId::new(0);
    let loc = ctx.new_location(block);
    let origin = ctx.new_existential_origin(loc);
    let path = Path::local(0);
    
    // Try to issue mutable borrow with constrained resources
    let loan = ctx.issue_loan(loc, path, origin, true);
    
    // Context may warn about resource-intensive operations
    match ctx.validate() {
        Ok(()) => println!("✅ Safe for mobile"),
        Err(warnings) => {
            println!("⚠️  Mobile warnings:");
            for warning in warnings {
                println!("   - {}", warning);
            }
        }
    }
}
```

### Example 9: Parser with Error Recovery

```nova
use nova::compiler::parser_combinators::error_recovery::*;

fn parse_with_recovery() {
    let tokens = vec![
        Token { kind: TokenKind::Let, /* ... */ },
        Token { kind: TokenKind::Ident("x".to_string()), /* ... */ },
        Token { kind: TokenKind::Eq, /* ... */ },
        Token { kind: TokenKind::IntLit(42), /* ... */ },
        // Missing semicolon here!
        Token { kind: TokenKind::Let, /* ... */ },
        // ...
    ];
    
    let stmt_parser = /* ... */;
    let recovering = RecoveringParser::new(
        stmt_parser,
        vec![TokenKind::Semicolon, TokenKind::RBrace]
    );
    
    match recovering.parse(&tokens) {
        ParseResult::Success(stmt, _) => println!("✅ Parsed"),
        ParseResult::Failure(err) => {
            println!("❌ Error: {}", err.message);
            println!("💡 Suggestions:");
            for suggestion in &err.recovery_suggestions {
                println!("   - {}", suggestion);
            }
            
            // Auto-suggest fixes
            let fixes = suggest_fix(&err, &tokens);
            for fix in fixes {
                println!("   🔧 {}", fix);
            }
        }
    }
}
```

### Example 10: Algebraic Effect Handlers

```nova
use nova::compiler::effect_inference::handlers::*;

fn use_effect_handlers() {
    // Define exception handler
    let mut handler = exception_handler();
    
    // In real implementation, would handle code like:
    // handle {
    //     throw("Error!");
    // } with exception_handler
    
    println!("✅ Effect handlers configured");
    
    // State handler example
    let state_h = state_handler(0); // Initial state = 0
    
    // Async handler example
    let async_h = async_handler();
    
    // Multiple handlers can be composed
}
```

---

## Running These Examples

```bash
# Compile and run example 1
znc examples/borrow_checking_basic.zn
./borrow_checking_basic

# Run with tests
zntest examples/

# Run specific example
zn run examples/effect_inference.zn
```

---

## Real-World Scenario

### Web Server with Effects

```nova
// Effect-tracked web server
fn handle_request(req: Request) -> Response | IO + Async {
    let user_id = extract_user(req);           // Pure
    let user = fetch_from_db(user_id);          // IO + Async
    let processed = process_data(user);         // Pure
    send_response(processed)                    // IO + Async
}

fn main() | IO + Async {
    let server = create_server();
    
    // Effect inference automatically determines:
    // - fetch_from_db: IO + Async
    // - process_data: Pure
    // - handle_request: IO + Async (union of all effects)
    
    server.listen(handle_request);
}
```

### Embedded System with Mobile-Aware Borrowing

```nova
fn sensor_loop() | IO {
    let mut ctx = PoloniusContext::with_mobile(
        battery_level(),
        thermal_state(),
        Platform::Embedded,
    );
    
    loop {
        // Borrow sensor data
        let data = &sensors.temperature;
        
        // Mobile-aware: reduce sampling if battery low
        if ctx.mobile_context.battery_level < 0.15 {
            sleep(1000); // Longer delay when battery low
        }
        
        process(data);
    }
}
```
