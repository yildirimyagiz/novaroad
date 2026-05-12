// ═══════════════════════════════════════════════════════════════
// Complete MNIST Training Example with nova nova
// Shows: Model definition, training, validation, callbacks
// ═══════════════════════════════════════════════════════════════

use zenith_nova::prelude::*;
use std::collections::HashMap;

// ═══════════════════════════════════════════════════════════════
// MNIST Model Definition
// ═══════════════════════════════════════════════════════════════

struct MNISTModel {
    ctx: Context,
    
    // Layers (simplified - in production would use actual nova layers)
    layer1_weight: Tensor,
    layer1_bias: Tensor,
    
    layer2_weight: Tensor,
    layer2_bias: Tensor,
    
    layer3_weight: Tensor,
    layer3_bias: Tensor,
}

impl MNISTModel {
    fn new(ctx: Context) -> Self {
        // Initialize model parameters
        let layer1_weight = Tensor::new(&ctx, &[128, 784], ZenithDType::Float32);
        let layer1_bias = Tensor::new(&ctx, &[128], ZenithDType::Float32);
        
        let layer2_weight = Tensor::new(&ctx, &[256, 128], ZenithDType::Float32);
        let layer2_bias = Tensor::new(&ctx, &[256], ZenithDType::Float32);
        
        let layer3_weight = Tensor::new(&ctx, &[10, 256], ZenithDType::Float32);
        let layer3_bias = Tensor::new(&ctx, &[10], ZenithDType::Float32);
        
        // Set requires_grad
        layer1_weight.set_requires_grad(true);
        layer1_bias.set_requires_grad(true);
        layer2_weight.set_requires_grad(true);
        layer2_bias.set_requires_grad(true);
        layer3_weight.set_requires_grad(true);
        layer3_bias.set_requires_grad(true);
        
        MNISTModel {
            ctx,
            layer1_weight,
            layer1_bias,
            layer2_weight,
            layer2_bias,
            layer3_weight,
            layer3_bias,
        }
    }
    
    fn parameters(&self) -> Vec<Tensor> {
        vec![
            self.layer1_weight.clone(),
            self.layer1_bias.clone(),
            self.layer2_weight.clone(),
            self.layer2_bias.clone(),
            self.layer3_weight.clone(),
            self.layer3_bias.clone(),
        ]
    }
}

impl novaModule for MNISTModel {
    fn forward(&self, x: &Tensor) -> Tensor {
        // Layer 1: Linear + ReLU
        let z1 = x.matmul(&self.layer1_weight.transpose(0, 1))
            .add(&self.layer1_bias);
        let a1 = z1.relu();
        
        // Layer 2: Linear + ReLU
        let z2 = a1.matmul(&self.layer2_weight.transpose(0, 1))
            .add(&self.layer2_bias);
        let a2 = z2.relu();
        
        // Layer 3: Linear (output)
        let z3 = a2.matmul(&self.layer3_weight.transpose(0, 1))
            .add(&self.layer3_bias);
        
        z3
    }
    
    fn training_step(&mut self, batch: &Batch, batch_idx: usize) -> TrainingStepOutput {
        // Forward pass
        let logits = self.forward(&batch.inputs);
        
        // Compute loss
        let loss = cross_entropy_loss(&logits, &batch.targets);
        let loss_val = loss.item();
        
        // Backward pass (in real implementation)
        loss.backward();
        
        // Compute accuracy
        let predictions = logits.softmax(1);
        let accuracy = self.compute_accuracy(&predictions, &batch.targets);
        
        let mut logs = HashMap::new();
        logs.insert("train_loss".to_string(), loss_val);
        logs.insert("train_acc".to_string(), accuracy);
        
        TrainingStepOutput {
            loss: loss_val,
            logs,
        }
    }
    
    fn validation_step(&self, batch: &Batch, batch_idx: usize) -> ValidationStepOutput {
        let logits = self.forward(&batch.inputs);
        let loss = cross_entropy_loss(&logits, &batch.targets);
        let loss_val = loss.item();
        
        let predictions = logits.softmax(1);
        let accuracy = self.compute_accuracy(&predictions, &batch.targets);
        
        let mut logs = HashMap::new();
        logs.insert("val_loss".to_string(), loss_val);
        logs.insert("val_acc".to_string(), accuracy);
        
        ValidationStepOutput {
            loss: loss_val,
            logs,
        }
    }
    
    fn configure_optimizers(&self, ctx: &Context) -> Box<dyn Optimizer> {
        // Create Adam optimizer with weight decay
        let params = self.parameters();
        let adam = Adam::with_config(
            ctx.clone(),
            params,
            0.001,  // lr
            0.9,    // beta1
            0.999,  // beta2
            1e-8,   // eps
            0.01,   // weight_decay
            false,  // amsgrad
        );
        
        Box::new(adam)
    }
    
    fn training_epoch_end(&mut self, outputs: &[TrainingStepOutput]) {
        if !outputs.is_empty() {
            let avg_loss: f64 = outputs.iter().map(|o| o.loss).sum::<f64>() / outputs.len() as f64;
            let avg_acc: f64 = outputs.iter()
                .filter_map(|o| o.logs.get("train_acc"))
                .sum::<f64>() / outputs.len() as f64;
            
            println!("  📊 Training - Loss: {:.4}, Acc: {:.2}%", avg_loss, avg_acc * 100.0);
        }
    }
    
    fn validation_epoch_end(&mut self, outputs: &[ValidationStepOutput]) {
        if !outputs.is_empty() {
            let avg_loss: f64 = outputs.iter().map(|o| o.loss).sum::<f64>() / outputs.len() as f64;
            let avg_acc: f64 = outputs.iter()
                .filter_map(|o| o.logs.get("val_acc"))
                .sum::<f64>() / outputs.len() as f64;
            
            println!("  🔍 Validation - Loss: {:.4}, Acc: {:.2}%", avg_loss, avg_acc * 100.0);
        }
    }
}

impl MNISTModel {
    fn compute_accuracy(&self, predictions: &Tensor, targets: &Tensor) -> f64 {
        // Simplified accuracy computation
        // In production, would properly compute accuracy
        0.95 // Mock accuracy
    }
}

// ═══════════════════════════════════════════════════════════════
// Mock MNIST Dataset
// ═══════════════════════════════════════════════════════════════

struct MNISTDataset {
    ctx: Context,
    num_samples: usize,
    is_train: bool,
}

impl MNISTDataset {
    fn new(ctx: Context, is_train: bool) -> Self {
        let num_samples = if is_train { 60000 } else { 10000 };
        MNISTDataset {
            ctx,
            num_samples,
            is_train,
        }
    }
}

impl Dataset for MNISTDataset {
    fn len(&self) -> usize {
        self.num_samples
    }
    
    fn get(&self, index: usize) -> (Tensor, Tensor) {
        // Mock data generation
        // In production, would load actual MNIST images
        let image = Tensor::new(&self.ctx, &[784], ZenithDType::Float32);
        let label = Tensor::new(&self.ctx, &[1], ZenithDType::Float32);
        
        (image, label)
    }
}

// ═══════════════════════════════════════════════════════════════
// Main Training Script
// ═══════════════════════════════════════════════════════════════

fn main() {
    println!("🚀 nova nova - MNIST Training Example\n");
    
    // Create context
    let ctx = Context::new();
    println!("✅ Context created - Device: {:?}", ctx.get_device());
    
    // Create datasets
    let train_dataset = Box::new(MNISTDataset::new(ctx.clone(), true));
    let val_dataset = Box::new(MNISTDataset::new(ctx.clone(), false));
    
    println!("✅ Datasets loaded");
    println!("  Training samples: {}", train_dataset.len());
    println!("  Validation samples: {}", val_dataset.len());
    
    // Create data loaders
    let train_loader = DataLoader::new(train_dataset, 128).with_shuffle(true);
    let val_loader = DataLoader::new(val_dataset, 256);
    
    println!("✅ DataLoaders created");
    println!("  Training batches: {}", train_loader.len());
    println!("  Validation batches: {}\n", val_loader.len());
    
    // Create model
    let mut model = MNISTModel::new(ctx.clone());
    println!("✅ Model created\n");
    
    // Create callbacks
    let checkpoint = ModelCheckpoint::new("./checkpoints", "val_loss")
        .save_top_k(3)
        .mode(CheckpointMode::Min)
        .every_n_epochs(1);
    
    let early_stopping = EarlyStopping::new("val_loss", 5)
        .min_delta(0.001)
        .verbose(true);
    
    let lr_monitor = LearningRateMonitor::new();
    
    // Create trainer
    let mut trainer = Trainer::new(ctx)
        .max_epochs(10)
        .gradient_clip_val(1.0)
        .accumulate_grad_batches(1)
        .add_callback(Box::new(checkpoint))
        .add_callback(Box::new(early_stopping))
        .add_callback(Box::new(lr_monitor));
    
    println!("✅ Trainer configured");
    println!("  Max epochs: 10");
    println!("  Gradient clipping: 1.0");
    println!("  Callbacks: ModelCheckpoint, EarlyStopping, LRMonitor\n");
    
    // Train!
    println!("{'='*60}");
    println!("STARTING TRAINING");
    println!("{'='*60}\n");
    
    trainer.fit(&mut model, &train_loader, Some(&val_loader));
    
    println!("\n{'='*60}");
    println!("TRAINING COMPLETE!");
    println!("{'='*60}\n");
    
    // Test the model
    println!("🧪 Running final test...\n");
    let test_dataset = Box::new(MNISTDataset::new(Context::new(), false));
    let test_loader = DataLoader::new(test_dataset, 256);
    
    let test_outputs = trainer.test(&model, &test_loader);
    
    if !test_outputs.is_empty() {
        let avg_loss: f64 = test_outputs.iter().map(|o| o.loss).sum::<f64>() 
            / test_outputs.len() as f64;
        println!("✅ Test complete - Avg Loss: {:.4}\n", avg_loss);
    }
    
    println!("{'='*60}");
    println!("ALL DONE! 🎉");
    println!("{'='*60}");
}
