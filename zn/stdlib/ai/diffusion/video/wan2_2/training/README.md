# WAN 2.2 Training Module

Native Nova implementation for fine-tuning WAN 2.2 video generation models.

## Quick Start

```nova
import ai.diffusion.video.wan2_2.training as wan_train

# Quick LoRA training
wan_train.quick_lora_train(
    dataset_path="./data/my_videos",
    metadata_path="./data/my_videos/metadata.csv",
    num_epochs=5,
)
```

## Features

✅ **LoRA Training** - Efficient fine-tuning on consumer GPUs  
✅ **Full Model Training** - Complete retraining for enterprises  
✅ **Multiple Model Types** - T2V, I2V, TI2V support  
✅ **Advanced Optimization** - Gradient checkpointing, 8-bit Adam, mixed precision  
✅ **Flow Matching Loss** - State-of-the-art training objective  
✅ **Flexible Configuration** - Full control over all parameters  

## Modules

- `config.py` - Training configuration
- `trainer.py` - Main training logic
- `losses.py` - Loss functions (Flow Matching, Distillation)
- `dataset.py` - Video dataset loader
- `lora_training.py` - LoRA-specific training
- `full_training.py` - Full model training

## Documentation

See [WAN2_2_TRAINING_GUIDE.md](../../../../../docs/WAN2_2_TRAINING_GUIDE.md) for complete guide.

## Examples

- `examples/wan2_2_training_simple.zn` - Minimal example
- `examples/wan2_2_training_demo.zn` - Full demo
- `examples/wan2_2_training_advanced.zn` - Advanced usage

## Requirements

- PyTorch 2.0+
- CUDA 11.8+
- PEFT library (for LoRA)
- 16GB+ VRAM (LoRA)
- 80GB+ VRAM per GPU (Full training)
