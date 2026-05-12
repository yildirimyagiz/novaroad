# Nova Standard Library - System Module

The `system` module provides low-level interfaces to operating system services,
hardware interaction, process management, and system-level utilities for
applications requiring direct system access.

## Architecture

```
system/
├── process/             # Process management
│   ├── process.zn      # Process creation and management
│   ├── command.zn      # Command execution
│   └── pipe.zn         # Inter-process communication
├── fs/                  # File system operations
│   ├── path.zn         # Path manipulation
│   ├── file.zn         # File operations
│   └── dir.zn          # Directory operations
├── io/                  # System I/O
│   ├── stdin.zn        # Standard input
│   ├── stdout.zn       # Standard output
│   └── stderr.zn       # Standard error
├── net/                 # System-level networking
│   ├── socket.zn       # Low-level sockets
│   ├── tcp.zn          # TCP connections
│   └── udp.zn          # UDP communication
├── time/                # System time and timers
│   ├── instant.zn      # High-resolution timestamps
│   ├── duration.zn     # Time durations
│   └── timer.zn        # System timers
└── env.zn               # Environment variables and system info
```

## Key Features

### Process Management

- Process spawning and control
- Command execution with piping
- Process monitoring and signals
- Exit codes and status handling

### File System

- Path manipulation and normalization
- File creation, reading, writing
- Directory traversal and manipulation
- File permissions and metadata
- Memory-mapped files

### System I/O

- Standard streams (stdin/stdout/stderr)
- Raw terminal access
- Non-blocking I/O operations
- I/O redirection

### Networking

- Raw socket programming
- TCP/UDP server and client implementations
- Network interface enumeration
- DNS resolution

### Time Management

- High-precision system time
- Duration calculations
- Timer events and scheduling
- Time zone handling

### System Information

- Environment variables
- System properties (OS, architecture, etc.)
- CPU and memory information
- Hardware detection

## Usage Examples

### Process Execution

```cpp
use system::process::Command;

fn main() {
    let output = Command::new("ls")
        .arg("-la")
        .output()
        .expect("Failed to execute command");

    println("Output: {}", String::from_utf8(output.stdout));
}
```

### File Operations

```cpp
use system::fs::{File, Path};

fn main() {
    // Read entire file
    let content = File::read_to_string("example.txt")
        .expect("Failed to read file");

    // Write to file
    File::write("output.txt", "Hello, World!")
        .expect("Failed to write file");

    // Path manipulation
    let path = Path::new("dir/file.txt");
    println("Parent: {}", path.parent().unwrap());
    println("Extension: {}", path.extension().unwrap());
}
```

### Environment Variables

```cpp
use system::env;

fn main() {
    // Get environment variable
    if let Some(home) = env::var("HOME") {
        println("Home directory: {}", home);
    }

    // Set environment variable
    env::set_var("MY_VAR", "value");

    // Get system information
    println("OS: {}", env::consts::OS);
    println("Architecture: {}", env::consts::ARCH);
}
```

### Networking

```cpp
use system::net::{TcpListener, TcpStream};

fn server() {
    let listener = TcpListener::bind("127.0.0.1:8080")
        .expect("Failed to bind");

    for stream in listener.incoming() {
        let mut stream = stream.expect("Failed to accept");
        stream.write(b"Hello from server!");
    }
}

fn client() {
    let mut stream = TcpStream::connect("127.0.0.1:8080")
        .expect("Failed to connect");

    let mut buffer = [0; 1024];
    let bytes_read = stream.read(&mut buffer)
        .expect("Failed to read");

    println("Received: {}", String::from_utf8_lossy(&buffer[..bytes_read]));
}
```

### Time Operations

```cpp
use system::time::{Instant, Duration};

fn main() {
    let start = Instant::now();

    // Do some work
    sleep(Duration::from_secs(1));

    let elapsed = start.elapsed();
    println("Elapsed: {} seconds", elapsed.as_secs());
}
```

## Performance Characteristics

- **Zero-copy operations**: Direct system call integration
- **Buffered I/O**: Efficient file and network operations
- **Async I/O**: Non-blocking system operations
- **Memory mapping**: Direct file-to-memory mapping

## Integration with Other Modules

- **Net**: High-level networking builds on system sockets
- **Concurrency**: Process spawning and thread management
- **Memory**: Memory-mapped file support
- **Crypto**: System entropy sources for random number generation

## Safety

- Bounds checking on all buffer operations
- Safe wrappers around unsafe system calls
- Resource cleanup on scope exit
- Input validation for system operations

## Platform Support

- **Linux**: Full POSIX compliance
- **macOS**: Darwin/BSD compatibility
- **Windows**: Win32 API integration
- **Cross-platform**: Unified API across platforms

## Testing

Run the system module tests:

```bash
nova test stdlib/system/
```

Includes tests for:

- File system operations
- Process management
- Network connectivity
- Cross-platform compatibility

## Contributing

When adding system features:

1. Ensure cross-platform compatibility
2. Provide safe abstractions over unsafe operations
3. Include comprehensive error handling
4. Document platform-specific behavior
5. Test on all supported platforms
