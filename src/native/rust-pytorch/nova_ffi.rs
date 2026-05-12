// ═══════════════════════════════════════════════════════════════
// nova FFI Bindings - Native Tensor Operations
// Production-ready bindings to nova's C API
// ═══════════════════════════════════════════════════════════════

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;

// ═══════════════════════════════════════════════════════════════
// nova C API Declarations
// ═══════════════════════════════════════════════════════════════

/// Opaque nova types
#[repr(C)]
pub struct ZenithContext {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ZenithTensor {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ZenithGraph {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ZenithOptimizer {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ZenithGraphNode {
    _private: [u8; 0],
}

/// nova device types
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ZenithDevice {
    CPU = 0,
    CUDA = 1,
    Metal = 2,
    Vulkan = 3,
    RemoteFabric = 4,
}

/// nova data types
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ZenithDType {
    Float32 = 0,
    Float64 = 1,
    Float16 = 2,
    BFloat16 = 3,
    Int32 = 4,
    Int64 = 5,
    Int8 = 6,
    UInt8 = 7,
    Bool = 8,
}

/// nova operation types
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum ZenithOpType {
    Add = 0,
    Mul = 1,
    MatMul = 2,
    Conv2D = 3,
    ReLU = 4,
    Softmax = 5,
    CrossEntropy = 6,
    MSE = 7,
    Linear = 8,
    Dropout = 9,
    BatchNorm = 10,
    LayerNorm = 11,
}

/// nova formal verification mode
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum ZenithFormalMode {
    Disabled = 0,
    Lightweight = 1,
    Standard = 2,
    Paranoid = 3,
}

/// nova optimizer type
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum ZenithOptimizerType {
    SGD = 0,
    Adam = 1,
    AdamW = 2,
    RMSprop = 3,
    Adagrad = 4,
}

// ═══════════════════════════════════════════════════════════════
// External C Functions
// ═══════════════════════════════════════════════════════════════

#[link(name = "nova")]
extern "C" {
    // Context management
    pub fn zenith_context_create() -> *mut ZenithContext;
    pub fn zenith_context_destroy(ctx: *mut ZenithContext);
    pub fn zenith_context_set_device(ctx: *mut ZenithContext, device: ZenithDevice);
    pub fn zenith_context_get_device(ctx: *mut ZenithContext) -> ZenithDevice;
    
    // Tensor creation and destruction
    pub fn zenith_tensor_create(
        ctx: *mut ZenithContext,
        shape: *const usize,
        ndim: c_int,
        dtype: ZenithDType,
    ) -> *mut ZenithTensor;
    
    pub fn zenith_tensor_from_data(
        ctx: *mut ZenithContext,
        data: *const c_void,
        shape: *const usize,
        ndim: c_int,
        dtype: ZenithDType,
    ) -> *mut ZenithTensor;
    
    pub fn zenith_tensor_destroy(tensor: *mut ZenithTensor);
    pub fn zenith_tensor_clone(tensor: *mut ZenithTensor) -> *mut ZenithTensor;
    
    // Tensor properties
    pub fn zenith_tensor_shape(tensor: *mut ZenithTensor, out_shape: *mut usize, out_ndim: *mut c_int);
    pub fn zenith_tensor_dtype(tensor: *mut ZenithTensor) -> ZenithDType;
    pub fn zenith_tensor_numel(tensor: *mut ZenithTensor) -> usize;
    pub fn zenith_tensor_device(tensor: *mut ZenithTensor) -> ZenithDevice;
    
    // Tensor data access
    pub fn zenith_tensor_data_ptr(tensor: *mut ZenithTensor) -> *mut c_void;
    pub fn zenith_tensor_item(tensor: *mut ZenithTensor) -> f64;
    pub fn zenith_tensor_to_vec_f32(tensor: *mut ZenithTensor, out: *mut f32, len: usize);
    pub fn zenith_tensor_to_vec_f64(tensor: *mut ZenithTensor, out: *mut f64, len: usize);
    
    // Basic operations
    pub fn zenith_tensor_add(a: *mut ZenithTensor, b: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_sub(a: *mut ZenithTensor, b: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_mul(a: *mut ZenithTensor, b: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_div(a: *mut ZenithTensor, b: *mut ZenithTensor) -> *mut ZenithTensor;
    
    pub fn zenith_tensor_matmul(a: *mut ZenithTensor, b: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_transpose(tensor: *mut ZenithTensor, dim0: c_int, dim1: c_int) -> *mut ZenithTensor;
    
    // Activations
    pub fn zenith_tensor_relu(tensor: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_sigmoid(tensor: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_tanh(tensor: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_softmax(tensor: *mut ZenithTensor, dim: c_int) -> *mut ZenithTensor;
    
    // Loss functions
    pub fn zenith_cross_entropy_loss(
        input: *mut ZenithTensor,
        target: *mut ZenithTensor,
    ) -> *mut ZenithTensor;
    
    pub fn zenith_mse_loss(
        input: *mut ZenithTensor,
        target: *mut ZenithTensor,
    ) -> *mut ZenithTensor;
    
    // Autograd
    pub fn zenith_tensor_backward(tensor: *mut ZenithTensor);
    pub fn zenith_tensor_grad(tensor: *mut ZenithTensor) -> *mut ZenithTensor;
    pub fn zenith_tensor_zero_grad(tensor: *mut ZenithTensor);
    pub fn zenith_tensor_set_requires_grad(tensor: *mut ZenithTensor, requires_grad: bool);
    pub fn zenith_tensor_requires_grad(tensor: *mut ZenithTensor) -> bool;
    
    // Graph operations
    pub fn zenith_graph_create(ctx: *mut ZenithContext) -> *mut ZenithGraph;
    pub fn zenith_graph_destroy(graph: *mut ZenithGraph);
    
    pub fn zenith_graph_add_input(
        graph: *mut ZenithGraph,
        tensor: *mut ZenithTensor,
        name: *const c_char,
    ) -> *mut ZenithGraphNode;
    
    pub fn zenith_graph_add_op(
        graph: *mut ZenithGraph,
        op_type: ZenithOpType,
        inputs: *const *mut ZenithGraphNode,
        num_inputs: c_int,
        attrs: *const c_void,
    ) -> *mut ZenithGraphNode;
    
    pub fn zenith_graph_node_get_tensor(node: *mut ZenithGraphNode) -> *mut ZenithTensor;
    pub fn zenith_graph_compile(graph: *mut ZenithGraph);
    pub fn zenith_graph_execute(graph: *mut ZenithGraph);
    
    // Optimizer
    pub fn zenith_optimizer_create(
        ctx: *mut ZenithContext,
        opt_type: ZenithOptimizerType,
        params: *const *mut ZenithTensor,
        num_params: c_int,
        lr: f64,
    ) -> *mut ZenithOptimizer;
    
    pub fn zenith_optimizer_destroy(optimizer: *mut ZenithOptimizer);
    pub fn zenith_optimizer_step(optimizer: *mut ZenithOptimizer);
    pub fn zenith_optimizer_zero_grad(optimizer: *mut ZenithOptimizer);
    pub fn zenith_optimizer_set_lr(optimizer: *mut ZenithOptimizer, lr: f64);
    pub fn zenith_optimizer_get_lr(optimizer: *mut ZenithOptimizer) -> f64;
    
    // Adam-specific
    pub fn zenith_optimizer_adam_set_betas(optimizer: *mut ZenithOptimizer, beta1: f64, beta2: f64);
    pub fn zenith_optimizer_adam_set_eps(optimizer: *mut ZenithOptimizer, eps: f64);
    pub fn zenith_optimizer_adam_set_weight_decay(optimizer: *mut ZenithOptimizer, weight_decay: f64);
    
    // Formal verification
    pub fn zenith_verify_optimizer_step(
        current_lr: f64,
        prev_lr: f64,
        grad_norm: f64,
        loss: f64,
        prev_loss: f64,
    ) -> bool;
    
    pub fn zenith_verify_model_architecture(graph: *mut ZenithGraph) -> bool;
    
    pub fn zenith_proof_cache_init();
    pub fn zenith_proof_cache_destroy();
    
    // Distributed
    pub fn zenith_fabric_init(world_size: c_int, rank: c_int) -> *mut c_void;
    pub fn zenith_fabric_destroy(fabric: *mut c_void);
    pub fn zenith_fabric_broadcast(fabric: *mut c_void, tensor: *mut ZenithTensor, root: c_int);
    pub fn zenith_fabric_all_reduce(fabric: *mut c_void, tensor: *mut ZenithTensor);
    
    // Autotuner
    pub fn zenith_autotuner_create(ctx: *mut ZenithContext) -> *mut c_void;
    pub fn zenith_autotuner_destroy(tuner: *mut c_void);
    pub fn zenith_autotune_detect_arch(tuner: *mut c_void);
    pub fn zenith_autotune_batch_size(
        tuner: *mut c_void,
        graph: *mut ZenithGraph,
        memory_budget: usize,
    ) -> usize;
}

// ═══════════════════════════════════════════════════════════════
// Safe Rust Wrappers
// ═══════════════════════════════════════════════════════════════

/// Safe wrapper for ZenithContext
pub struct Context {
    ptr: *mut ZenithContext,
}

impl Context {
    pub fn new() -> Self {
        unsafe {
            let ptr = zenith_context_create();
            assert!(!ptr.is_null(), "Failed to create nova context");
            Context { ptr }
        }
    }
    
    pub fn set_device(&self, device: ZenithDevice) {
        unsafe {
            zenith_context_set_device(self.ptr, device);
        }
    }
    
    pub fn get_device(&self) -> ZenithDevice {
        unsafe {
            zenith_context_get_device(self.ptr)
        }
    }
    
    pub fn as_ptr(&self) -> *mut ZenithContext {
        self.ptr
    }
}

impl Drop for Context {
    fn drop(&mut self) {
        unsafe {
            zenith_context_destroy(self.ptr);
        }
    }
}

unsafe impl Send for Context {}
unsafe impl Sync for Context {}

/// Safe wrapper for ZenithTensor
pub struct Tensor {
    ptr: *mut ZenithTensor,
    owns_memory: bool,
}

impl Tensor {
    pub fn new(ctx: &Context, shape: &[usize], dtype: ZenithDType) -> Self {
        unsafe {
            let ptr = zenith_tensor_create(
                ctx.as_ptr(),
                shape.as_ptr(),
                shape.len() as c_int,
                dtype,
            );
            assert!(!ptr.is_null(), "Failed to create tensor");
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn from_data(ctx: &Context, data: &[f32], shape: &[usize]) -> Self {
        unsafe {
            let ptr = zenith_tensor_from_data(
                ctx.as_ptr(),
                data.as_ptr() as *const c_void,
                shape.as_ptr(),
                shape.len() as c_int,
                ZenithDType::Float32,
            );
            assert!(!ptr.is_null(), "Failed to create tensor from data");
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn shape(&self) -> Vec<usize> {
        unsafe {
            let mut shape = vec![0usize; 8];
            let mut ndim: c_int = 0;
            zenith_tensor_shape(self.ptr, shape.as_mut_ptr(), &mut ndim);
            shape.truncate(ndim as usize);
            shape
        }
    }
    
    pub fn dtype(&self) -> ZenithDType {
        unsafe {
            zenith_tensor_dtype(self.ptr)
        }
    }
    
    pub fn numel(&self) -> usize {
        unsafe {
            zenith_tensor_numel(self.ptr)
        }
    }
    
    pub fn item(&self) -> f64 {
        unsafe {
            zenith_tensor_item(self.ptr)
        }
    }
    
    pub fn to_vec_f32(&self) -> Vec<f32> {
        let numel = self.numel();
        let mut vec = vec![0.0f32; numel];
        unsafe {
            zenith_tensor_to_vec_f32(self.ptr, vec.as_mut_ptr(), numel);
        }
        vec
    }
    
    pub fn to_vec_f64(&self) -> Vec<f64> {
        let numel = self.numel();
        let mut vec = vec![0.0f64; numel];
        unsafe {
            zenith_tensor_to_vec_f64(self.ptr, vec.as_mut_ptr(), numel);
        }
        vec
    }
    
    // Operations
    pub fn add(&self, other: &Tensor) -> Tensor {
        unsafe {
            let ptr = zenith_tensor_add(self.ptr, other.ptr);
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn sub(&self, other: &Tensor) -> Tensor {
        unsafe {
            let ptr = zenith_tensor_sub(self.ptr, other.ptr);
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn mul(&self, other: &Tensor) -> Tensor {
        unsafe {
            let ptr = zenith_tensor_mul(self.ptr, other.ptr);
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn matmul(&self, other: &Tensor) -> Tensor {
        unsafe {
            let ptr = zenith_tensor_matmul(self.ptr, other.ptr);
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn relu(&self) -> Tensor {
        unsafe {
            let ptr = zenith_tensor_relu(self.ptr);
            Tensor { ptr, owns_memory: true }
        }
    }
    
    pub fn softmax(&self, dim: i32) -> Tensor {
        unsafe {
            let ptr = zenith_tensor_softmax(self.ptr, dim);
            Tensor { ptr, owns_memory: true }
        }
    }
    
    // Autograd
    pub fn backward(&self) {
        unsafe {
            zenith_tensor_backward(self.ptr);
        }
    }
    
    pub fn grad(&self) -> Option<Tensor> {
        unsafe {
            let ptr = zenith_tensor_grad(self.ptr);
            if ptr.is_null() {
                None
            } else {
                Some(Tensor { ptr, owns_memory: false })
            }
        }
    }
    
    pub fn zero_grad(&self) {
        unsafe {
            zenith_tensor_zero_grad(self.ptr);
        }
    }
    
    pub fn set_requires_grad(&self, requires_grad: bool) {
        unsafe {
            zenith_tensor_set_requires_grad(self.ptr, requires_grad);
        }
    }
    
    pub fn as_ptr(&self) -> *mut ZenithTensor {
        self.ptr
    }
}

impl Clone for Tensor {
    fn clone(&self) -> Self {
        unsafe {
            let ptr = zenith_tensor_clone(self.ptr);
            Tensor { ptr, owns_memory: true }
        }
    }
}

impl Drop for Tensor {
    fn drop(&mut self) {
        if self.owns_memory {
            unsafe {
                zenith_tensor_destroy(self.ptr);
            }
        }
    }
}

unsafe impl Send for Tensor {}
unsafe impl Sync for Tensor {}

// ═══════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════

pub fn cross_entropy_loss(input: &Tensor, target: &Tensor) -> Tensor {
    unsafe {
        let ptr = zenith_cross_entropy_loss(input.as_ptr(), target.as_ptr());
        Tensor { ptr, owns_memory: true }
    }
}

pub fn mse_loss(input: &Tensor, target: &Tensor) -> Tensor {
    unsafe {
        let ptr = zenith_mse_loss(input.as_ptr(), target.as_ptr());
        Tensor { ptr, owns_memory: true }
    }
}

/// Helper to create CString safely
pub fn c_str(s: &str) -> CString {
    CString::new(s).expect("CString creation failed")
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_context_creation() {
        let ctx = Context::new();
        assert_eq!(ctx.get_device(), ZenithDevice::CPU);
    }
    
    #[test]
    fn test_tensor_creation() {
        let ctx = Context::new();
        let tensor = Tensor::new(&ctx, &[2, 3], ZenithDType::Float32);
        assert_eq!(tensor.shape(), vec![2, 3]);
        assert_eq!(tensor.numel(), 6);
    }
    
    #[test]
    fn test_tensor_operations() {
        let ctx = Context::new();
        let a = Tensor::from_data(&ctx, &[1.0, 2.0, 3.0], &[3]);
        let b = Tensor::from_data(&ctx, &[4.0, 5.0, 6.0], &[3]);
        
        let c = a.add(&b);
        let result = c.to_vec_f32();
        assert_eq!(result, vec![5.0, 7.0, 9.0]);
    }
}
