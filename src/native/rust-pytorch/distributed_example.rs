// ═══════════════════════════════════════════════════════════════
// Distributed Training Example - Multi-GPU with DDP
// Shows: Distributed setup, DDP training, multi-node support
// ═══════════════════════════════════════════════════════════════

use zenith_nova::prelude::*;
use std::collections::HashMap;

// ═══════════════════════════════════════════════════════════════
// Simple Model for Distributed Training
// ═══════════════════════════════════════════════════════════════

struct DistributedModel {
    ctx: Context,
    weight: Tensor,
    bias: Tensor,
}

impl DistributedModel {
    fn new(ctx: Context) -> Self {
        let weight = Tensor::new(&ctx, &[256, 128], ZenithDType::Float32);
        let bias = Tensor::new(&ctx, &[256], ZenithDType::Float32);
        
        weight.set_requires_grad(true);
        bias.set_requires_grad(true);
        
        DistributedModel {
            ctx,
            weight,
            bias,
        }
    }
    
    fn parameters(&self) -> Vec<Tensor> {
        vec![self.weight.clone(), self.bias.clone()]
    }
}

impl novaModule for DistributedModel {
    fn forward(&self, x: &Tensor) -> Tensor {
        x.matmul(&self.weight.transpose(0, 1)).add(&self.bias)
    }
    
    fn training_step(&mut self, batch: &Batch, batch_idx: usize) -> TrainingStepOutput {
        let output = self.forward(&batch.inputs);
        let loss = mse_loss(&output, &batch.targets);
        let loss_val = loss.item();
        
        loss.backward();
        
        let mut logs = HashMap::new();
        logs.insert("train_loss".to_string(), loss_val);
        
        TrainingStepOutput {
            loss: loss_val,
            logs,
        }
    }
    
    fn configure_optimizers(&self, ctx: &Context) -> Box<dyn Optimizer> {
        Box::new(Adam::new(ctx.clone(), self.parameters(), 0.001))
    }
}

// ═══════════════════════════════════════════════════════════════
// Mock Dataset
// ═══════════════════════════════════════════════════════════════

struct MockDataset {
    ctx: Context,
    size: usize,
}

impl Dataset for MockDataset {
    fn len(&self) -> usize {
        self.size
    }
    
    fn get(&self, _index: usize) -> (Tensor, Tensor) {
        let input = Tensor::new(&self.ctx, &[128], ZenithDType::Float32);
        let target = Tensor::new(&self.ctx, &[256], ZenithDType::Float32);
        (input, target)
    }
}

// ═══════════════════════════════════════════════════════════════
// Main - Single Node Multi-GPU Training
// ═══════════════════════════════════════════════════════════════

fn main() {
    println!("🌐 nova nova - Distributed Training Example\n");
    
    // Detect number of GPUs (mock - in production would detect actual GPUs)
    let num_gpus = 4;
    println!("✅ Detected {} GPUs\n", num_gpus);
    
    // Create distributed configuration
    let config = DistributedConfig::single_node(num_gpus);
    
    println!("{'='*60}");
    println!("DISTRIBUTED CONFIGURATION");
    println!("{'='*60}");
    println!("  Strategy: DDP (Distributed Data Parallel)");
    println!("  Backend: nova Fabric");
    println!("  World size: {}", config.world_size);
    println!("  Number of nodes: {}", config.num_nodes);
    println!("  Master addr: {}", config.master_addr);
    println!("  Master port: {}", config.master_port);
    println!("{'='*60}\n");
    
    // Train on each GPU (in production, this would spawn processes)
    train_on_rank(0, config.clone());
    
    println!("\n{'='*60}");
    println!("DISTRIBUTED TRAINING COMPLETE!");
    println!("{'='*60}");
}

fn train_on_rank(rank: usize, config: DistributedConfig) {
    println!("[Rank {}] Initializing training process...", rank);
    
    // Create context with GPU
    let ctx = Context::new();
    ctx.set_device(ZenithDevice::CUDA);
    println!("[Rank {}] Context created - Device: {:?}", rank, ctx.get_device());
    
    // Create dataset (each rank gets full dataset, DDP handles sharding)
    let train_dataset = Box::new(MockDataset {
        ctx: ctx.clone(),
        size: 10000,
    });
    
    let train_loader = DataLoader::new(train_dataset, 64).with_shuffle(true);
    println!("[Rank {}] DataLoader created - {} batches", rank, train_loader.len());
    
    // Create model
    let model = DistributedModel::new(ctx.clone());
    println!("[Rank {}] Model created", rank);
    
    // Create distributed trainer
    let mut trainer = DistributedTrainer::new(
        ctx,
        DistributedStrategy::DDP,
        config,
    )
    .max_epochs(5)
    .gradient_clip_val(1.0);
    
    println!("[Rank {}] Trainer configured", rank);
    
    // Train!
    if rank == 0 {
        println!("\n{'='*60}");
        println!("STARTING DISTRIBUTED TRAINING");
        println!("{'='*60}\n");
    }
    
    trainer.fit(model, &train_loader, None);
    
    if rank == 0 {
        println!("\n[Rank 0] Training complete! 🎉");
    }
}

// ═══════════════════════════════════════════════════════════════
// Alternative: Launch with Spawner
// ═══════════════════════════════════════════════════════════════

#[allow(dead_code)]
fn alternative_launch_method() {
    println!("🚀 Launching distributed training with spawner...\n");
    
    // This would spawn multiple processes automatically
    launch_distributed(4, || {
        let ctx = Context::new();
        DistributedModel::new(ctx)
    });
}

// ═══════════════════════════════════════════════════════════════
// Multi-Node Training Example
// ═══════════════════════════════════════════════════════════════

#[allow(dead_code)]
fn multi_node_example() {
    println!("🌐 Multi-Node Distributed Training\n");
    
    // Configuration for 2 nodes, 4 GPUs each = 8 total processes
    let config = DistributedConfig {
        world_size: 8,
        rank: 0, // Set by launch script
        local_rank: 0,
        num_nodes: 2,
        backend: DistributedBackend::ZenithFabric,
        master_addr: "node1.cluster.local".to_string(),
        master_port: 29500,
        timeout_seconds: 1800,
    };
    
    println!("Configuration:");
    println!("  Nodes: {}", config.num_nodes);
    println!("  Total GPUs: {}", config.world_size);
    println!("  GPUs per node: {}", config.world_size / config.num_nodes);
    println!("  Master: {}:{}", config.master_addr, config.master_port);
    
    // Training would proceed same as single-node
}
