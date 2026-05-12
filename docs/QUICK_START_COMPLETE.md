# Nova Language - Complete Quick Start Guide

Welcome to Nova! This comprehensive guide will get you up and running with Nova in minutes.

## Table of Contents

1. [Installation](#installation)
2. [Your First Nova Program](#your-first-nova-program)
3. [Language Fundamentals](#language-fundamentals)
4. [Advanced Features](#advanced-features)
5. [Building and Running](#building-and-running)
6. [Next Steps](#next-steps)

## Installation

### Prerequisites

- **macOS**: Xcode Command Line Tools
- **Linux**: GCC/Clang, CMake, Ninja
- **Windows**: Visual Studio 2019+ or MinGW

### Quick Install (macOS/Linux)

```bash
# Clone the repository
git clone https://github.com/yourusername/nova.git
cd nova

# Build Nova compiler
./build_stage0.sh

# Verify installation
./nova --version
```

### Quick Install (Windows)

```powershell
# Clone the repository
git clone https://github.com/yourusername/nova.git
cd nova

# Build using CMake
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

# Verify installation
.\Release\nova.exe --version
```

### Install from Binary (Recommended)

Download pre-built binaries from [Releases](https://github.com/yourusername/nova/releases):

```bash
# macOS (Apple Silicon)
wget https://github.com/yourusername/nova/releases/latest/download/nova-macos-arm64.tar.gz
tar xzf nova-macos-arm64.tar.gz
sudo mv nova /usr/local/bin/

# Linux (x86_64)
wget https://github.com/yourusername/nova/releases/latest/download/nova-linux-x86_64.tar.gz
tar xzf nova-linux-x86_64.tar.gz
sudo mv nova /usr/local/bin/
```

## Your First Nova Program

### Hello World

Create a file `hello.zn`:

```nova
// hello.zn
module main;

fn main() {
    println("Hello, Nova!");
}
```

Run it:

```bash
nova run hello.zn
```

### Interactive REPL

Nova includes an interactive REPL:

```bash
nova repl
```

Try some code:

```nova
>>> let x = 42
>>> let y = x * 2
>>> println("Answer: {}", y)
Answer: 84
```

## Language Fundamentals

### Variables and Types

```nova
// Immutable by default
let x = 42;
let name = "Alice";

// Mutable variables
let mut counter = 0;
counter += 1;

// Type annotations
let age: i32 = 30;
let score: f64 = 95.5;

// Type inference
let items = vec![1, 2, 3]; // Vec<i32>
```

### Functions

```nova
// Basic function
fn add(a: i32, b: i32) -> i32 {
    a + b
}

// Generic function
fn swap<T>(a: T, b: T) -> (T, T) {
    (b, a)
}

// Higher-order function
fn apply<T, F>(value: T, f: F) -> T 
where F: Fn(T) -> T {
    f(value)
}

// Usage
let result = apply(10, |x| x * 2);
println!("{}", result); // 20
```

### Control Flow

```nova
// If expressions
let max = if x > y { x } else { y };

// Pattern matching
match value {
    Some(x) if x > 0 => println!("Positive: {}", x),
    Some(x) if x < 0 => println!("Negative: {}", x),
    Some(0) => println!("Zero"),
    None => println!("No value"),
}

// Loops
for i in 0..10 {
    println!("{}", i);
}

let mut count = 0;
while count < 5 {
    count += 1;
}

loop {
    if condition {
        break;
    }
}
```

### Collections

```nova
import collections::{Vec, HashMap, HashSet};

// Vector
let mut numbers = Vec::new();
numbers.push(1);
numbers.push(2);
numbers.push(3);

// HashMap
let mut scores = HashMap::new();
scores.insert("Alice", 100);
scores.insert("Bob", 95);

// HashSet
let mut unique = HashSet::new();
unique.insert("apple");
unique.insert("banana");
```

### Error Handling

```nova
// Result type
fn divide(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err("Division by zero")
    } else {
        Ok(a / b)
    }
}

// Using Result
match divide(10, 2) {
    Ok(result) => println!("Result: {}", result),
    Err(e) => println!("Error: {}", e),
}

// ? operator
fn calculate() -> Result<i32, String> {
    let x = divide(10, 2)?;
    let y = divide(20, 4)?;
    Ok(x + y)
}
```

## Advanced Features

### Unit Algebra (Type-Safe Physics)

```nova
import physics::units::*;

fn calculate_energy() {
    let mass = 10.0 @ kg;
    let velocity = 5.0 @ m/s;
    
    let kinetic_energy = 0.5 * mass * velocity * velocity;
    println!("Energy: {}", kinetic_energy); // 125 kg⋅m²⋅s⁻²
}
```

### Machine Learning

```nova
import ml::nn::*;
import ml::tensor::*;

fn train_model() {
    // Create a simple neural network
    let model = Sequential::new()
        .add(Linear::new(784, 128))
        .add(ReLU::new())
        .add(Linear::new(128, 10))
        .add(Softmax::new());
    
    // Create optimizer
    let optimizer = Adam::new(0.001);
    
    // Training loop
    for epoch in 0..10 {
        let loss = model.train(&train_data, &optimizer);
        println!("Epoch {}: Loss = {:.4}", epoch, loss);
    }
}
```

### Async/Await

```nova
async fn fetch_data(url: &str) -> Result<String, Error> {
    let response = http::get(url).await?;
    let body = response.text().await?;
    Ok(body)
}

async fn main() {
    let data = fetch_data("https://api.example.com/data").await;
    match data {
        Ok(body) => println!("Received: {}", body),
        Err(e) => println!("Error: {}", e),
    }
}
```

### GPU Computing

```nova
import gpu::cuda::*;

#[gpu_kernel]
fn vector_add(a: &[f32], b: &[f32], c: &mut [f32], n: usize) {
    let idx = thread_idx() + block_idx() * block_dim();
    if idx < n {
        c[idx] = a[idx] + b[idx];
    }
}

fn main() {
    let a = vec![1.0f32; 1000];
    let b = vec![2.0f32; 1000];
    let mut c = vec![0.0f32; 1000];
    
    vector_add<<<(1000 + 255) / 256, 256>>>(&a, &b, &mut c, 1000);
}
```

### Web Development

```nova
import web::framework::*;

#[route("/")]
async fn index() -> Html {
    html! {
        <div>
            <h1>"Welcome to Nova Web"</h1>
            <p>"Built with Nova's web framework"</p>
        </div>
    }
}

#[route("/api/users")]
async fn get_users() -> Json<Vec<User>> {
    let users = database::query("SELECT * FROM users").await;
    Json(users)
}

fn main() {
    let app = App::new()
        .route("/", index)
        .route("/api/users", get_users);
    
    app.listen("127.0.0.1:8080").await;
}
```

## Building and Running

### Compile and Run

```bash
# Run directly
nova run main.zn

# Compile to binary
nova build main.zn -o myapp

# Run with optimizations
nova run --release main.zn

# Run tests
nova test

# Run benchmarks
nova bench
```

### Project Structure

```
my-nova-project/
├── nova.toml           # Project configuration
├── src/
│   ├── main.zn         # Entry point
│   ├── lib.zn          # Library code
│   └── utils/
│       └── helper.zn
├── tests/
│   └── integration_test.zn
└── benches/
    └── performance_bench.zn
```

### nova.toml Configuration

```toml
[package]
name = "my-nova-app"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]

[dependencies]
collections = "1.0"
async = "1.0"
ml = { version = "2.0", features = ["gpu"] }

[build]
optimization_level = 3
target = "native"
```

## Next Steps

### Learning Resources

1. **The Nova Book**: Comprehensive language guide
   - Read online: https://nova.org/book
   - Or build locally: `cd docs && mdbook serve`

2. **Examples**: Explore real-world examples
   ```bash
   cd examples
   nova run web_server.zn
   nova run ml_training.zn
   nova run game_engine.zn
   ```

3. **API Documentation**: Complete API reference
   ```bash
   nova doc --open
   ```

### Community

- **Discord**: https://discord.gg/nova
- **Forum**: https://forum.nova.org
- **GitHub**: https://github.com/yourusername/nova
- **Twitter**: @NovaLang

### Tutorials

1. **Web Development with Nova**
   - Building REST APIs
   - Server-side rendering
   - WebSocket applications

2. **Machine Learning in Nova**
   - Training neural networks
   - GPU acceleration
   - Model deployment

3. **Systems Programming**
   - Writing operating system kernels
   - Device drivers
   - Embedded systems

### Advanced Topics

- **Formal Verification**: Prove your code correct
- **Quantum Computing**: Quantum circuit simulation
- **Distributed Systems**: Build scalable applications
- **Game Development**: High-performance game engines

## Examples Gallery

### CLI Application

```nova
import cli::*;

fn main() {
    let matches = App::new("myapp")
        .version("1.0")
        .author("Your Name")
        .about("Does awesome things")
        .arg(Arg::with_name("input")
            .short("i")
            .long("input")
            .value_name("FILE")
            .help("Sets the input file")
            .required(true))
        .get_matches();
    
    let input = matches.value_of("input").unwrap();
    println!("Processing: {}", input);
}
```

### Database Application

```nova
import db::postgres::*;

async fn main() {
    let db = Database::connect("postgresql://localhost/mydb").await?;
    
    let users = db.query("SELECT * FROM users WHERE age > $1", &[25]).await?;
    
    for user in users {
        println!("User: {} ({})", user.name, user.age);
    }
}
```

### Real-time Data Processing

```nova
import streaming::*;

async fn main() {
    let stream = DataStream::from_kafka("localhost:9092", "events")
        .filter(|event| event.priority > 5)
        .map(|event| process_event(event))
        .window(Duration::seconds(60))
        .reduce(|acc, event| acc + event.value);
    
    stream.for_each(|result| {
        println!("Window result: {}", result);
    }).await;
}
```

## Troubleshooting

### Common Issues

1. **Compilation errors**
   ```bash
   # Enable verbose output
   nova build --verbose
   
   # Check syntax
   nova check main.zn
   ```

2. **Performance issues**
   ```bash
   # Profile your application
   nova profile main.zn
   
   # Enable optimizations
   nova build --release -O3
   ```

3. **Dependency issues**
   ```bash
   # Update dependencies
   nova update
   
   # Clean build
   nova clean && nova build
   ```

## Getting Help

If you're stuck:

1. Check the [FAQ](https://nova.org/faq)
2. Search [GitHub Issues](https://github.com/yourusername/nova/issues)
3. Ask on [Discord](https://discord.gg/nova)
4. Post on [Stack Overflow](https://stackoverflow.com/questions/tagged/nova)

---

**Ready to build something amazing?** Start coding with Nova today! 🚀
