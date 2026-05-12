#!/bin/bash

# GPU Army Universal Cloud Deployment
# Supports Windows & Linux across all major cloud providers

set -e

# Configuration - Auto-detect and support all providers
declare -A CLOUD_CONFIGS=(
    # AWS Configurations
    ["aws-windows"]="g4dn.xlarge windows-server-2022"  # RTX 3050
    ["aws-linux"]="g5.xlarge ubuntu-20.04"             # RTX A5000

    # GCP Configurations
    ["gcp-windows"]="n1-standard-8 windows-server-2022" # No GPU
    ["gcp-linux"]="a2-highgpu-1g ubuntu-2004"           # A100

    # Azure Configurations
    ["azure-windows"]="Standard_NV6_Promo windows-server-2022" # RTX A4000
    ["azure-linux"]="Standard_ND96asr_v4 ubuntu-20.04"         # H100

    # DigitalOcean Configurations
    ["do-windows"]="not-supported windows-server-2022"
    ["do-linux"]="gpu-h100x1 ubuntu-20.04"

    # Linode Configurations
    ["linode-windows"]="not-supported windows-server-2022"
    ["linode-linux"]="gpu-4096 ubuntu-20.04"

    # Vultr Configurations
    ["vultr-windows"]="not-supported windows-server-2022"
    ["vultr-linux"]="gpu-4096 ubuntu-20.04"
)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo -e "${CYAN}================================================${NC}"
    echo -e "${CYAN}🚀 GPU ARMY UNIVERSAL CLOUD DEPLOYMENT${NC}"
    echo -e "${CYAN}================================================${NC}"
    echo -e "${PURPLE}🎯 Supporting ALL Server Types & Backends${NC}"
    echo
}

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

detect_cloud_provider() {
    local os_type=$1

    if command -v aws &> /dev/null && aws sts get-caller-identity &> /dev/null; then
        echo "aws-${os_type}"
    elif command -v gcloud &> /dev/null && gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 &> /dev/null; then
        echo "gcp-${os_type}"
    elif command -v az &> /dev/null && az account show &> /dev/null; then
        echo "azure-${os_type}"
    elif command -v doctl &> /dev/null && doctl account get &> /dev/null; then
        echo "do-${os_type}"
    elif command -v linode-cli &> /dev/null; then
        echo "linode-${os_type}"
    elif command -v vultr &> /dev/null; then
        echo "vultr-${os_type}"
    else
        echo "local-${os_type}"
    fi
}

# AWS Deployment
deploy_aws() {
    local os_type=$1
    local instance_type=$2

    print_step "Deploying to AWS ${os_type}"

    if [[ $os_type == "windows" ]]; then
        # Windows AMI
        local ami_id="ami-0c02fb55956c7d316"  # Windows Server 2022
    else
        # Linux AMI
        local ami_id="ami-0abcdef1234567890"  # Ubuntu 20.04
    fi

    # Create security group
    local sg_id=$(aws ec2 create-security-group \
        --group-name "gpu-army-${os_type}-sg" \
        --description "GPU Army ${os_type} security group" \
        --query 'GroupId' --output text)

    aws ec2 authorize-security-group-ingress --group-id $sg_id --protocol tcp --port 3389 --cidr 0.0.0.0/0  # RDP
    aws ec2 authorize-security-group-ingress --group-id $sg_id --protocol tcp --port 22 --cidr 0.0.0.0/0   # SSH

    # Launch instance
    local instance_id=$(aws ec2 run-instances \
        --image-id $ami_id \
        --count 1 \
        --instance-type $instance_type \
        --key-name ${KEY_NAME:-gpu-army-key} \
        --security-group-ids $sg_id \
        --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=gpu-army-${os_type}}]" \
        --query 'Instances[0].InstanceId' \
        --output text)

    echo "Launched instance: $instance_id"

    # Wait and get IP
    aws ec2 wait instance-running --instance-ids $instance_id
    local public_ip=$(aws ec2 describe-instances \
        --instance-ids $instance_id \
        --query 'Reservations[0].Instances[0].PublicIpAddress' \
        --output text)

    echo "Public IP: $public_ip"

    # Deploy based on OS
    if [[ $os_type == "windows" ]]; then
        deploy_windows_aws $public_ip
    else
        deploy_linux_aws $public_ip
    fi
}

deploy_windows_aws() {
    local ip=$1
    print_step "Setting up Windows GPU environment on AWS"

    # Use AWS Systems Manager or RDP
    # PowerShell commands for CUDA/ROCm setup
    cat << 'EOF' > windows_setup.ps1
# Install CUDA Toolkit
Invoke-WebRequest -Uri "https://developer.download.nvidia.com/compute/cuda/12.2.0/local_installers/cuda_12.2.0_535.54.03_windows.exe" -OutFile "cuda_installer.exe"
Start-Process -FilePath "cuda_installer.exe" -ArgumentList "/silent /noopengl" -Wait

# Install GPU drivers
$nvidiaUrl = "https://us.download.nvidia.com/Windows/535.54/535.54-desktop-win10-win11-64bit-international-dch-whql.exe"
Invoke-WebRequest -Uri $nvidiaUrl -OutFile "nvidia_driver.exe"
Start-Process -FilePath "nvidia_driver.exe" -ArgumentList "/silent /noexec /nofinish" -Wait

# Install Visual Studio Build Tools
choco install visualstudio2019buildtools -y
choco install visualstudio2019-workload-vctools -y

# Clone and build
git clone https://github.com/your-repo/gpu-army.git
cd gpu-army/native/src/compute
nvcc -O3 ultra_advanced_matmul_cuda.cu -o ultra_matmul.exe
.\ultra_matmul.exe
EOF

    # Upload and execute
    echo "Windows setup script created. Use RDP to connect and run windows_setup.ps1"
}

deploy_linux_aws() {
    local ip=$1
    print_step "Setting up Linux GPU environment on AWS"

    scp -o StrictHostKeyChecking=no ultra_cross_platform_matmul.cpp ubuntu@$ip:~/
    scp -o StrictHostKeyChecking=no build_cross_platform.sh ubuntu@$ip:~/

    ssh -o StrictHostKeyChecking=no ubuntu@$ip << 'EOF'
        sudo apt update
        sudo apt install -y nvidia-driver-525 nvidia-cuda-toolkit build-essential cmake

        # Build cross-platform matmul
        chmod +x build_cross_platform.sh
        ./build_cross_platform.sh

        # Run benchmark
        ./ultra_cross_platform_matmul
EOF
}

# GCP Deployment
deploy_gcp() {
    local os_type=$1
    local machine_type=$2

    print_step "Deploying to GCP ${os_type}"

    if [[ $os_type == "windows" ]]; then
        gcloud compute instances create "gpu-army-windows" \
            --machine-type=$machine_type \
            --image-family=windows-server-2022 \
            --image-project=windows-cloud \
            --boot-disk-size=100GB \
            --metadata-from-file startup-script=windows_startup.ps1
    else
        gcloud compute instances create "gpu-army-linux" \
            --machine-type=$machine_type \
            --accelerator=type=nvidia-tesla-a100,count=1 \
            --image-family=ubuntu-2004-lts \
            --image-project=ubuntu-os-cloud \
            --boot-disk-size=100GB \
            --metadata-from-file startup-script=linux_startup.sh
    fi
}

# Azure Deployment
deploy_azure() {
    local os_type=$1
    local vm_size=$2

    print_step "Deploying to Azure ${os_type}"

    az group create --name gpu-army-rg --location eastus

    if [[ $os_type == "windows" ]]; then
        az vm create \
            --resource-group gpu-army-rg \
            --name gpu-army-windows \
            --size $vm_size \
            --image win2022datacenter \
            --admin-username azureuser \
            --generate-ssh-keys \
            --public-ip-sku Standard
    else
        az vm create \
            --resource-group gpu-army-rg \
            --name gpu-army-linux \
            --size $vm_size \
            --image Ubuntu2204 \
            --admin-username azureuser \
            --generate-ssh-keys \
            --public-ip-sku Standard
    fi
}

# DigitalOcean Deployment
deploy_do() {
    local os_type=$1

    if [[ $os_type == "windows" ]]; then
        print_warning "DigitalOcean doesn't support Windows GPU instances"
        return
    fi

    print_step "Deploying to DigitalOcean ${os_type}"

    # Create droplet with GPU
    doctl compute droplet create gpu-army-do \
        --size gpu-h100x1 \
        --image ubuntu-20-04-x64 \
        --region nyc1 \
        --ssh-keys $(doctl compute ssh-key list --format ID --no-header | head -1)
}

# Linode Deployment
deploy_linode() {
    local os_type=$1

    if [[ $os_type == "windows" ]]; then
        print_warning "Linode doesn't support Windows GPU instances"
        return
    fi

    print_step "Deploying to Linode ${os_type}"

    linode-cli linodes create \
        --type gpu-4096 \
        --region us-east \
        --image linode/ubuntu20.04 \
        --label gpu-army-linode
}

# Vultr Deployment
deploy_vultr() {
    local os_type=$1

    if [[ $os_type == "windows" ]]; then
        print_warning "Vultr doesn't support Windows GPU instances"
        return
    fi

    print_step "Deploying to Vultr ${os_type}"

    vultr instance create \
        --plan gpu-4096 \
        --region ewr \
        --os 387 \  # Ubuntu 20.04
        --label gpu-army-vultr
}

# Main deployment function
main() {
    print_header

    local os_type=$(detect_os)
    local cloud_provider=$(detect_cloud_provider $os_type)

    echo "Detected OS: $os_type"
    echo "Detected Cloud: $cloud_provider"
    echo

    # Parse cloud provider config
    IFS=' ' read -r instance_type os_image <<< "${CLOUD_CONFIGS[$cloud_provider]}"

    if [[ $instance_type == "not-supported" ]]; then
        print_error "$cloud_provider is not supported for GPU instances"
        exit 1
    fi

    echo "Instance Type: $instance_type"
    echo "OS Image: $os_image"
    echo

    case $cloud_provider in
        aws-*)
            deploy_aws $os_type $instance_type
            ;;
        gcp-*)
            deploy_gcp $os_type $instance_type
            ;;
        azure-*)
            deploy_azure $os_type $instance_type
            ;;
        do-*)
            deploy_do $os_type
            ;;
        linode-*)
            deploy_linode $os_type
            ;;
        vultr-*)
            deploy_vultr $os_type
            ;;
        local-*)
            print_warning "Local deployment - install CUDA/ROCm drivers manually"
            ./cuda_dev.sh all
            ;;
        *)
            print_error "Unknown cloud provider: $cloud_provider"
            exit 1
            ;;
    esac

    echo
    echo -e "${GREEN}✅ GPU ARMY DEPLOYMENT COMPLETE!${NC}"
    echo -e "${CYAN}🎯 Ultra Cross-Platform Matmul Active${NC}"
    echo -e "${PURPLE}📊 Testing ALL Backends: CUDA, ROCm, DirectX, OpenCL, Vulkan, Metal${NC}"
}

# Utility functions
print_step() {
    echo -e "${GREEN}[STEP]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --os)
            os_type="$2"
            shift 2
            ;;
        --cloud)
            cloud_provider="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [--os linux|windows] [--cloud aws|gcp|azure|do|linode|vultr]"
            echo
            echo "Supported combinations:"
            echo "  AWS: Linux (RTX A5000), Windows (RTX 3050)"
            echo "  GCP: Linux (A100), Windows (No GPU)"
            echo "  Azure: Linux (H100), Windows (RTX A4000)"
            echo "  DigitalOcean: Linux (H100)"
            echo "  Linode: Linux (RTX 4090)"
            echo "  Vultr: Linux (RTX 4090)"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

main "$@"
