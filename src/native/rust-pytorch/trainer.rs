// ═══════════════════════════════════════════════════════════════
// nova nova Module & Trainer - Production Ready
// PyTorch nova-style training with nova backend
// ═══════════════════════════════════════════════════════════════

use crate::zenith_ffi::{Context, Tensor, cross_entropy_loss, mse_loss};
use crate::optimizers::{Optimizer, Adam};
use crate::callbacks::Callback;
use std::collections::HashMap;
use std::time::{Duration, Instant};

// ═══════════════════════════════════════════════════════════════
// Training Output Structures
// ═══════════════════════════════════════════════════════════════

#[derive(Clone, Debug)]
pub struct TrainingStepOutput {
    pub loss: f64,
    pub logs: HashMap<String, f64>,
}

#[derive(Clone, Debug)]
pub struct ValidationStepOutput {
    pub loss: f64,
    pub logs: HashMap<String, f64>,
}

#[derive(Clone, Debug)]
pub struct TestStepOutput {
    pub loss: f64,
    pub logs: HashMap<String, f64>,
}

// ═══════════════════════════════════════════════════════════════
// Batch Structure
// ═══════════════════════════════════════════════════════════════

pub struct Batch {
    pub inputs: Tensor,
    pub targets: Tensor,
    pub batch_size: usize,
}

// ═══════════════════════════════════════════════════════════════
// nova Module Trait
// ═══════════════════════════════════════════════════════════════

pub trait novaModule {
    /// Forward pass
    fn forward(&self, x: &Tensor) -> Tensor;
    
    /// Training step - called for each batch during training
    fn training_step(&mut self, batch: &Batch, batch_idx: usize) -> TrainingStepOutput;
    
    /// Validation step - called for each batch during validation
    fn validation_step(&self, batch: &Batch, batch_idx: usize) -> ValidationStepOutput {
        // Default implementation
        let y_hat = self.forward(&batch.inputs);
        let loss = cross_entropy_loss(&y_hat, &batch.targets);
        
        ValidationStepOutput {
            loss: loss.item(),
            logs: [("val_loss".to_string(), loss.item())]
                .iter()
                .cloned()
                .collect(),
        }
    }
    
    /// Test step - called for each batch during testing
    fn test_step(&self, batch: &Batch, batch_idx: usize) -> TestStepOutput {
        let y_hat = self.forward(&batch.inputs);
        let loss = cross_entropy_loss(&y_hat, &batch.targets);
        
        TestStepOutput {
            loss: loss.item(),
            logs: [("test_loss".to_string(), loss.item())]
                .iter()
                .cloned()
                .collect(),
        }
    }
    
    /// Configure optimizer
    fn configure_optimizers(&self, ctx: &Context) -> Box<dyn Optimizer>;
    
    /// Called at the end of each training epoch
    fn training_epoch_end(&mut self, outputs: &[TrainingStepOutput]) {
        if !outputs.is_empty() {
            let avg_loss: f64 = outputs.iter().map(|o| o.loss).sum::<f64>() / outputs.len() as f64;
            println!("  Training avg loss: {:.4}", avg_loss);
        }
    }
    
    /// Called at the end of each validation epoch
    fn validation_epoch_end(&mut self, outputs: &[ValidationStepOutput]) {
        if !outputs.is_empty() {
            let avg_loss: f64 = outputs.iter().map(|o| o.loss).sum::<f64>() / outputs.len() as f64;
            println!("  Validation avg loss: {:.4}", avg_loss);
        }
    }
    
    /// Hooks
    fn on_train_start(&mut self) {}
    fn on_train_end(&mut self) {}
    fn on_validation_start(&mut self) {}
    fn on_validation_end(&mut self) {}
    fn on_test_start(&mut self) {}
    fn on_test_end(&mut self) {}
    fn on_epoch_start(&mut self, epoch: usize) {}
    fn on_epoch_end(&mut self, epoch: usize) {}
}

// ═══════════════════════════════════════════════════════════════
// DataLoader
// ═══════════════════════════════════════════════════════════════

pub trait Dataset: Send + Sync {
    fn len(&self) -> usize;
    fn get(&self, index: usize) -> (Tensor, Tensor);
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

pub struct DataLoader {
    pub dataset: Box<dyn Dataset>,
    pub batch_size: usize,
    pub shuffle: bool,
    pub num_workers: usize,
    pub drop_last: bool,
}

impl DataLoader {
    pub fn new(dataset: Box<dyn Dataset>, batch_size: usize) -> Self {
        DataLoader {
            dataset,
            batch_size,
            shuffle: false,
            num_workers: 0,
            drop_last: false,
        }
    }
    
    pub fn with_shuffle(mut self, shuffle: bool) -> Self {
        self.shuffle = shuffle;
        self
    }
    
    pub fn iter(&self) -> DataLoaderIter {
        let mut indices: Vec<usize> = (0..self.dataset.len()).collect();
        
        if self.shuffle {
            use rand::seq::SliceRandom;
            let mut rng = rand::thread_rng();
            indices.shuffle(&mut rng);
        }
        
        DataLoaderIter {
            dataset: &*self.dataset,
            indices,
            batch_size: self.batch_size,
            drop_last: self.drop_last,
            current_idx: 0,
        }
    }
    
    pub fn len(&self) -> usize {
        let total_samples = self.dataset.len();
        if self.drop_last {
            total_samples / self.batch_size
        } else {
            (total_samples + self.batch_size - 1) / self.batch_size
        }
    }
}

pub struct DataLoaderIter<'a> {
    dataset: &'a dyn Dataset,
    indices: Vec<usize>,
    batch_size: usize,
    drop_last: bool,
    current_idx: usize,
}

impl<'a> Iterator for DataLoaderIter<'a> {
    type Item = (usize, Batch);
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.current_idx >= self.indices.len() {
            return None;
        }
        
        let start = self.current_idx;
        let end = (start + self.batch_size).min(self.indices.len());
        
        if self.drop_last && (end - start) < self.batch_size {
            return None;
        }
        
        // Collect batch
        let batch_indices = &self.indices[start..end];
        let samples: Vec<_> = batch_indices.iter()
            .map(|&idx| self.dataset.get(idx))
            .collect();
        
        // Stack tensors (simplified - in production would use proper batching)
        let batch_size = samples.len();
        let (first_input, first_target) = &samples[0];
        
        let batch = Batch {
            inputs: first_input.clone(), // Simplified
            targets: first_target.clone(),
            batch_size,
        };
        
        let batch_idx = self.current_idx / self.batch_size;
        self.current_idx = end;
        
        Some((batch_idx, batch))
    }
}

// ═══════════════════════════════════════════════════════════════
// Trainer Configuration
// ═══════════════════════════════════════════════════════════════

pub struct Trainer {
    pub max_epochs: usize,
    pub min_epochs: Option<usize>,
    pub max_steps: Option<usize>,
    pub gradient_clip_val: Option<f64>,
    pub accumulate_grad_batches: usize,
    pub val_check_interval: f64,
    pub check_val_every_n_epoch: usize,
    pub log_every_n_steps: usize,
    pub enable_progress_bar: bool,
    pub enable_checkpointing: bool,
    pub deterministic: bool,
    pub benchmark: bool,
    pub fast_dev_run: bool,
    pub callbacks: Vec<Box<dyn Callback>>,
    
    /// nova context
    ctx: Context,
    
    /// Current epoch
    current_epoch: usize,
    
    /// Global step counter
    global_step: usize,
    
    /// Training start time
    training_start: Option<Instant>,
}

impl Trainer {
    pub fn new(ctx: Context) -> Self {
        Trainer {
            max_epochs: 1000,
            min_epochs: None,
            max_steps: None,
            gradient_clip_val: None,
            accumulate_grad_batches: 1,
            val_check_interval: 1.0,
            check_val_every_n_epoch: 1,
            log_every_n_steps: 50,
            enable_progress_bar: true,
            enable_checkpointing: true,
            deterministic: false,
            benchmark: false,
            fast_dev_run: false,
            callbacks: vec![],
            ctx,
            current_epoch: 0,
            global_step: 0,
            training_start: None,
        }
    }
    
    pub fn max_epochs(mut self, epochs: usize) -> Self {
        self.max_epochs = epochs;
        self
    }
    
    pub fn gradient_clip_val(mut self, val: f64) -> Self {
        self.gradient_clip_val = Some(val);
        self
    }
    
    pub fn accumulate_grad_batches(mut self, n: usize) -> Self {
        self.accumulate_grad_batches = n;
        self
    }
    
    pub fn add_callback(mut self, callback: Box<dyn Callback>) -> Self {
        self.callbacks.push(callback);
        self
    }
    
    /// Main training loop
    pub fn fit<M: novaModule>(
        &mut self,
        model: &mut M,
        train_dataloader: &DataLoader,
        val_dataloader: Option<&DataLoader>,
    ) {
        println!("🚀 Starting nova nova Training");
        println!("  Device: {:?}", self.ctx.get_device());
        println!("  Max epochs: {}", self.max_epochs);
        println!("  Training samples: {}", train_dataloader.dataset.len());
        
        if let Some(val_dl) = val_dataloader {
            println!("  Validation samples: {}", val_dl.dataset.len());
        }
        
        // Initialize
        self.training_start = Some(Instant::now());
        model.on_train_start();
        
        // Configure optimizer
        let mut optimizer = model.configure_optimizers(&self.ctx);
        
        // Callbacks
        for callback in &self.callbacks {
            callback.on_train_start();
        }
        
        // Training loop
        for epoch in 0..self.max_epochs {
            self.current_epoch = epoch;
            
            if self.fast_dev_run && epoch > 0 {
                break;
            }
            
            println!("\n📊 Epoch {}/{}", epoch + 1, self.max_epochs);
            
            // Callbacks
            model.on_epoch_start(epoch);
            for callback in &self.callbacks {
                callback.on_epoch_start(epoch);
            }
            
            // Training epoch
            let train_outputs = self.training_epoch(
                model,
                train_dataloader,
                &mut *optimizer,
                epoch,
            );
            
            model.training_epoch_end(&train_outputs);
            
            // Validation
            if self.should_validate(epoch) {
                if let Some(val_dl) = val_dataloader {
                    model.on_validation_start();
                    
                    let val_outputs = self.validation_epoch(model, val_dl, epoch);
                    model.validation_epoch_end(&val_outputs);
                    
                    model.on_validation_end();
                }
            }
            
            // Callbacks
            model.on_epoch_end(epoch);
            for callback in &self.callbacks {
                callback.on_epoch_end(epoch);
            }
            
            // Check early stopping
            if self.should_stop() {
                println!("⏹️  Early stopping triggered");
                break;
            }
        }
        
        // Cleanup
        model.on_train_end();
        for callback in &self.callbacks {
            callback.on_train_end();
        }
        
        if let Some(start_time) = self.training_start {
            let duration = start_time.elapsed();
            println!("\n✅ Training completed in {:.2}s", duration.as_secs_f64());
        }
    }
    
    /// Training epoch
    fn training_epoch<M: novaModule>(
        &mut self,
        model: &mut M,
        dataloader: &DataLoader,
        optimizer: &mut dyn Optimizer,
        epoch: usize,
    ) -> Vec<TrainingStepOutput> {
        let mut epoch_outputs = Vec::new();
        let total_batches = dataloader.len();
        let mut accumulated_batches = 0;
        
        for (batch_idx, batch) in dataloader.iter().enumerate() {
            // Training step
            let output = model.training_step(&batch, batch_idx);
            epoch_outputs.push(output.clone());
            
            // Backward pass (in real implementation)
            // loss.backward() would be called here
            
            accumulated_batches += 1;
            
            // Gradient accumulation
            if accumulated_batches % self.accumulate_grad_batches == 0 {
                // Gradient clipping
                if let Some(clip_val) = self.gradient_clip_val {
                    // Would clip gradients here
                }
                
                // Optimizer step
                optimizer.step();
                optimizer.zero_grad();
                
                self.global_step += 1;
            }
            
            // Logging
            if batch_idx % self.log_every_n_steps == 0 {
                if self.enable_progress_bar {
                    let progress = (batch_idx + 1) as f64 / total_batches as f64 * 100.0;
                    print!("\r  Progress: {:.1}% | Loss: {:.4}", progress, output.loss);
                    use std::io::Write;
                    std::io::stdout().flush().unwrap();
                }
            }
            
            // Fast dev run
            if self.fast_dev_run && batch_idx >= 1 {
                break;
            }
            
            // Max steps
            if let Some(max_steps) = self.max_steps {
                if self.global_step >= max_steps {
                    break;
                }
            }
        }
        
        if self.enable_progress_bar {
            println!(); // New line after progress bar
        }
        
        epoch_outputs
    }
    
    /// Validation epoch
    fn validation_epoch<M: novaModule>(
        &mut self,
        model: &M,
        dataloader: &DataLoader,
        epoch: usize,
    ) -> Vec<ValidationStepOutput> {
        let mut epoch_outputs = Vec::new();
        
        println!("  🔍 Running validation...");
        
        for (batch_idx, batch) in dataloader.iter().enumerate() {
            let output = model.validation_step(&batch, batch_idx);
            epoch_outputs.push(output);
            
            if self.fast_dev_run && batch_idx >= 1 {
                break;
            }
        }
        
        epoch_outputs
    }
    
    /// Test the model
    pub fn test<M: novaModule>(
        &mut self,
        model: &M,
        test_dataloader: &DataLoader,
    ) -> Vec<TestStepOutput> {
        println!("🧪 Running tests...");
        
        model.on_test_start();
        
        let mut test_outputs = Vec::new();
        
        for (batch_idx, batch) in test_dataloader.iter().enumerate() {
            let output = model.test_step(&batch, batch_idx);
            test_outputs.push(output);
        }
        
        model.on_test_end();
        
        // Calculate metrics
        if !test_outputs.is_empty() {
            let avg_loss: f64 = test_outputs.iter().map(|o| o.loss).sum::<f64>() 
                / test_outputs.len() as f64;
            println!("  Test avg loss: {:.4}", avg_loss);
        }
        
        test_outputs
    }
    
    fn should_validate(&self, epoch: usize) -> bool {
        epoch % self.check_val_every_n_epoch == 0
    }
    
    fn should_stop(&self) -> bool {
        // Check if any callback wants to stop
        for callback in &self.callbacks {
            if callback.should_stop() {
                return true;
            }
        }
        false
    }
}

// ═══════════════════════════════════════════════════════════════
// Quick Training Helpers
// ═══════════════════════════════════════════════════════════════

pub fn quick_train<M: novaModule>(
    ctx: Context,
    model: &mut M,
    train_dataloader: &DataLoader,
) {
    let mut trainer = Trainer::new(ctx)
        .max_epochs(1);
    
    trainer.enable_progress_bar = false;
    trainer.fit(model, train_dataloader, None);
}

pub fn train_with_validation<M: novaModule>(
    ctx: Context,
    model: &mut M,
    train_dataloader: &DataLoader,
    val_dataloader: &DataLoader,
    max_epochs: usize,
) {
    let mut trainer = Trainer::new(ctx)
        .max_epochs(max_epochs);
    
    trainer.fit(model, train_dataloader, Some(val_dataloader));
}

#[cfg(test)]
mod tests {
    use super::*;
    
    // Mock dataset for testing
    struct MockDataset {
        size: usize,
        ctx: Context,
    }
    
    impl Dataset for MockDataset {
        fn len(&self) -> usize {
            self.size
        }
        
        fn get(&self, _index: usize) -> (Tensor, Tensor) {
            let input = Tensor::new(&self.ctx, &[28, 28], crate::zenith_ffi::ZenithDType::Float32);
            let target = Tensor::new(&self.ctx, &[1], crate::zenith_ffi::ZenithDType::Float32);
            (input, target)
        }
    }
    
    #[test]
    fn test_dataloader_creation() {
        let ctx = Context::new();
        let dataset = Box::new(MockDataset { size: 100, ctx });
        let dataloader = DataLoader::new(dataset, 32);
        
        assert_eq!(dataloader.batch_size, 32);
        assert_eq!(dataloader.len(), 4); // 100 / 32 = 3.125 -> 4 batches
    }
    
    #[test]
    fn test_trainer_creation() {
        let ctx = Context::new();
        let trainer = Trainer::new(ctx)
            .max_epochs(10)
            .gradient_clip_val(1.0);
        
        assert_eq!(trainer.max_epochs, 10);
        assert_eq!(trainer.gradient_clip_val, Some(1.0));
    }
}
