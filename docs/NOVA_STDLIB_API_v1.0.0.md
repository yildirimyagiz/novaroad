# Nova Standard Library API Specification
# Version: 1.0.0-rc1
# Status: API FREEZE - No breaking changes allowed
# Date: 2026-02-24

# =============================================================================
# NOVA v1.0.0 STANDARD LIBRARY PUBLIC API
# =============================================================================

# This document specifies the complete public API surface of Nova's standard
# library. All interfaces listed here are guaranteed to be stable for the
# 1.0.0 release and will not break without a major version bump.

# =============================================================================
# 1. CORE TYPES
# =============================================================================

# 1.1 Primitive Types (built-in, no API surface)
# i32, i64, f32, f64, bool, str, usize, u8

# 1.2 Standard Library Types

## String Type
shape String {
    # Fields (ABI frozen)
    len: usize,
    capacity: usize,
    data: *u8,

    # Methods (API frozen)
    fn new() -> String
    fn from(s: str) -> String
    fn len(self: *String) -> usize
    fn is_empty(self: *String) -> bool
    fn push(self: *mut String, ch: u8)
    fn push_str(self: *mut String, s: str)
    fn as_str(self: *String) -> str
    fn clone(self: *String) -> String
    fn drop(self: *mut String)
}

## Vec<T> Generic Type
shape Vec<T> {
    # Fields (ABI frozen)
    len: usize,
    capacity: usize,
    data: *T,

    # Methods (API frozen)
    fn new<T>() -> Vec<T>
    fn with_capacity<T>(capacity: usize) -> Vec<T>
    fn len<T>(self: *Vec<T>) -> usize
    fn is_empty<T>(self: *Vec<T>) -> bool
    fn capacity<T>(self: *Vec<T>) -> usize
    fn push<T>(self: *mut Vec<T>, value: T)
    fn pop<T>(self: *mut Vec<T>) -> Option<T>
    fn get<T>(self: *Vec<T>, index: usize) -> Option<*T>
    fn get_mut<T>(self: *mut Vec<T>, index: usize) -> Option<*mut T>
    fn clear<T>(self: *mut Vec<T>)
    fn clone<T: Clone>(self: *Vec<T>) -> Vec<T>
    fn drop<T>(self: *mut Vec<T>)
}

## HashMap<K,V> Generic Type
shape HashMap<K: Hash + Eq, V> {
    # Fields (ABI frozen - implementation detail)
    # Implementation uses Vec for buckets, separate chaining

    # Methods (API frozen)
    fn new<K: Hash + Eq, V>() -> HashMap<K, V>
    fn insert<K: Hash + Eq, V>(self: *mut HashMap<K, V>, key: K, value: V) -> Option<V>
    fn get<K: Hash + Eq, V>(self: *HashMap<K, V>, key: *K) -> Option<*V>
    fn get_mut<K: Hash + Eq, V>(self: *mut HashMap<K, V>, key: *K) -> Option<*mut V>
    fn contains_key<K: Hash + Eq, V>(self: *HashMap<K, V>, key: *K) -> bool
    fn remove<K: Hash + Eq, V>(self: *mut HashMap<K, V>, key: *K) -> Option<V>
    fn len<K: Hash + Eq, V>(self: *HashMap<K, V>) -> usize
    fn is_empty<K: Hash + Eq, V>(self: *HashMap<K, V>) -> bool
    fn clear<K: Hash + Eq, V>(self: *mut HashMap<K, V>)
    fn clone<K: Hash + Eq + Clone, V: Clone>(self: *HashMap<K, V>) -> HashMap<K, V>
    fn drop<K: Hash + Eq, V>(self: *mut HashMap<K, V>)
}

## Option<T> Enum
enum Option<T> {
    Some(T),
    None
}

# Associated functions for Option
fn some<T>(value: T) -> Option<T>
fn none<T>() -> Option<T>
fn is_some<T>(self: *Option<T>) -> bool
fn is_none<T>(self: *Option<T>) -> bool
fn unwrap<T>(self: Option<T>) -> T  # Panics if None
fn unwrap_or<T>(self: Option<T>, default: T) -> T
fn unwrap_or_else<T, F: Fn() -> T>(self: Option<T>, f: F) -> T

## Result<T,E> Enum
enum Result<T, E> {
    Ok(T),
    Err(E)
}

# Associated functions for Result
fn ok<T, E>(value: T) -> Result<T, E>
fn err<T, E>(error: E) -> Result<T, E>
fn is_ok<T, E>(self: *Result<T, E>) -> bool
fn is_err<T, E>(self: *Result<T, E>) -> bool
fn unwrap<T, E>(self: Result<T, E>) -> T  # Panics if Err
fn unwrap_err<T, E>(self: Result<T, E>) -> E  # Panics if Ok
fn unwrap_or<T, E>(self: Result<T, E>, default: T) -> T
fn unwrap_or_else<T, E, F: Fn(E) -> T>(self: Result<T, E>, f: F) -> T

# =============================================================================
# 2. CORE TRAITS
# =============================================================================

## Hash Trait
trait Hash {
    fn hash(self: *Self) -> u64
}

## Eq Trait
trait Eq {
    fn eq(self: *Self, other: *Self) -> bool
}

## Display Trait
trait Display {
    fn format(self: *Self, buffer: *mut String)
}

## Clone Trait
trait Clone {
    fn clone(self: *Self) -> Self
}

## Drop Trait (implicit, called by runtime)
trait Drop {
    fn drop(self: *mut Self)
}

# =============================================================================
# 3. IMPLEMENTATIONS
# =============================================================================

# 3.1 Primitive Implementations

# Hash implementations
impl Hash for i32 { ... }
impl Hash for i64 { ... }
impl Hash for usize { ... }
impl Hash for u8 { ... }
impl Hash for String { ... }

# Eq implementations
impl Eq for i32 { ... }
impl Eq for i64 { ... }
impl Eq for usize { ... }
impl Eq for u8 { ... }
impl Eq for String { ... }

# Display implementations
impl Display for i32 { ... }
impl Display for i64 { ... }
impl Display for usize { ... }
impl Display for u8 { ... }
impl Display for String { ... }

# Clone implementations
impl Clone for i32 { ... }
impl Clone for i64 { ... }
impl Clone for usize { ... }
impl Clone for u8 { ... }
impl Clone for String { ... }

# 3.2 Container Implementations

# Vec<T> implementations (conditional)
impl<T: Clone> Clone for Vec<T> { ... }
impl<T> Drop for Vec<T> { ... }

# HashMap<K,V> implementations (conditional)
impl<K: Hash + Eq + Clone, V: Clone> Clone for HashMap<K, V> { ... }
impl<K: Hash + Eq, V> Drop for HashMap<K, V> { ... }

# =============================================================================
# 4. MEMORY MANAGEMENT
# =============================================================================

# 4.1 Allocation Functions
fn alloc(size: usize) -> *mut u8
fn dealloc(ptr: *mut u8, size: usize)
fn realloc(old_ptr: *mut u8, old_size: usize, new_size: usize) -> *mut u8

# 4.2 Garbage Collection (if enabled)
fn gc_collect()  # Manual GC trigger
fn gc_stats() -> GCStats  # Memory statistics

# =============================================================================
# 5. I/O FUNCTIONS
# =============================================================================

# 5.1 File I/O
fn file_open(path: str, mode: str) -> Result<File, IOError>
fn file_close(file: File)
fn file_read(file: File, buffer: *mut u8, size: usize) -> Result<usize, IOError>
fn file_write(file: File, buffer: *u8, size: usize) -> Result<usize, IOError>

# 5.2 Console I/O
fn print(s: str)
fn println(s: str)
fn eprint(s: str)
fn eprintln(s: str)

# 5.3 String Formatting
fn format(args: ...) -> String  # Variadic formatting

# =============================================================================
# 6. ERROR TYPES
# =============================================================================

# IOError enum
enum IOError {
    NotFound,
    PermissionDenied,
    IsDirectory,
    NotDirectory,
    AlreadyExists,
    InvalidInput,
    UnexpectedEof,
    Other(String)
}

# =============================================================================
# 7. UTILITY FUNCTIONS
# =============================================================================

# 7.1 Math Functions
fn abs_i32(x: i32) -> i32
fn abs_i64(x: i64) -> i64
fn min_i32(a: i32, b: i32) -> i32
fn max_i32(a: i32, b: i32) -> i32
fn min_i64(a: i64, b: i64) -> i64
fn max_i64(a: i64, b: i64) -> i64

# 7.2 Hash Functions
fn hash_u64(value: u64) -> u64
fn hash_string(s: str) -> u64
fn hash_bytes(data: *u8, len: usize) -> u64

# 7.3 Panic and Assert
fn panic(message: str)  # Terminates program
fn assert(condition: bool, message: str)  # Debug assert
fn unreachable()  # For exhaustiveness checking

# =============================================================================
# 8. COMPILER BUILT-INS
# =============================================================================

# 8.1 Memory Operations
fn size_of<T>() -> usize  # Type size in bytes
fn align_of<T>() -> usize  # Type alignment

# 8.2 Type Operations
fn type_id<T>() -> usize  # Runtime type identifier
fn type_name<T>() -> str  # Type name as string

# =============================================================================
# IMPLEMENTATION NOTES
# =============================================================================

# ABI Considerations:
# - All struct layouts are frozen
# - Enum representations are stable
# - Name mangling follows deterministic rules
# - Function calling conventions are fixed

# Compatibility Guarantees:
# - No breaking changes to public APIs
# - Backward compatibility maintained
# - Forward compatibility for new methods (via traits)

# Testing Requirements:
# - All public APIs must have tests
# - Edge cases must be covered
# - Performance regressions prevented

# =============================================================================
# END OF NOVA v1.0.0 STANDARD LIBRARY API SPECIFICATION
# =============================================================================
