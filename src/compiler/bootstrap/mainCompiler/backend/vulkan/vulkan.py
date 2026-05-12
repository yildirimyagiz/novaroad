#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ComputeShader:
    pass

@dataclass
class VulkanBackend:
    pass

class DeviceType(IntEnum):
    """DeviceType definition"""
    DiscreteGPU = 0
    IntegratedGPU = 1
    VirtualGPU = 2
    CPU = 3


