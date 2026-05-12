# Nova NDArray

**NumPy for Nova - N-dimensional arrays with unit algebra and GPU support**

Version: 0.1.0-nova

---

## 🎯 Overview

Nova NDArray is a high-performance N-dimensional array library inspired by NumPy, but enhanced with Nova's unique features like compile-time unit checking, GPU acceleration, and type safety.

### Key Features

✅ **N-dimensional Arrays**
- Generic over element types
- Row-major (C-order) storage
- Strided arrays for efficient views
- Shape and dtype tracking

✅ **NumPy-Compatible API**
- Familiar creation functions (zeros, ones, arange, linspace)
- Element-wise operations (+, -, *, /, pow, sqrt, exp, log)
- Linear algebra (matmul, dot, transpose, inv, det)
- Reductions (sum, mean, std, min, max)
- Random number generation

✅ **Nova-Specific Advantages**
- Compile-time type safety
- Unit algebra integration (coming soon)
- GPU acceleration (planned)
- Zero-copy interop with Tensor and DataFrame

---

## 📦 Installation

```nova
use ndarray::{NDArray, zeros, ones, array};
```

---

## 🚀 Quick Start

### Creating Arrays

```nova
use ndarray::*;

// From vector
let arr = array(vec![1.0, 2.0, 3.0, 4.0]);

// Zeros
let z = zeros::<f64>(vec![3, 4]);

// Ones
let o = ones::<f64>(vec![2, 2]);

// Range
let r = arange(0.0, 10.0, 1.0);

// Linspace
let l = linspace(0.0, 1.0, 50);

// Identity matrix
let I = eye(3);

// Random
let rand_arr = rand(vec![3, 3]);
let randn_arr = randn(vec![100]);
```

### Basic Operations

```nova
let a = zeros::<f64>(vec![3, 3]);
let b = ones::<f64>(vec![3, 3]);

// Element-wise
let c = a.add(&b)?;
let d = a.mul(&b)?;
let e = a.add_scalar(5.0);
let f = a.mul_scalar(2.0);

// Math functions
let sqrt_a = a.sqrt();
let exp_a = a.exp();
let log_a = a.log();
let pow_a = a.pow(2.0);

// Shape
println!("Shape: {:?}", a.shape);
println!("Size: {}", a.size());
println!("Ndim: {}", a.ndim());
```

### Linear Algebra

```nova
// Matrix multiplication
let A = array_2d(vec![
    vec![1.0, 2.0],
    vec![3.0, 4.0],
]);

let B = array_2d(vec![
    vec![5.0, 6.0],
    vec![7.0, 8.0],
]);

let C = A.matmul(&B)?;

// Transpose
let At = A.T();

// Inverse (2x2)
let A_inv = A.inv()?;

// Determinant
let det = A.det()?;

// Trace
let tr = A.trace()?;
```

### Statistical Operations

```nova
let data = arange(0.0, 100.0, 1.0);

println!("Mean: {}", data.mean());
println!("Std: {}", data.std());
println!("Var: {}", data.var());
println!("Min: {:?}", data.min());
println!("Max: {:?}", data.max());
println!("Sum: {}", data.sum());

// Cumulative
let cumsum = data.cumsum();
let cumprod = data.cumprod();

// Indices
let min_idx = data.argmin();
let max_idx = data.argmax();
```

### Indexing and Slicing

```nova
let arr = arange(0.0, 12.0, 1.0)
    .reshape(vec![3, 4])?;

// Get element
let val = arr.get(&[1, 2])?;

// Set element
arr.set(&[1, 2], 99.0)?;

// Get row
let row = arr.row(0)?;

// Get column
let col = arr.col(2)?;

// Slice
let slice = arr.slice_1d(2, 8)?;

// Boolean mask
let mask = arr.gt(&threshold)?;
let filtered = arr.mask(&mask)?;
```

### Random Numbers

```nova
// Uniform [0, 1)
let r1 = rand(vec![3, 3]);

// Normal (mean=0, std=1)
let r2 = randn(vec![100]);

// Uniform range
let r3 = uniform(-1.0, 1.0, vec![50]);

// Normal with parameters
let r4 = normal(10.0, 2.0, vec![1000]);

// Random integers
let r5 = randint(0, 100, vec![20]);

// Shuffle
let mut arr = arange(0.0, 10.0, 1.0);
arr.shuffle();
```

---

## 📚 API Reference

### Core Types

```nova
expose data NDArray<T> {
    open data: Vec<T>,
    open shape: Vec<usize>,
    open strides: Vec<isize>,
    open dtype: DType,
}

expose cases DType {
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,
    Float32, Float64,
    Complex64, Complex128,
    Bool, Object,
}
```

### Creation Functions

- `zeros<T>(shape)` - Array of zeros
- `ones<T>(shape)` - Array of ones
- `arange(start, stop, step)` - Range array
- `linspace(start, stop, num)` - Linearly spaced
- `eye(n)` - Identity matrix
- `diag(vec)` - Diagonal matrix
- `rand(shape)` - Uniform random [0, 1)
- `randn(shape)` - Normal random (μ=0, σ=1)
- `array(vec)` - From vector
- `array_2d(vec_vec)` - 2D from nested vectors

### Operations

**Element-wise:**
- `add(&other)` - Addition
- `sub(&other)` - Subtraction
- `mul(&other)` - Multiplication
- `div(&other)` - Division
- `pow(exp)` - Power
- `sqrt()` - Square root
- `exp()` - Exponential
- `log()` - Natural logarithm

**Scalar:**
- `add_scalar(s)` - Add scalar
- `mul_scalar(s)` - Multiply by scalar

**Comparison:**
- `gt(&other)` - Greater than
- `lt(&other)` - Less than
- `eq(&other)` - Equal

### Linear Algebra

- `dot(&other)` - Dot product / matrix multiply
- `matmul(&other)` - Matrix multiplication
- `transpose()` - Transpose
- `T()` - Transpose (shorthand)
- `inv()` - Matrix inverse (2x2)
- `det()` - Determinant (2x2)
- `trace()` - Trace (sum of diagonal)

### Reductions

- `sum()` - Sum all elements
- `mean()` - Mean
- `std()` - Standard deviation
- `var()` - Variance
- `min()` - Minimum
- `max()` - Maximum
- `argmin()` - Index of minimum
- `argmax()` - Index of maximum
- `prod()` - Product
- `cumsum()` - Cumulative sum
- `cumprod()` - Cumulative product

### Shape Manipulation

- `reshape(new_shape)` - Reshape array
- `flatten()` - Flatten to 1D
- `transpose()` - Reverse dimensions

### Indexing

- `get(indices)` - Get element
- `set(indices, value)` - Set element
- `row(i)` - Get row (2D)
- `col(j)` - Get column (2D)
- `slice_1d(start, end)` - Slice 1D
- `take(indices)` - Take elements
- `mask(bool_array)` - Boolean indexing
- `diag()` - Extract diagonal

### Random

- `rand(shape)` - Uniform [0, 1)
- `randn(shape)` - Standard normal
- `randint(low, high, shape)` - Random integers
- `uniform(low, high, shape)` - Uniform distribution
- `normal(mean, std, shape)` - Normal distribution
- `shuffle()` - Shuffle in-place
- `choice(n)` - Random sample

---

## 🆚 Comparison with NumPy

| Feature | NumPy | Nova NDArray |
|---------|-------|--------------|
| **Type Safety** | Runtime | ✅ Compile-time |
| **Unit Algebra** | ❌ | ✅ Built-in (coming) |
| **GPU Support** | CuPy (separate) | ✅ Planned built-in |
| **Performance** | C backend | ✅ Native + SIMD |
| **API Compatibility** | - | ✅ NumPy-like |
| **Zero-copy** | Limited | ✅ Tensor/DataFrame |

---

## 🎨 Advanced Examples

### Matrix Operations

```nova
let A = randn(vec![100, 100]);
let b = randn(vec![100, 1]);

// Linear system: Ax = b
// (Would need LU decomposition)

// Eigenvalues (TODO)
// let (eigenvals, eigenvecs) = A.eig()?;
```

### Signal Processing

```nova
let t = linspace(0.0, 1.0, 1000);
let signal = t.mul_scalar(2.0 * PI).sin();

// FFT (TODO)
// let spectrum = signal.fft();
```

### Statistics

```nova
let data = randn(vec![10000]);

println!("Mean: {}", data.mean());
println!("Std: {}", data.std());
println!("Min: {:?}", data.min());
println!("Max: {:?}", data.max());

// Percentiles (TODO)
// let p50 = data.percentile(50.0);
// let p95 = data.percentile(95.0);
```

---

## 🚧 Roadmap

### Phase 1 - Core (✅ Implemented)
- [x] NDArray type and storage
- [x] Basic creation (zeros, ones, arange, linspace)
- [x] Element-wise operations
- [x] Reductions (sum, mean, std)
- [x] Linear algebra basics (matmul, transpose)
- [x] Random generation
- [x] Indexing and slicing

### Phase 2 - Advanced (🚧 In Progress)
- [ ] Broadcasting (partial)
- [ ] Axis-based operations
- [ ] More linear algebra (SVD, eigenvalues, QR)
- [ ] FFT (Fast Fourier Transform)
- [ ] Interpolation
- [ ] Sorting and searching

### Phase 3 - Performance (📋 Planned)
- [ ] SIMD optimizations
- [ ] GPU kernels (CUDA/Metal)
- [ ] Parallel operations
- [ ] Memory pooling
- [ ] Lazy evaluation

### Phase 4 - Integration (📋 Planned)
- [ ] Unit algebra support
- [ ] Zero-copy Tensor interop
- [ ] DataFrame column backend
- [ ] NumPy FFI (import NumPy arrays)
- [ ] I/O (.npy, .npz formats)

---

## 🔬 Implementation Status

| Module | Status | Lines | Features |
|--------|--------|-------|----------|
| core.zn | ✅ Done | ~250 | NDArray, creation, reshape |
| dtype.zn | ✅ Done | ~150 | Types, Complex |
| operations.zn | ✅ Done | ~200 | Element-wise, broadcasting |
| linalg.zn | ✅ Done | ~180 | matmul, inv, det, trace |
| reduction.zn | ✅ Done | ~150 | sum, mean, std, cumsum |
| random.zn | ✅ Done | ~150 | rand, randn, uniform, normal |
| indexing.zn | ✅ Done | ~150 | slicing, masking, row/col |
| examples.zn | ✅ Done | ~200 | 7 complete examples |
| **Total** | **✅** | **~1,430** | **Core complete** |

---

## 🎓 Learning Resources

### Examples

Run built-in examples:
```nova
use ndarray::examples::run_all_examples;

run_all_examples();
```

### NumPy Migration

NumPy users will find the API familiar:

```python
# NumPy
import numpy as np
a = np.zeros((3, 4))
b = np.random.randn(100)
c = a @ b
```

```nova
// Nova
use ndarray::*;
let a = zeros::<f64>(vec![3, 4]);
let b = randn(vec![100]);
let c = a.matmul(&b)?;
```

---

## 🤝 Contributing

Nova NDArray is part of the Nova standard library. See the main Nova repository for contribution guidelines.

---

## 📄 License

Part of the Nova programming language ecosystem.

---

**Built with Nova's powerful type system and designed for high-performance scientific computing.**
