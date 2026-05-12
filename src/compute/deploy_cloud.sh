#!/bin/bash

# GPU Army Matmul Cloud Deployment Script
# Supports AWS, GCP, Azure GPU instances

set -e

# Configuration
PROJECT_NAME="ultra_matmul_gpu_army"
REGION="${REGION:-us-east-1}"
INSTANCE_TYPE="${INSTANCE_TYPE:-g5.12xlarge}"  # AWS RTX A5000
GCP_MACHINE_TYPE="${GCP_MACHINE_TYPE:-a2-highgpu-1g}"  # GCP A100
AZURE_VM_SIZE="${AZURE_VM_SIZE:-Standard_ND96asr_v4}"  # Azure H100

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}🚀 GPU ARMY MATMUL CLOUD DEPLOYMENT${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo
}

print_step() {
    echo -e "${GREEN}[STEP]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect cloud provider
detect_cloud() {
    if command -v aws &> /dev/null && aws sts get-caller-identity &> /dev/null; then
        echo "aws"
    elif command -v gcloud &> /dev/null && gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 &> /dev/null; then
        echo "gcp"
    elif command -v az &> /dev/null && az account show &> /dev/null; then
        echo "azure"
    else
        echo "local"
    fi
}

# AWS Deployment
deploy_aws() {
    print_step "Deploying to AWS GPU instances..."

    # Create VPC and security group if not exists
    VPC_ID=$(aws ec2 describe-vpcs --filters "Name=isDefault,Values=true" --query 'Vpcs[0].VpcId' --output text 2>/dev/null || echo "")
    if [ -z "$VPC_ID" ]; then
        VPC_ID=$(aws ec2 create-vpc --cidr-block 10.0.0.0/16 --query 'Vpc.VpcId' --output text)
        aws ec2 create-tags --resources $VPC_ID --tags Key=Name,Value=gpu-army-vpc
    fi

    SG_ID=$(aws ec2 create-security-group --group-name gpu-army-sg --description "GPU Army security group" --vpc-id $VPC_ID --query 'GroupId' --output text)
    aws ec2 authorize-security-group-ingress --group-id $SG_ID --protocol tcp --port 22 --cidr 0.0.0.0/0

    # Launch GPU instances
    print_step "Launching $INSTANCE_COUNT x $INSTANCE_TYPE instances..."

    INSTANCE_IDS=$(aws ec2 run-instances \
        --image-id ami-0c02fb55956c7d316 \
        --count $INSTANCE_COUNT \
        --instance-type $INSTANCE_TYPE \
        --key-name ${KEY_NAME:-gpu-army-key} \
        --security-group-ids $SG_ID \
        --block-device-mappings '[{"DeviceName":"/dev/sda1","Ebs":{"VolumeSize":100,"VolumeType":"gp3"}}]' \
        --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=gpu-army-matmul}]" \
        --query 'Instances[*].InstanceId' \
        --output text)

    echo "Launched instances: $INSTANCE_IDS"

    # Wait for instances to be running
    aws ec2 wait instance-running --instance-ids $INSTANCE_IDS
    echo "Instances are running!"

    # Get public IPs
    PUBLIC_IPS=$(aws ec2 describe-instances --instance-ids $INSTANCE_IDS --query 'Reservations[*].Instances[*].PublicIpAddress' --output text)

    echo "Public IPs: $PUBLIC_IPS"

    # Deploy code to instances
    for IP in $PUBLIC_IPS; do
        print_step "Deploying to $IP..."
        scp -o StrictHostKeyChecking=no -i ~/.ssh/gpu-army-key.pem ultra_advanced_matmul_cuda.cu $IP:~/
        scp -o StrictHostKeyChecking=no -i ~/.ssh/gpu-armay-key.pem build_cuda.sh $IP:~/

        ssh -o StrictHostKeyChecking=no -i ~/.ssh/gpu-army-key.pem ubuntu@$IP << 'EOF'
            sudo apt update
            sudo apt install -y nvidia-driver-525 nvidia-cuda-toolkit build-essential

            # Build CUDA code
            chmod +x build_cuda.sh
            ./build_cuda.sh run
EOF
    done
}

# GCP Deployment
deploy_gcp() {
    print_step "Deploying to GCP GPU instances..."

    # Set project
    gcloud config set project ${GCP_PROJECT:-gpu-army-project}

    # Create instances
    for i in $(seq 1 $INSTANCE_COUNT); do
        INSTANCE_NAME="gpu-army-matmul-$i"

        gcloud compute instances create $INSTANCE_NAME \
            --machine-type=$GCP_MACHINE_TYPE \
            --accelerator=type=nvidia-tesla-a100,count=1 \
            --image-family=ubuntu-2004-lts \
            --image-project=ubuntu-os-cloud \
            --boot-disk-size=100GB \
            --boot-disk-type=pd-ssd \
            --maintenance-policy=TERMINATE \
            --restart-on-failure \
            --metadata-from-file startup-script=startup.sh

        echo "Created instance: $INSTANCE_NAME"
    done

    # Deploy code
    for i in $(seq 1 $INSTANCE_COUNT); do
        INSTANCE_NAME="gpu-army-matmul-$i"
        gcloud compute scp ultra_advanced_matmul_cuda.cu $INSTANCE_NAME:~/
        gcloud compute scp build_cuda.sh $INSTANCE_NAME:~/

        gcloud compute ssh $INSTANCE_NAME --command="
            chmod +x build_cuda.sh
            ./build_cuda.sh run
        "
    done
}

# Azure Deployment
deploy_azure() {
    print_step "Deploying to Azure GPU VMs..."

    # Create resource group
    az group create --name gpu-army-rg --location eastus

    # Create VMs
    for i in $(seq 1 $INSTANCE_COUNT); do
        VM_NAME="gpu-army-matmul-$i"

        az vm create \
            --resource-group gpu-army-rg \
            --name $VM_NAME \
            --size $AZURE_VM_SIZE \
            --image Ubuntu2204 \
            --admin-username azureuser \
            --generate-ssh-keys \
            --public-ip-sku Standard \
            --nsg gpu-army-nsg

        echo "Created VM: $VM_NAME"
    done

    # Deploy code
    for i in $(seq 1 $INSTANCE_COUNT); do
        VM_NAME="gpu-army-matmul-$i"
        az vm run-command invoke \
            --resource-group gpu-army-rg \
            --name $VM_NAME \
            --command-id RunShellScript \
            --scripts "
                curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
                wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-ubuntu2204.pin
                sudo mv cuda-ubuntu2204.pin /etc/apt/preferences.d/cuda-repository-pin-600
                wget https://developer.download.nvidia.com/compute/cuda/12.2.0/local_installers/cuda_12.2.0_535.54.03_linux.run
                sudo sh cuda_12.2.0_535.54.03_linux.run --silent --toolkit
                echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
                source ~/.bashrc
                nvcc --version
            "
    done
}

# Local deployment (for testing)
deploy_local() {
    print_step "Running locally..."

    if ! command -v nvidia-smi &> /dev/null; then
        print_error "NVIDIA GPU not detected. This script requires NVIDIA GPUs."
        exit 1
    fi

    ./build_cuda.sh run
}

# Main deployment function
main() {
    INSTANCE_COUNT="${INSTANCE_COUNT:-1}"

    print_header

    echo "Configuration:"
    echo "  Instances: $INSTANCE_COUNT"
    echo "  AWS Type: $INSTANCE_TYPE"
    echo "  GCP Type: $GCP_MACHINE_TYPE"
    echo "  Azure Size: $AZURE_VM_SIZE"
    echo

    CLOUD_PROVIDER=$(detect_cloud)
    echo "Detected cloud provider: $CLOUD_PROVIDER"

    case $CLOUD_PROVIDER in
        aws)
            deploy_aws
            ;;
        gcp)
            deploy_gcp
            ;;
        azure)
            deploy_azure
            ;;
        local)
            deploy_local
            ;;
        *)
            print_error "No cloud provider detected. Please configure AWS, GCP, or Azure CLI."
            exit 1
            ;;
    esac

    echo
    echo -e "${GREEN}✅ GPU ARMY MATMUL DEPLOYMENT COMPLETE!${NC}"
    echo -e "${BLUE}🚀 Ultra Advanced Matmul running on cloud GPUs${NC}"
    echo -e "${BLUE}📊 Target: 10+ TFLOPS across all instances${NC}"
}

# Handle command line arguments
case "$1" in
    aws)
        CLOUD_PROVIDER="aws"
        shift
        ;;
    gcp)
        CLOUD_PROVIDER="gcp"
        shift
        ;;
    azure)
        CLOUD_PROVIDER="azure"
        shift
        ;;
    local)
        CLOUD_PROVIDER="local"
        shift
        ;;
    *)
        ;;
esac

main "$@"
