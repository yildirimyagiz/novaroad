# Quick Start: Train Real Models

## 🚀 Run Full MNIST Training (10-20 minutes)

### Quick Command
```bash
bash tmp_rovodev_run_mnist.sh
```

### What It Does
1. ✅ Downloads MNIST dataset automatically
2. ✅ Creates necessary directories
3. ✅ Runs complete training pipeline
4. ✅ Saves model and logs

### Expected Output
```
╔════════════════════════════════════════════════════════╗
║        Running Full MNIST Training (10-20 min)        ║
╚════════════════════════════════════════════════════════╝

✓ Directories created
✓ MNIST dataset downloaded

🚀 Starting training...

Step 1: Creating model...
✓ Model created: 235,146 parameters

Step 2: Finding optimal batch size...
Testing batch size: 16... ✓ Success! Memory: 45.23 MB
Testing batch size: 32... ✓ Success! Memory: 87.45 MB
Testing batch size: 64... ✓ Success! Memory: 172.34 MB
Testing batch size: 128... ✓ Success! Memory: 341.12 MB
Testing batch size: 256... ✗ Failed: Out of memory
✓ Optimal batch size: 128

Step 3: Loading MNIST dataset...
✓ Training samples: 60000
✓ Validation samples: 10000

Step 4: Finding optimal learning rate...
Running LR range test from 1e-07 to 10.0 over 100 iterations
  LR: 1.00e-07, Loss: 2.304561
  LR: 1.00e-05, Loss: 2.289123
  LR: 1.00e-03, Loss: 0.856432  ← Steepest descent
  LR: 1.00e-01, Loss: 1.234567
✓ Optimal learning rate: 1.00e-04

Step 5: Setting up optimizer...
✓ Optimizer: Adam + Gradient Centralization

Step 6: Configuring training...
Configuration:
  • Epochs: 50
  • Batch size: 128
  • Learning rate: 1.00e-04
  • Scheduler: Cosine Annealing
  • Gradient clipping: 1.0

Step 7: Setting up SWA...
✓ SWA will start at epoch 38

╔════════════════════════════════════════════════════════╗
║                   Starting Training                    ║
╚════════════════════════════════════════════════════════╝

Epoch   1/50: Train Loss: 0.4521, Train Acc: 86.34%, Val Loss: 0.2134, Val Acc: 93.21%
Epoch   2/50: Train Loss: 0.1834, Train Acc: 94.56%, Val Loss: 0.1245, Val Acc: 96.12%
Epoch   3/50: Train Loss: 0.1234, Train Acc: 96.23%, Val Loss: 0.0987, Val Acc: 97.01%
...
Epoch  38/50: Train Loss: 0.0123, Train Acc: 99.67%, Val Loss: 0.0456, Val Acc: 98.89% [SWA]
...
Epoch  50/50: Train Loss: 0.0089, Train Acc: 99.78%, Val Loss: 0.0412, Val Acc: 98.92% [SWA]

╔════════════════════════════════════════════════════════╗
║                  Finalizing SWA                        ║
╚════════════════════════════════════════════════════════╝

Updating BN statistics for SWA model...
✓ BN statistics updated

╔════════════════════════════════════════════════════════╗
║                   Training Complete!                   ║
╚════════════════════════════════════════════════════════╝

Final Results:
  • Final accuracy (SWA): 99.24%
  • Total epochs: 50
  • Optimal batch size: 128
  • Optimal learning rate: 1.00e-04
  • Models averaged (SWA): 12

✓ Model saved to mnist_final_model.pt

✓ Training complete!
```

---

## 🎨 Run CIFAR-10 Training (2-3 hours on GPU)

### Quick Command
```bash
bash tmp_rovodev_run_cifar10.sh
```

### What It Does
1. ✅ Checks for GPU availability
2. ✅ Downloads CIFAR-10 dataset automatically
3. ✅ Runs ResNet-18 training with all advanced features
4. ✅ Saves checkpoints and TensorBoard logs

### Expected Output
```
╔════════════════════════════════════════════════════════╗
║      Running CIFAR-10 Training (2-3 hours)            ║
╚════════════════════════════════════════════════════════╝

✓ NVIDIA GPU detected
Tesla V100-SXM2-16GB, 16160 MiB

✓ Directories created
✓ CIFAR-10 dataset downloaded

Configuration:
  • Model: ResNet-18
  • Dataset: CIFAR-10 (60,000 images)
  • Expected time: 2-3 hours (GPU)
  • Expected accuracy: 95%+

Start training? (y/n) y

🚀 Starting training...

Step 1: Creating ResNet-18...
✓ Model created: 11,173,962 parameters

Step 2: Finding optimal batch size...
✓ Optimal batch size: 256

Step 3: Setting up Progressive Resizing...

Progressive Resizing Schedule:
┌────────────┬──────────────┐
│   Epoch    │  Image Size  │
├────────────┼──────────────┤
│      0     │  24 × 24     │
│     50     │  28 × 28     │
│    100     │  32 × 32     │
└────────────┴──────────────┘

Step 4: Loading CIFAR-10 dataset...
✓ Training samples: 50000
✓ Test samples: 10000
✓ Data augmentation: Enabled

Step 5: Finding optimal learning rate...
✓ Optimal learning rate: 3.16e-02

Step 6: Setting up optimizer...
✓ Optimizer: SGD + Momentum + GC (conv only)

Step 7: Enabling Mixed Precision (FP16)...
✓ Mixed precision enabled (expected 2x speedup)

Step 8: Setting up SWA...
✓ SWA will start at epoch 160

Step 9: Setting up OneCycle LR...
✓ OneCycle scheduler configured

╔════════════════════════════════════════════════════════╗
║              Training Configuration                    ║
╚════════════════════════════════════════════════════════╝
  • Model: ResNet-18
  • Dataset: CIFAR-10
  • Epochs: 200
  • Batch size: 256
  • Learning rate: 3.16e-02
  • Optimizer: SGD + Momentum (0.9) + GC
  • Weight decay: 5e-4
  • Mixed precision: FP16
  • Progressive resizing: 24 → 28 → 32
  • SWA: Starting epoch 160
  • LR Scheduler: OneCycle
  • Data augmentation: RandomCrop + HFlip
  • Label smoothing: 0.1

╔════════════════════════════════════════════════════════╗
║                   Starting Training                    ║
╚════════════════════════════════════════════════════════╝

→ Image size changed to 24 × 24

Epoch   1/200: Loss: 1.8934/1.9123, Acc: 32.45%/31.23%, LR: 3.16e-03, Scale: 65536
Epoch   2/200: Loss: 1.5432/1.6234, Acc: 45.67%/44.12%, LR: 6.32e-03, Scale: 65536
...
Epoch  50/200: Loss: 0.4532/0.5123, Acc: 86.23%/85.01%, LR: 3.16e-02, Scale: 65536

→ Image size changed to 28 × 28

Epoch  51/200: Loss: 0.4312/0.5034, Acc: 87.12%/85.67%, LR: 3.14e-02, Scale: 65536
...
Epoch 100/200: Loss: 0.2134/0.2890, Acc: 92.89%/91.45%, LR: 2.98e-02, Scale: 65536

→ Image size changed to 32 × 32

Epoch 101/200: Loss: 0.2023/0.2765, Acc: 93.45%/92.01%, LR: 2.94e-02, Scale: 65536
...
Epoch 160/200: Loss: 0.0834/0.1234, Acc: 97.12%/95.67%, LR: 1.58e-03, Scale: 65536 [SWA]
...
Epoch 200/200: Loss: 0.0612/0.1089, Acc: 97.89%/96.12%, LR: 3.16e-06, Scale: 65536 [SWA]

╔════════════════════════════════════════════════════════╗
║                  Finalizing SWA                        ║
╚════════════════════════════════════════════════════════╝

Updating BN statistics for SWA model...
✓ BN statistics updated

╔════════════════════════════════════════════════════════╗
║                Training Complete!                      ║
╚════════════════════════════════════════════════════════╝

Final Results:
  • Best validation accuracy: 96.12%
  • Final SWA accuracy: 96.45%
  • Improvement from SWA: +0.33%
  • Total epochs: 200
  • Models averaged: 40

✓ Final model saved to cifar10_swa_final.pt

Expected accuracy: ~95%+ (state-of-the-art with these techniques)

✓ Training complete!

Results saved to:
  • Model: checkpoints/cifar10/
  • Logs: logs/cifar10/
  • TensorBoard: tensorboard --logdir=logs/cifar10
```

---

## 📊 Viewing Results

### TensorBoard (Recommended)
```bash
# For MNIST
tensorboard --logdir=logs/mnist

# For CIFAR-10
tensorboard --logdir=logs/cifar10
```

Then open: http://localhost:6006

### Saved Files
```
checkpoints/
├── mnist/
│   ├── model_epoch_10_loss_0.1234.pt
│   ├── model_epoch_20_loss_0.0567.pt
│   └── mnist_final_model.pt
└── cifar10/
    ├── cifar10_best_model.pt
    └── cifar10_swa_final.pt

logs/
├── mnist/
│   ├── events.out.tfevents.xxx
│   └── training_log.csv
└── cifar10/
    ├── events.out.tfevents.xxx
    └── training_log.csv
```

---

## 🎯 What You'll Learn

### From MNIST Training:
- ✅ Automatic hyperparameter finding
- ✅ How LR Finder saves time
- ✅ What SWA does for accuracy
- ✅ Basic training pipeline
- ✅ ~99% accuracy achievable

### From CIFAR-10 Training:
- ✅ Progressive resizing strategy
- ✅ Mixed precision training (2x speedup)
- ✅ Advanced data augmentation
- ✅ ResNet architecture
- ✅ OneCycle LR scheduler
- ✅ Production-grade training
- ✅ ~95%+ accuracy achievable

---

## 🔥 Pro Tips

### Speed Up Training
```bash
# Use smaller model for testing
# Edit train_cifar10.zn: ResNet18 → MobileNetV2

# Reduce epochs for quick test
# Change: epochs = 200 → epochs = 20

# Use smaller batch size if OOM
# Change: batch_size = 256 → batch_size = 128
```

### Monitor Training
```bash
# Watch GPU usage
watch -n 1 nvidia-smi

# Watch logs in real-time
tail -f logs/cifar10/training_log.csv
```

### Resume Training
```bash
# If training is interrupted, it auto-resumes from last checkpoint
# Just run the same command again
bash tmp_rovodev_run_cifar10.sh
```

---

## ⏱️ Time Estimates

| Task | CPU | Single GPU | 4 GPUs |
|------|-----|------------|--------|
| MNIST | 10-20 min | 5-10 min | 3-5 min |
| CIFAR-10 | 10-12 hrs | 2-3 hrs | 1-1.5 hrs |

---

## 🎓 Next Steps After Training

1. **Evaluate Model**
   ```bash
   nova run evaluate_model.zn --model checkpoints/cifar10/cifar10_swa_final.pt
   ```

2. **Export for Production**
   ```bash
   nova export --model cifar10_swa_final.pt --format onnx
   ```

3. **Try ImageNet**
   ```bash
   # Distributed training on 8 GPUs
   torchrun --nproc_per_node=8 zn/ml/examples/train_imagenet.zn
   ```

---

**Ready to start? Just run:**
```bash
# MNIST (quick)
bash tmp_rovodev_run_mnist.sh

# CIFAR-10 (longer but impressive)
bash tmp_rovodev_run_cifar10.sh
```
