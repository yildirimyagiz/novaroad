#!/bin/bash
#
# WAN2_2 Training Launch Script
# ==============================
#
# Automatically detects hardware and launches with optimal settings
#

set -e

echo "🚀 WAN2_2 Training Launcher"
echo "================================"
echo ""

# Detect hardware
detect_hardware() {
    # Detect GPUs
    if command -v nvidia-smi &> /dev/null; then
        GPU_COUNT=$(nvidia-smi --list-gpus | wc -l)
        GPU_TYPE="NVIDIA CUDA"
        echo "✅ Detected $GPU_COUNT NVIDIA GPU(s)"
        nvidia-smi --query-gpu=name,memory.total --format=csv,noheader | head -1
    elif command -v rocm-smi &> /dev/null; then
        GPU_COUNT=$(rocm-smi --showproductname | grep -c "GPU")
        GPU_TYPE="AMD ROCm"
        echo "✅ Detected $GPU_COUNT AMD GPU(s)"
    elif [ "$(uname)" = "Darwin" ]; then
        GPU_COUNT=1
        GPU_TYPE="Apple Metal"
        echo "✅ Detected Apple Silicon (Metal)"
        sysctl -n machdep.cpu.brand_string
    else
        GPU_COUNT=0
        GPU_TYPE="CPU"
        echo "⚠️  No GPU detected - using CPU"
    fi
    
    # Detect CPU cores
    if [ "$(uname)" = "Darwin" ]; then
        CPU_CORES=$(sysctl -n hw.ncpu)
    else
        CPU_CORES=$(nproc)
    fi
    echo "   CPU cores: $CPU_CORES"
    echo ""
}

# Set default arguments
DATA_DIR=${DATA_DIR:-"./data/videos"}
BATCH_SIZE=${BATCH_SIZE:-1}
NUM_EPOCHS=${NUM_EPOCHS:-100}
LEARNING_RATE=${LEARNING_RATE:-1e-4}
RESOLUTION=${RESOLUTION:-512}
NUM_FRAMES=${NUM_FRAMES:-16}
OUTPUT_DIR=${OUTPUT_DIR:-"./outputs"}
NUM_WORKERS=${NUM_WORKERS:-4}

# Enable all Nova optimizations by default
USE_MIXED_PRECISION=${USE_MIXED_PRECISION:-true}
USE_GRADIENT_CHECKPOINTING=${USE_GRADIENT_CHECKPOINTING:-true}
USE_MOE=${USE_MOE:-false}

detect_hardware

echo "📋 Training Configuration:"
echo "   Data directory: $DATA_DIR"
echo "   Batch size: $BATCH_SIZE"
echo "   Epochs: $NUM_EPOCHS"
echo "   Learning rate: $LEARNING_RATE"
echo "   Resolution: ${RESOLUTION}x${RESOLUTION}"
echo "   Frames: $NUM_FRAMES"
echo "   Output: $OUTPUT_DIR"
echo ""

echo "⚡ Nova Optimizations:"
echo "   Mixed Precision: $USE_MIXED_PRECISION"
echo "   Gradient Checkpointing: $USE_GRADIENT_CHECKPOINTING"
echo "   Mixture of Experts: $USE_MOE"
echo "   Flash Attention v2: ✅ (always enabled)"
echo ""

# Build training command
TRAINING_CMD="python3 -u training/train_nova.py"
TRAINING_CMD="$TRAINING_CMD --data_dir $DATA_DIR"
TRAINING_CMD="$TRAINING_CMD --batch_size $BATCH_SIZE"
TRAINING_CMD="$TRAINING_CMD --num_epochs $NUM_EPOCHS"
TRAINING_CMD="$TRAINING_CMD --learning_rate $LEARNING_RATE"
TRAINING_CMD="$TRAINING_CMD --resolution $RESOLUTION"
TRAINING_CMD="$TRAINING_CMD --num_frames $NUM_FRAMES"
TRAINING_CMD="$TRAINING_CMD --output_dir $OUTPUT_DIR"

if [ "$USE_MIXED_PRECISION" = "true" ]; then
    TRAINING_CMD="$TRAINING_CMD --mixed_precision"
fi

if [ "$USE_GRADIENT_CHECKPOINTING" = "true" ]; then
    TRAINING_CMD="$TRAINING_CMD --gradient_checkpointing"
fi

if [ "$USE_MOE" = "true" ]; then
    TRAINING_CMD="$TRAINING_CMD --use_moe"
fi

# Launch training
if [ "$GPU_COUNT" -gt 1 ]; then
    echo "🚀 Launching distributed training on $GPU_COUNT GPUs..."
    echo ""
    
    # Use torchrun for multi-GPU training
    torchrun --standalone \
        --nnodes=1 \
        --nproc_per_node=$GPU_COUNT \
        $TRAINING_CMD
else
    echo "🚀 Launching single-GPU/CPU training..."
    echo ""
    
    # Single GPU or CPU
    $TRAINING_CMD
fi

echo ""
echo "✅ Training complete!"
echo "   Checkpoints saved to: $OUTPUT_DIR/checkpoints"
echo "   Logs saved to: $OUTPUT_DIR/logs"
