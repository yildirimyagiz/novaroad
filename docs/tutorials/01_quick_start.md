# Quick Start Guide

Welcome to Nova! This tutorial will get you up and running in 5 minutes.

## Installation

### From Source

```bash
git clone https://github.com/nova/nova.git
cd nova
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

### Using Package Managers

```bash
# Python (PyPI)
pip install nova-ml

# JavaScript (npm)
npm install @nova/ml

# Rust (cargo)
cargo add nova-ml
```

## Your First Program

### C API

```c
#include <nova/ai/tensor.h>
#include <stdio.h>

int main() {
    printf("🚀 Nova Quick Start\n\n");
    
    // Create a simple matrix
    float data[] = {
        1, 2, 3,
        4, 5, 6
    };
    
    size_t shape[] = {2, 3};  // 2×3 matrix
    nova_tensor_t* A = nova_tensor_from_data(data, shape, 2);
    
    printf("Matrix A:\n");
    nova_tensor_print(A);
    
    // Matrix transpose
    nova_tensor_t* AT = nova_tensor_transpose(A, 0, 1);
    
    printf("\nMatrix A^T:\n");
    nova_tensor_print(AT);
    
    // Matrix multiplication: A × A^T
    nova_tensor_t* result = nova_tensor_matmul(A, AT);
    
    printf("\nA × A^T:\n");
    nova_tensor_print(result);
    
    // Cleanup
    nova_tensor_free(A);
    nova_tensor_free(AT);
    nova_tensor_free(result);
    
    return 0;
}
```

Compile:
```bash
gcc -o quick_start quick_start.c -lnova_ai -lm
./quick_start
```

Output:
```
🚀 Nova Quick Start

Matrix A:
[[1.0, 2.0, 3.0],
 [4.0, 5.0, 6.0]]

Matrix A^T:
[[1.0, 4.0],
 [2.0, 5.0],
 [3.0, 6.0]]

A × A^T:
[[14.0, 32.0],
 [32.0, 77.0]]
```

### Python API

```python
import nova

# Create tensor
A = nova.tensor([[1, 2, 3],
                 [4, 5, 6]], dtype=nova.float32)

print("Matrix A:")
print(A)

# Transpose
AT = A.T

# Matrix multiplication (auto-optimized!)
result = A @ AT

print("\nA × A^T:")
print(result)
```

## Basic Operations

### Creating Tensors

```python
import nova

# From list
a = nova.tensor([1, 2, 3, 4])

# Zeros
b = nova.zeros((3, 3))

# Ones
c = nova.ones((2, 4))

# Random
d = nova.randn((10, 10))

# Range
e = nova.arange(0, 10, 0.5)
```

### Math Operations

```python
# Element-wise
a + b          # Addition
a - b          # Subtraction
a * b          # Multiplication
a / b          # Division

# Matrix operations
a @ b          # Matrix multiplication
a.T            # Transpose

# Reductions
a.sum()        # Sum all elements
a.mean()       # Mean
a.max()        # Maximum
```

### Neural Network Layers

```python
import nova.nn as nn

# Linear layer
linear = nn.Linear(784, 128)
output = linear(input_tensor)

# Convolutional layer
conv = nn.Conv2d(3, 64, kernel_size=3)
features = conv(image)

# Attention layer
attn = nn.MultiHeadAttention(embed_dim=512, num_heads=8)
attended = attn(query, key, value)
```

## Performance Tips

### 1. Use Optimized Backends

```python
# Automatically selects best backend (CUDA, ROCm, Metal, CPU)
nova.set_device('auto')

# Or specify explicitly
nova.set_device('cuda')  # NVIDIA GPU
nova.set_device('rocm')  # AMD GPU
nova.set_device('metal') # Apple GPU
```

### 2. Enable Flash Attention

```python
# For transformers - 4-8× faster!
attn = nn.MultiHeadAttention(
    embed_dim=512,
    num_heads=8,
    use_flash=True  # ✅ Enable Flash Attention-2
)
```

### 3. Use Winograd for Convolutions

```python
# Automatically enabled for 3×3 convolutions
conv = nn.Conv2d(
    in_channels=64,
    out_channels=128,
    kernel_size=3,
    use_winograd=True  # ✅ 7-9× faster!
)
```

### 4. Quantization for Inference

```python
# Dynamic quantization (BERT/GPT)
model_int8 = nova.quantize_dynamic(model, dtype=nova.int8)

# 2-4× faster, 4× less memory
output = model_int8(input)
```

## Next Steps

- **[Tutorial 2: Build a Neural Network](02_neural_network.md)** - Train MNIST classifier
- **[Tutorial 3: ResNet Training](03_resnet.md)** - Train ResNet-50 on ImageNet
- **[Tutorial 4: BERT Fine-tuning](04_bert.md)** - Fine-tune BERT for NLP
- **[API Reference](../api/README.md)** - Complete API documentation

## Getting Help

- **[Examples](../../examples/)** - Sample code
- **[GitHub Issues](https://github.com/nova/nova/issues)** - Report bugs
- **[Discussions](https://github.com/nova/nova/discussions)** - Ask questions

---

**🎉 Congratulations! You're now ready to use Nova!**
