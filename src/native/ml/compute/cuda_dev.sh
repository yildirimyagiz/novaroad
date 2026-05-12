#!/bin/bash

# GPU Army CUDA Development Environment
# Easy CUDA development with Docker

set -e

IMAGE_NAME="gpu-army-cuda-dev"
CONTAINER_NAME="gpu-army-cuda-container"

print_info() {
    echo "🎯 GPU ARMY CUDA DEVELOPMENT ENVIRONMENT"
    echo "=========================================="
    echo "Docker Image: $IMAGE_NAME"
    echo "CUDA Version: 12.2"
    echo "Base: Ubuntu 22.04"
    echo
}

build_image() {
    echo "🔨 Building CUDA development image..."
    docker build -f Dockerfile.cuda -t $IMAGE_NAME .
    echo "✅ Image built successfully!"
}

run_container() {
    echo "🚀 Starting CUDA development container..."

    # Check if container exists and remove if running
    if docker ps -q -f name=$CONTAINER_NAME | grep -q .; then
        echo "Stopping existing container..."
        docker stop $CONTAINER_NAME
    fi

    if docker ps -aq -f name=$CONTAINER_NAME | grep -q .; then
        echo "Removing existing container..."
        docker rm $CONTAINER_NAME
    fi

    # Mount current directory and run container
    docker run -it --rm \
        --name $CONTAINER_NAME \
        --gpus all \
        -v $(pwd):/workspace \
        -w /workspace \
        $IMAGE_NAME \
        bash
}

test_cuda() {
    echo "🧪 Testing CUDA installation..."
    docker run --rm --gpus all $IMAGE_NAME nvidia-smi
    echo "✅ CUDA ready!"
}

build_and_test() {
    echo "🏗️  Building and testing ultra matmul..."
    docker run --rm --gpus all \
        -v $(pwd):/workspace \
        -w /workspace \
        $IMAGE_NAME \
        bash -c "./build_cuda.sh run"
}

case "$1" in
    build)
        print_info
        build_image
        ;;
    run)
        print_info
        run_container
        ;;
    test)
        print_info
        test_cuda
        ;;
    all)
        print_info
        build_image
        echo
        test_cuda
        echo
        build_and_test
        ;;
    *)
        echo "Usage: $0 {build|run|test|all}"
        echo
        echo "Commands:"
        echo "  build  - Build CUDA development image"
        echo "  run    - Start interactive CUDA container"
        echo "  test   - Test CUDA installation"
        echo "  all    - Build, test, and run benchmarks"
        echo
        echo "Example:"
        echo "  $0 all    # Complete setup and testing"
        echo "  $0 run    # Start development environment"
        ;;
esac
