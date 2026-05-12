// ═══════════════════════════════════════════════════════════════
// nova nova - PyTorch nova for nova
// Production-ready deep learning training framework
// ═══════════════════════════════════════════════════════════════

//! # nova nova
//! 
//! A high-level training framework for nova, inspired by PyTorch nova.
//! 
//! ## Features
//! 
//! - **Easy-to-use API**: Define your model, configure training, and go
//! - **Distributed Training**: Built-in support for DDP, FSDP, and nova Fabric
//! - **Callbacks**: ModelCheckpoint, EarlyStopping, LearningRateMonitor, etc.
//! - **Formal Verification**: Leverage nova's formal verification capabilities
//! - **Production-Ready**: Real optimizers, autograd, and hardware optimization
//! 
//! ## Quick Start
//! 
//! ```ignore
//! use zenith_nova::prelude::*;
//! 
//! // Define your model
//! struct MyModel {
//!     linear: Linear,
//! }
//! 
//! impl novaModule for MyModel {
//!     fn forward(&self, x: &Tensor) -> Tensor {
//!         self.linear.forward(x)
//!     }
//!     
//!     fn training_step(&mut self, batch: &Batch, batch_idx: usize) -> TrainingStepOutput {
//!         let y_hat = self.forward(&batch.inputs);
//!         let loss = cross_entropy_loss(&y_hat, &batch.targets);
//!         
//!         TrainingStepOutput {
//!             loss: loss.item(),
//!             logs: HashMap::new(),
//!         }
//!     }
//!     
//!     fn configure_optimizers(&self, ctx: &Context) -> Box<dyn Optimizer> {
//!         Box::new(Adam::new(ctx.clone(), self.parameters(), 0.001))
//!     }
//! }
//! 
//! // Train your model
//! fn main() {
//!     let ctx = Context::new();
//!     let model = MyModel::new();
//!     let train_loader = DataLoader::new(dataset, 32);
//!     
//!     let mut trainer = Trainer::new(ctx)
//!         .max_epochs(10)
//!         .gradient_clip_val(1.0);
//!     
//!     trainer.fit(&mut model, &train_loader, None);
//! }
//! ```

pub mod zenith_ffi;
pub mod optimizers;
pub mod trainer;
pub mod callbacks;
pub mod distributed;

// Re-exports for convenience
pub mod prelude {
    pub use crate::zenith_ffi::{
        Context, Tensor, ZenithDevice, ZenithDType,
        cross_entropy_loss, mse_loss,
    };
    
    pub use crate::optimizers::{
        Optimizer, Adam, AdamW, SGD,
    };
    
    pub use crate::trainer::{
        novaModule, Trainer, DataLoader, Dataset,
        Batch, TrainingStepOutput, ValidationStepOutput, TestStepOutput,
        quick_train, train_with_validation,
    };
    
    pub use crate::callbacks::{
        Callback, ModelCheckpoint, EarlyStopping,
        LearningRateMonitor, CheckpointMode,
    };
    
    pub use crate::distributed::{
        DistributedStrategy, DistributedConfig, DistributedTrainer,
        ZenithFabric, launch_distributed,
    };
    
    pub use std::collections::HashMap;
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_library_imports() {
        // Test that all modules are accessible
        use prelude::*;
        
        let ctx = Context::new();
        assert_eq!(ctx.get_device(), ZenithDevice::CPU);
    }
}
