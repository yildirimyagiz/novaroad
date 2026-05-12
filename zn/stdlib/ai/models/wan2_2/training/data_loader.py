"""
WAN2_2 Data Loader - Optimized for Video Training
==================================================

Features:
- Efficient video loading with decord
- Multi-process data loading
- On-the-fly augmentation
- Text-video pair handling
- Memory-efficient streaming
"""

import os
import json
import random
import torch
from torch.utils.data import Dataset, DataLoader
from torchvision import transforms
import numpy as np
from PIL import Image
from typing import List, Dict, Tuple

try:
    import decord
    decord.bridge.set_bridge('torch')
    DECORD_AVAILABLE = True
except ImportError:
    DECORD_AVAILABLE = False
    print("⚠️  decord not available - using fallback video loader")


class VideoTextDataset(Dataset):
    """
    Video-Text dataset for WAN2_2 training
    
    Expected structure:
    data_dir/
        videos/
            video_001.mp4
            video_002.mp4
            ...
        metadata.json  # {"video_001.mp4": {"caption": "...", "duration": 10.5}}
    """
    
    def __init__(
        self,
        data_dir: str,
        resolution: int = 512,
        num_frames: int = 16,
        frame_stride: int = 1,
        random_crop: bool = True,
        random_flip: bool = True
    ):
        self.data_dir = data_dir
        self.video_dir = os.path.join(data_dir, 'videos')
        self.resolution = resolution
        self.num_frames = num_frames
        self.frame_stride = frame_stride
        self.random_crop = random_crop
        self.random_flip = random_flip
        
        # Load metadata
        metadata_path = os.path.join(data_dir, 'metadata.json')
        if os.path.exists(metadata_path):
            with open(metadata_path, 'r') as f:
                self.metadata = json.load(f)
        else:
            print(f"⚠️  No metadata.json found in {data_dir}")
            self.metadata = {}
        
        # Get list of videos
        self.video_files = [
            f for f in os.listdir(self.video_dir)
            if f.endswith(('.mp4', '.avi', '.mov', '.mkv'))
        ]
        
        print(f"📹 Loaded {len(self.video_files)} videos from {data_dir}")
        
        # Setup transforms
        self.transform = transforms.Compose([
            transforms.Resize(resolution),
            transforms.CenterCrop(resolution) if not random_crop else transforms.RandomCrop(resolution),
            transforms.RandomHorizontalFlip() if random_flip else transforms.Lambda(lambda x: x),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])
        ])
    
    def __len__(self):
        return len(self.video_files)
    
    def __getitem__(self, idx):
        video_file = self.video_files[idx]
        video_path = os.path.join(self.video_dir, video_file)
        
        # Load video
        frames = self.load_video(video_path)
        
        # Get caption
        caption = self.metadata.get(video_file, {}).get('caption', '')
        if not caption:
            caption = f"A video clip {idx}"  # Fallback caption
        
        return {
            'video': frames,
            'prompt': caption,
            'video_id': video_file
        }
    
    def load_video(self, video_path: str) -> torch.Tensor:
        """Load video frames efficiently"""
        if DECORD_AVAILABLE:
            return self._load_video_decord(video_path)
        else:
            return self._load_video_fallback(video_path)
    
    def _load_video_decord(self, video_path: str) -> torch.Tensor:
        """Load video using decord (fast!)"""
        from decord import VideoReader, cpu
        
        try:
            vr = VideoReader(video_path, ctx=cpu(0))
            total_frames = len(vr)
            
            # Sample frames
            if total_frames >= self.num_frames * self.frame_stride:
                # Random start point
                max_start = total_frames - self.num_frames * self.frame_stride
                start_idx = random.randint(0, max_start)
                frame_indices = list(range(
                    start_idx,
                    start_idx + self.num_frames * self.frame_stride,
                    self.frame_stride
                ))
            else:
                # Repeat frames if video too short
                frame_indices = np.linspace(0, total_frames - 1, self.num_frames, dtype=int)
            
            # Load frames
            frames = vr.get_batch(frame_indices)  # [T, H, W, C]
            frames = frames.float() / 255.0
            
            # Apply transforms to each frame
            transformed_frames = []
            for i in range(frames.shape[0]):
                frame = transforms.ToPILImage()(frames[i].permute(2, 0, 1))
                frame = self.transform(frame)
                transformed_frames.append(frame)
            
            # Stack to [C, T, H, W]
            video_tensor = torch.stack(transformed_frames, dim=1)
            
            return video_tensor
            
        except Exception as e:
            print(f"Error loading {video_path}: {e}")
            # Return dummy video
            return torch.randn(3, self.num_frames, self.resolution, self.resolution)
    
    def _load_video_fallback(self, video_path: str) -> torch.Tensor:
        """Fallback video loader (slower)"""
        import cv2
        
        try:
            cap = cv2.VideoCapture(video_path)
            frames = []
            
            frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            
            # Sample frame indices
            if frame_count >= self.num_frames * self.frame_stride:
                max_start = frame_count - self.num_frames * self.frame_stride
                start_idx = random.randint(0, max_start)
                frame_indices = list(range(
                    start_idx,
                    start_idx + self.num_frames * self.frame_stride,
                    self.frame_stride
                ))
            else:
                frame_indices = np.linspace(0, frame_count - 1, self.num_frames, dtype=int)
            
            # Load frames
            for frame_idx in frame_indices:
                cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
                ret, frame = cap.read()
                
                if ret:
                    # Convert BGR to RGB
                    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    frame = Image.fromarray(frame)
                    frame = self.transform(frame)
                    frames.append(frame)
                else:
                    # Use last frame if read fails
                    if frames:
                        frames.append(frames[-1])
                    else:
                        # Dummy frame
                        frames.append(torch.randn(3, self.resolution, self.resolution))
            
            cap.release()
            
            # Stack to [C, T, H, W]
            video_tensor = torch.stack(frames, dim=1)
            return video_tensor
            
        except Exception as e:
            print(f"Error loading {video_path}: {e}")
            return torch.randn(3, self.num_frames, self.resolution, self.resolution)


def create_dataloader(
    data_dir: str,
    batch_size: int = 1,
    num_workers: int = 4,
    resolution: int = 512,
    num_frames: int = 16,
    shuffle: bool = True,
    pin_memory: bool = True
) -> DataLoader:
    """
    Create optimized data loader for training
    
    Args:
        data_dir: Path to data directory
        batch_size: Batch size
        num_workers: Number of data loading workers
        resolution: Video resolution
        num_frames: Number of frames to sample
        shuffle: Whether to shuffle data
        pin_memory: Pin memory for faster GPU transfer
    
    Returns:
        DataLoader instance
    """
    dataset = VideoTextDataset(
        data_dir=data_dir,
        resolution=resolution,
        num_frames=num_frames,
        random_crop=True,
        random_flip=True
    )
    
    dataloader = DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=shuffle,
        num_workers=num_workers,
        pin_memory=pin_memory,
        drop_last=True,  # Drop last incomplete batch
        persistent_workers=True if num_workers > 0 else False  # Keep workers alive
    )
    
    print(f"✅ DataLoader created:")
    print(f"   Dataset size: {len(dataset)}")
    print(f"   Batch size: {batch_size}")
    print(f"   Num workers: {num_workers}")
    print(f"   Steps per epoch: {len(dataloader)}")
    
    return dataloader


def create_dummy_dataloader(
    batch_size: int = 1,
    num_steps: int = 100,
    resolution: int = 512,
    num_frames: int = 16
):
    """
    Create dummy dataloader for testing
    """
    class DummyDataset(Dataset):
        def __init__(self, num_samples):
            self.num_samples = num_samples
        
        def __len__(self):
            return self.num_samples
        
        def __getitem__(self, idx):
            return {
                'video': torch.randn(3, num_frames, resolution, resolution),
                'prompt': f"A dummy video {idx}",
                'video_id': f'dummy_{idx}'
            }
    
    dataset = DummyDataset(num_steps * batch_size)
    dataloader = DataLoader(
        dataset,
        batch_size=batch_size,
        shuffle=False,
        num_workers=0
    )
    
    print(f"⚠️  Using dummy dataloader for testing")
    print(f"   Steps: {num_steps}")
    
    return dataloader


# Test the data loader
if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser()
    parser.add_argument('--data_dir', type=str, required=True)
    parser.add_argument('--batch_size', type=int, default=1)
    parser.add_argument('--num_workers', type=int, default=4)
    args = parser.parse_args()
    
    # Create dataloader
    dataloader = create_dataloader(
        data_dir=args.data_dir,
        batch_size=args.batch_size,
        num_workers=args.num_workers
    )
    
    # Test loading one batch
    print("\n🧪 Testing data loading...")
    batch = next(iter(dataloader))
    
    print(f"   Video shape: {batch['video'].shape}")
    print(f"   Prompt: {batch['prompt'][0][:100]}...")
    print(f"   Video ID: {batch['video_id'][0]}")
    print("\n✅ Data loader test passed!")
