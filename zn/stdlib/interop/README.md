# Nova Stdlib - Interop Module

## Overview

The interop module enables seamless integration with other programming languages
and platforms, allowing Nova code to leverage existing libraries and systems.

## Architecture

```text
interop/
├── python/              # Python integration
├── c/                   # C language bindings
├── rust/                # Rust integration (planned)
├── js/                  # JavaScript/WebAssembly
├── mobile_bridge.zn     # Mobile platform bridging
└── foreign/             # Foreign function interface utilities
```

## Key Features

### 🐍 Python Integration

- **Import Python Modules**: Direct access to Python ecosystem
- **Type Conversion**: Automatic Python ↔ Nova type mapping
- **Exception Handling**: Python exceptions in Nova code
- **Performance**: Zero-copy data sharing where possible

### 🇨 C Language Bindings

- **FFI Support**: Call C functions from Nova
- **Memory Management**: Safe memory handling across boundaries
- **Header Parsing**: Automatic C header file processing
- **Callback Support**: C functions calling Nova code

### 🌐 JavaScript/WebAssembly

- **WASM Compilation**: Nova to WebAssembly compilation
- **JS Interop**: Call JavaScript from Nova, vice versa
- **DOM Access**: Web API integration
- **Browser Compatibility**: Cross-browser WebAssembly support

### 📱 Mobile Bridging

- **iOS Integration**: Swift/Objective-C interop
- **Android Integration**: Java/Kotlin interop
- **Native APIs**: Access to platform-specific features
- **UI Integration**: Native mobile UI components

## Usage Examples

### Python Integration

```cpp
import std::interop::python;

// Import Python modules
let np = python::import("numpy");
let pd = python::import("pandas");

// Use Python functions
let array = np.array([1, 2, 3, 4, 5]);
let mean = np.mean(array);
println("Mean: {}", mean);

// DataFrame operations
let df = pd.DataFrame({
    "name": ["Alice", "Bob", "Charlie"],
    "age": [25, 30, 35]
});
let avg_age = df["age"].mean();
```

### C Function Calls

```cpp
import std::interop::c;

// Load C library
let lib = c::load_library("libmath.so");

// Call C functions
let sin_func = lib.get_function("sin", fn(f64) -> f64);
let result = sin_func(3.14159 / 2.0);
println("sin(π/2) = {}", result);

// Complex C interop
#[c_binding]
extern "C" {
    fn complex_calculation(data: *const c_void, len: size_t) -> c_int;
}

let data = vec![1.0, 2.0, 3.0];
let result = complex_calculation(data.as_ptr() as *const c_void, data.len());
```

### JavaScript Interop

```cpp
import std::interop::js;

// Call JavaScript functions
let document = js::global("document");
let element = document.getElementById("myDiv");
element.innerHTML = "Hello from Nova!";

// Nova function callable from JS
#[js_export]
fn calculate_fibonacci(n: i32) -> i32 {
    if n <= 1 { return n; }
    return calculate_fibonacci(n - 1) + calculate_fibonacci(n - 2);
}

// Use in JavaScript
// const result = calculate_fibonacci(10);
```

### Mobile Platform Integration

```cpp
import std::interop::mobile_bridge;

// iOS specific code
#[cfg(target_os = "ios")]
fn ios_specific() {
    let camera = mobile_bridge::ios::camera();
    let image = camera.take_photo()?;
    // Process image...
}

// Android specific code
#[cfg(target_os = "android")]
fn android_specific() {
    let location = mobile_bridge::android::location_service();
    let coords = location.get_current_position()?;
    // Use coordinates...
}
```

## Type System Integration

### Automatic Type Conversion

```cpp
// Python types to Nova
let py_list = python::eval("[1, 2, 3]");
let zn_vec: Vec<i32> = py_list.into();  // Automatic conversion

// C types to Nova
let c_string: *const c_char = c_function_returning_string();
let zn_string: String = c::c_str_to_string(c_string);

// JavaScript types
let js_object = js::eval("({x: 10, y: 20})");
let zn_struct: Point = js_object.into();
```

## Memory Management

### Safe Cross-Language Memory

```cpp
// Python memory management
let py_array = python::numpy::zeros([1000, 1000]);
{
    // py_array is automatically managed by Python GC
    let result = process_large_array(&py_array);
} // py_array cleaned up by Python

// C memory management
let c_buffer = c::malloc(1024);
defer { c::free(c_buffer); }  // Automatic cleanup

// Use buffer safely...
c_function_using_buffer(c_buffer);
```

## Performance Considerations

| Integration Type | Overhead                | Use Case                  |
| ---------------- | ----------------------- | ------------------------- |
| Python           | Low (direct calls)      | Data science, ML          |
| C                | Very Low (native calls) | Performance-critical code |
| JavaScript       | Low (WASM)              | Web applications          |
| Mobile           | Medium (bridging)       | Platform features         |

## Error Handling

```cpp
// Python exceptions
match python::call_function("risky_operation") {
    Ok(result) => println("Success: {}", result),
    Err(e) => {
        if e.is_python_exception() {
            println("Python error: {}", e.python_traceback());
        }
    }
}

// C error handling
let result = c::call_with_error_check(|| unsafe_c_function());
if result.is_error() {
    println("C error code: {}", result.error_code());
}
```

## Security Considerations

- **Sandboxing**: Isolated execution environments
- **Type Safety**: Runtime type checking at boundaries
- **Memory Safety**: Bounds checking for foreign memory
- **Permission Model**: Access control for system resources

## Testing

```bash
# Test language interop
nova test interop/python/
nova test interop/c/
nova test interop/js/

# Integration testing
nova test interop/ --integration
```

## Platform Support

| Language   | Windows | macOS | Linux | Web       |
| ---------- | ------- | ----- | ----- | --------- |
| Python     | ✅      | ✅    | ✅    | ❌        |
| C          | ✅      | ✅    | ✅    | ✅ (WASM) |
| JavaScript | ❌      | ❌    | ❌    | ✅        |
| Rust       | 🚧      | 🚧    | 🚧    | 🚧        |

## Contributing

- **FFI Expertise**: C binding implementations needed
- **Platform Knowledge**: OS-specific integration code
- **Performance**: Minimize crossing language boundaries
- **Safety**: Comprehensive error handling and validation

## Future Expansions

- **Rust Integration**: Full Rust interop with ownership transfer
- **Java Integration**: Android Java/Kotlin bindings
- **Swift Integration**: iOS/macOS Swift interop
- **.NET Integration**: C# and F# bindings
- **WASM Extensions**: Advanced WebAssembly features
