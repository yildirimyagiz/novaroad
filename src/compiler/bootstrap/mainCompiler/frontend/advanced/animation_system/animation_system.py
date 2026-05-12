#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class AnimatedValue:
    pass

@dataclass
class AnimationController:
    pass

@dataclass
class AnimationModifier:
    pass

@dataclass
class AnimationMonitor:
    pass

@dataclass
class AnimationSpec:
    pass

@dataclass
class AnimationState:
    pass

@dataclass
class Keyframe:
    pass

@dataclass
class KeyframeAnimation:
    pass

@dataclass
class PresetAnimations:
    pass

@dataclass
class ViewAnimation:
    pass

class AnimationCurve(IntEnum):
    """AnimationCurve definition"""
    Linear = 0
    EaseIn = 1
    EaseOut = 2
    EaseInOut = 3
    CubicBezier = 4
    Spring = 5
    Bounce = 6
    Elastic = 7
    Custom = 8

class RepeatMode(IntEnum):
    """RepeatMode definition"""
    Restart = 9
    Reverse = 10
    Mirror = 11

class AnimationDirection(IntEnum):
    """AnimationDirection definition"""
    Normal = 12
    Reverse = 13
    Alternate = 14

class SlideDirection(IntEnum):
    """SlideDirection definition"""
    Top = 15
    Bottom = 16
    Left = 17
    Right = 18

class AnimationTrigger(IntEnum):
    """AnimationTrigger definition"""
    OnAppear = 19
    OnDisappear = 20
    OnTap = 21
    OnStateChange = 22
    Manual = 23


