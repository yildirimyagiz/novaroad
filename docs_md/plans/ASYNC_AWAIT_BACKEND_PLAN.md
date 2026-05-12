# ⚡ Async/Await Backend Implementation Plan

**Modern Concurrency for Nova**  
**Status**: Runtime 60% ready, Backend 0% done  
**Estimated Time**: ~1 week (5-7 days)

---

## 📊 Current Status Analysis

### ✅ What's Already Done

#### 1. **Runtime Infrastructure (60%)**
- ✅ Coroutines: `src/runtime/async/coroutine.c` (520 lines)
- ✅ Event loop: `src/runtime/async/event_loop.c` (467 lines)
- ✅ Futures: `src/runtime/async/future.c` (570 lines)
- ✅ Standard library: `zn/stdlib/async.zn` (455 lines)

#### 2. **Existing Runtime Support**
```c
// In coroutine.c
typedef struct NovaCoroutine {
    void *stack;
    size_t stack_size;
    CoroutineState state;
    void *(*func)(void *);
    void *arg;
    void *result;
} NovaCoroutine;

// In future.c
typedef struct NovaFuture {
    FutureState state;
    void *result;
    NovaError error;
    Waker *wakers;
    size_t waker_count;
} NovaFuture;

// In event_loop.c
typedef struct NovaEventLoop {
    Task *task_queue;
    Reactor *reactor;
    bool running;
} NovaEventLoop;
```

#### 3. **What Works in stdlib/async.zn**
- ✅ Future trait definition
- ✅ Task spawning API
- ✅ JoinHandle implementation
- ✅ Executor interface
- ✅ Runtime setup

### ❌ What's Missing (Compiler Integration)

1. **Parser Integration** - Parse `async fn` and `await` syntax
2. **Semantic Analysis** - Type checking for async functions
3. **Async Transformation** - Convert async/await to state machines
4. **Codegen Integration** - Generate bytecode for async operations
5. **Error Handling** - Async error propagation

---

## 🎯 Implementation Plan

### **Phase 1: Parser Integration** (2 days)

#### Day 1 - Async Function Syntax
**File**: `src/compiler/parser.c`

**Task 1.1**: Parse async functions
```c
// async fn fetch(url: String) -> Result<Data, Error>
FnDecl *parse_async_function(Parser *p) {
    bool is_async = false;
    
    if (match(p, TOKEN_ASYNC)) {
        is_async = true;
    }
    
    expect(p, TOKEN_FN);
    char *name = parse_identifier(p);
    
    // Parse parameters
    Param *params = parse_params(p);
    
    // Parse return type
    Type *return_type = parse_return_type(p);
    
    // For async functions, wrap return type in Future<T>
    if (is_async) {
        return_type = type_create_future(return_type);
    }
    
    // Parse body
    Block *body = parse_block(p);
    
    FnDecl *fn = fn_create(name, params, return_type, body);
    fn->is_async = is_async;
    return fn;
}
```

**Task 1.2**: Parse await expressions
```c
// await fetch(url)
Expr *parse_await_expr(Parser *p) {
    expect(p, TOKEN_AWAIT);
    Expr *future_expr = parse_expr(p);
    
    return expr_create_await(future_expr);
}
```

**Task 1.3**: Add async tokens
```c
// In lexer.c
typedef enum {
    // ... existing tokens
    TOKEN_ASYNC,
    TOKEN_AWAIT,
} TokenType;

// Keyword table
{"async", TOKEN_ASYNC},
{"await", TOKEN_AWAIT},
```

**Files to modify**:
- `src/compiler/lexer.c` - Add async/await keywords
- `src/compiler/parser.c` - Parse async functions
- `src/compiler/tokens.c` - Add tokens

**Tests**: Parse async function declarations

---

#### Day 2 - AST Integration
**File**: `src/compiler/ast.c`

**Task 2.1**: Add async AST nodes
```c
// Add to ast.h
typedef struct {
    char *name;
    Param *params;
    size_t param_count;
    Type *return_type;
    Block *body;
    bool is_async;        // NEW!
} FnDecl;

typedef struct {
    Expr *future;         // Expression that returns Future<T>
} AwaitExpr;

typedef enum {
    // ... existing
    EXPR_AWAIT,
} ExprKind;
```

**Task 2.2**: Create async node constructors
```c
FnDecl *fn_create_async(char *name, Param *params, Type *ret, Block *body);
Expr *expr_create_await(Expr *future);
Type *type_create_future(Type *inner);
```

**Files to modify**:
- `include/nova_ast.h` - Add async node types
- `src/compiler/ast.c` - Add constructors

**Tests**: AST construction for async functions

---

### **Phase 2: Semantic Analysis** (2 days)

#### Day 3 - Async Type Checking
**File**: `src/compiler/semantic.c`

**Task 3.1**: Type check async functions
```c
Type *typecheck_async_fn(Checker *c, FnDecl *fn) {
    // Enter async context
    c->in_async_fn = true;
    
    // Check parameters
    for (size_t i = 0; i < fn->param_count; i++) {
        typecheck_param(c, &fn->params[i]);
    }
    
    // Check body
    Type *body_type = typecheck_block(c, fn->body);
    
    // Verify return type matches
    // async fn foo() -> T  actually returns Future<T>
    Type *expected_inner = get_future_inner_type(fn->return_type);
    if (!type_equals(body_type, expected_inner)) {
        error("async function returns %s, expected %s",
              type_to_string(body_type),
              type_to_string(expected_inner));
    }
    
    c->in_async_fn = false;
    return fn->return_type; // Future<T>
}
```

**Task 3.2**: Type check await expressions
```c
Type *typecheck_await(Checker *c, AwaitExpr *await) {
    // Can only await in async functions
    if (!c->in_async_fn) {
        error("'await' can only be used in async functions");
    }
    
    // Type check the future expression
    Type *future_type = typecheck_expr(c, await->future);
    
    // Verify it's a Future<T>
    if (!is_future_type(future_type)) {
        error("'await' expects Future<T>, got %s",
              type_to_string(future_type));
    }
    
    // await Future<T> returns T
    return get_future_inner_type(future_type);
}
```

**Task 3.3**: Async error propagation
```c
// async fn foo() -> Result<T, E>
// await bar()? should work
Type *typecheck_try_await(Checker *c, Expr *expr) {
    // await expr? 
    // 1. Type check await (returns Result<T, E>)
    // 2. Apply try operator (returns T or early return E)
    
    Type *result_type = typecheck_await(c, expr);
    
    if (!is_result_type(result_type)) {
        error("try operator expects Result<T, E>");
    }
    
    return get_result_ok_type(result_type);
}
```

**Files to modify**:
- `src/compiler/semantic.c` - Add async type checking
- `include/nova_diagnostics.h` - Add async errors

**Tests**: Type checking for async functions

---

#### Day 4 - Async State Machine Transformation
**File**: `src/compiler/async_transform.c` (NEW)

**Task 4.1**: Transform async to state machine
```c
// Transform:
//   async fn fetch(url: String) -> Result<Data, Error> {
//       let response = await http_get(url);
//       let data = await response.json();
//       Ok(data)
//   }
//
// Into state machine:
//   fn fetch_state_machine(url: String, state: &mut State) -> Poll<Result<Data, Error>> {
//       loop {
//           match state.step {
//               0 => {
//                   match http_get(url).poll() {
//                       Poll::Ready(response) => {
//                           state.response = response;
//                           state.step = 1;
//                       }
//                       Poll::Pending => return Poll::Pending,
//                   }
//               }
//               1 => {
//                   match state.response.json().poll() {
//                       Poll::Ready(data) => {
//                           state.step = 2;
//                           return Poll::Ready(Ok(data));
//                       }
//                       Poll::Pending => return Poll::Pending,
//                   }
//               }
//               _ => panic!("invalid state"),
//           }
//       }
//   }

typedef struct {
    size_t current_state;
    size_t await_count;
    Stmt **transformed_states;
} AsyncTransform;

AsyncTransform *transform_async_function(FnDecl *async_fn) {
    AsyncTransform *transform = malloc(sizeof(AsyncTransform));
    transform->current_state = 0;
    transform->await_count = count_awaits(async_fn->body);
    
    // Create state machine
    transform->transformed_states = malloc(sizeof(Stmt*) * transform->await_count);
    
    // Visit each await point and create a state
    visit_async_body(async_fn->body, transform);
    
    return transform;
}
```

**Task 4.2**: Implement await point splitting
```c
// Split function at each await point
void visit_await_expr(AsyncTransform *t, AwaitExpr *await) {
    // Save local variables to state
    save_locals_to_state(t);
    
    // Create poll check
    Stmt *poll = create_poll_check(await->future);
    
    // If Pending, return Pending
    // If Ready, continue to next state
    t->transformed_states[t->current_state] = poll;
    t->current_state++;
}
```

**Task 4.3**: Generate state struct
```c
// Generate state struct for local variables
typedef struct {
    size_t step;
    // Local variables from async function
    String url;
    Response response;
    Data data;
} FetchState;
```

**Files to create**:
- `src/compiler/async_transform.c` - State machine transformation

**Tests**: Transform async functions to state machines

---

### **Phase 3: Code Generation** (2 days)

#### Day 5 - Bytecode Generation
**File**: `src/compiler/codegen.c`

**Task 5.1**: Generate async function wrapper
```c
void codegen_async_fn(Codegen *cg, FnDecl *fn) {
    // Generate two functions:
    // 1. Wrapper that creates Future
    // 2. State machine that implements Poll
    
    // 1. Wrapper function
    codegen_fn_prologue(cg, fn->name);
    
    // Create state struct
    emit_create_state(cg, fn);
    
    // Create Future from state machine
    emit_create_future(cg, fn);
    
    // Return Future
    emit_return(cg);
    
    // 2. State machine function
    char *state_fn_name = format("%s_state_machine", fn->name);
    codegen_state_machine(cg, fn, state_fn_name);
}
```

**Task 5.2**: Generate await bytecode
```c
void codegen_await(Codegen *cg, AwaitExpr *await) {
    // Generate code for the future expression
    codegen_expr(cg, await->future);
    
    // Call poll on the future
    emit_byte(cg, OP_POLL_FUTURE);
    
    // Check result
    emit_byte(cg, OP_MATCH_POLL);
    
    // If Pending: save state and return Pending
    size_t pending_jump = emit_jump(cg, OP_JUMP_IF_PENDING);
    
    // If Ready: extract value and continue
    emit_byte(cg, OP_UNWRAP_READY);
    
    patch_jump(cg, pending_jump);
}
```

**Task 5.3**: Add async opcodes
```c
// In opcode.h
typedef enum {
    // ... existing opcodes
    OP_CREATE_FUTURE,
    OP_POLL_FUTURE,
    OP_MATCH_POLL,
    OP_UNWRAP_READY,
    OP_RETURN_PENDING,
} OpCode;
```

**Files to modify**:
- `src/compiler/codegen.c` - Add async codegen
- `src/backend/opcode.h` - Add opcodes
- `src/backend/bytecode.c` - Add bytecode emission

**Tests**: Generate bytecode for async functions

---

#### Day 6 - VM Integration
**File**: `src/backend/vm.c`

**Task 6.1**: Implement async opcodes
```c
void vm_execute_async_opcodes(VM *vm) {
    switch (READ_BYTE()) {
        case OP_CREATE_FUTURE: {
            // Create Future<T> from state machine
            StateMachineFn state_fn = READ_CONSTANT();
            void *state = READ_CONSTANT();
            
            NovaFuture *future = nova_future_create();
            future->poll_fn = state_fn;
            future->state = state;
            
            push(vm, FUTURE_VAL(future));
            break;
        }
        
        case OP_POLL_FUTURE: {
            // Poll a Future<T>
            Value future_val = pop(vm);
            NovaFuture *future = AS_FUTURE(future_val);
            
            PollResult result = nova_future_poll(future);
            push(vm, POLL_VAL(result));
            break;
        }
        
        case OP_MATCH_POLL: {
            // Match on Poll<T>
            Value poll_val = pop(vm);
            PollResult poll = AS_POLL(poll_val);
            
            if (poll.state == POLL_READY) {
                push(vm, BOOL_VAL(true));
                push(vm, poll.value);
            } else {
                push(vm, BOOL_VAL(false));
            }
            break;
        }
        
        case OP_UNWRAP_READY: {
            // Unwrap Poll::Ready value
            Value ready_val = pop(vm);
            push(vm, ready_val);
            break;
        }
    }
}
```

**Task 6.2**: Integrate with event loop
```c
// Connect VM to event loop
void vm_run_async(VM *vm, NovaEventLoop *loop) {
    while (loop->running) {
        // Execute ready tasks
        Task *task = event_loop_pop_ready(loop);
        if (task) {
            vm_execute_task(vm, task);
        } else {
            // Wait for I/O
            event_loop_wait(loop);
        }
    }
}
```

**Files to modify**:
- `src/backend/vm.c` - Add async execution
- `src/runtime/async/event_loop.c` - Integrate with VM

**Tests**: Run async functions end-to-end

---

### **Phase 4: Testing & Polish** (1 day)

#### Day 7 - Final Integration
**Files**: Create comprehensive tests

**Task 7.1**: Create async test suite
```nova
// tests/async/01_basic.nova
async fn test_basic_async() {
    let result = await simple_future();
    assert(result == 42);
}

// tests/async/02_error_handling.nova
async fn test_error_handling() -> Result<i64, String> {
    let data = await fetch("https://api.example.com")?;
    Ok(data.len())
}

// tests/async/03_concurrent.nova
async fn test_concurrent() {
    let task1 = spawn(fetch("url1"));
    let task2 = spawn(fetch("url2"));
    
    let result1 = await task1.join();
    let result2 = await task2.join();
    
    (result1, result2)
}

// tests/async/04_timeout.nova
async fn test_timeout() {
    let result = await timeout(
        fetch("slow-url"),
        Duration::seconds(5)
    );
    
    match result {
        Ok(data) => println("Got data: {}", data),
        Err(Timeout) => println("Request timed out"),
    }
}
```

**Task 7.2**: Performance tests
```nova
// Verify async overhead is minimal
async fn benchmark_async() {
    let start = now();
    
    // Spawn 10,000 tasks
    let tasks = (0..10000).map(|i| spawn(simple_task(i)));
    
    // Await all
    for task in tasks {
        await task.join();
    }
    
    let elapsed = now() - start;
    assert(elapsed < Duration::seconds(1));
}
```

**Task 7.3**: Integration with stdlib
```nova
// Test with async I/O
async fn test_async_io() {
    let file = await File::open("test.txt");
    let content = await file.read_to_string();
    println("{}", content);
}

// Test with networking
async fn test_async_net() {
    let listener = await TcpListener::bind("127.0.0.1:8080");
    
    loop {
        let (socket, addr) = await listener.accept();
        spawn(handle_client(socket));
    }
}
```

**Tests**: Full async test suite passes

---

## 📁 Files to Create/Modify

### New Files (2)

| File | Lines | Purpose |
|------|-------|---------|
| `src/compiler/async_transform.c` | ~500 | State machine transformation |
| `tests/async/*.nova` | ~300 | Async test suite |

### Files to Modify (8)

| File | Lines | Task |
|------|-------|------|
| `src/compiler/lexer.c` | +30 | Async/await tokens |
| `src/compiler/parser.c` | +150 | Parse async syntax |
| `include/nova_ast.h` | +40 | Async AST nodes |
| `src/compiler/ast.c` | +60 | Async constructors |
| `src/compiler/semantic.c` | +200 | Async type checking |
| `src/compiler/codegen.c` | +250 | Async codegen |
| `src/backend/opcode.h` | +10 | Async opcodes |
| `src/backend/vm.c` | +150 | Async execution |

**Total**: ~1,690 lines of new code

---

## 🎯 Success Criteria

### Must Have ✅
- [x] Parse `async fn` declarations
- [x] Parse `await` expressions
- [x] Type check async functions (return Future<T>)
- [x] Error on await outside async
- [x] Transform async to state machines
- [x] Generate correct bytecode
- [x] VM async execution
- [x] Integration with event loop
- [x] Basic async I/O works

### Nice to Have 🌟
- [ ] Async closures
- [ ] Stream/async iterators
- [ ] Select! macro (wait on multiple futures)
- [ ] Async error propagation with ?
- [ ] Timeout utilities
- [ ] Async debugging support

---

## 🚀 Key Features

### 1. **Rust-Style Async/Await**
```nova
async fn fetch(url: String) -> Result<Data, Error> {
    let response = await http_get(url);
    let data = await response.json()?;
    Ok(data)
}
```

### 2. **Zero-Cost Abstraction**
- State machines generated at compile-time
- No heap allocation for futures (stack-based)
- Minimal runtime overhead

### 3. **Structured Concurrency**
```nova
async fn process_all(urls: Vec<String>) -> Vec<Result<Data, Error>> {
    let tasks = urls.map(|url| spawn(fetch(url)));
    
    let results = vec![];
    for task in tasks {
        results.push(await task.join());
    }
    
    results
}
```

### 4. **Error Handling Integration**
```nova
async fn example() -> Result<(), Error> {
    let data = await fetch("url")?;  // Propagate errors
    let parsed = await parse(data)?;
    Ok(())
}
```

---

## 📊 Estimated Timeline

| Phase | Days | Tasks |
|-------|------|-------|
| Parser Integration | 2 | Async syntax parsing |
| Semantic Analysis | 2 | Type checking, transforms |
| Code Generation | 2 | Bytecode, VM opcodes |
| Testing & Polish | 1 | Tests, integration |
| **Total** | **7 days** | **~1,690 lines** |

---

## 🎓 Learning Resources

### Async/Await Theory
- Rust async book: https://rust-lang.github.io/async-book/
- JavaScript promises & async/await
- C# async/await implementation

### Implementation References
- Rust compiler async transformation
- Python asyncio implementation
- Go goroutines (different model, but useful)

---

## 💡 Comparison with Other Languages

| Feature | Nova | Rust | JavaScript | Python |
|---------|------|------|------------|--------|
| Syntax | ✅ async/await | ✅ async/await | ✅ async/await | ✅ async/await |
| Zero-cost | ✅ Yes | ✅ Yes | ❌ No | ❌ No |
| Type safety | ✅ Yes | ✅ Yes | ❌ No | ❌ No |
| Error handling | ✅ Result<T,E> | ✅ Result<T,E> | ❌ Exceptions | ❌ Exceptions |
| Runtime | ✅ Built-in | ❌ tokio | ✅ Built-in | ✅ Built-in |

---

## 🎯 Next Steps

After this is done, Nova will have:
1. ✅ Type System (world-class)
2. ✅ Pattern Matching (Rust-level)
3. ✅ Error Handling (Result<T, E>)
4. ✅ Generics (full monomorphization)
5. ✅ Unit Algebra (UNIQUE!) 🌟
6. ✅ **Async/Await (modern!)** ⚡

**Ready to start?** 🚀

Choose:
1. Start with Parser Integration (Day 1)
2. Review the plan first
3. Ask questions about specific parts
