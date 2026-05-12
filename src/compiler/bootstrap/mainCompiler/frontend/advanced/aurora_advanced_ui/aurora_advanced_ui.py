#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class ActiveAnimation:
    pass

@dataclass
class Animation:
    pass

@dataclass
class Animator:
    pass

@dataclass
class AuroraAccessibilityInfo:
    pass

@dataclass
class AuroraView:
    pass

@dataclass
class BackgroundModifier:
    pass

@dataclass
class Border:
    pass

@dataclass
class Color:
    pass

@dataclass
class EdgeInsets:
    pass

@dataclass
class Environment:
    pass

@dataclass
class FrameModifier:
    pass

@dataclass
class ImageSource:
    pass

@dataclass
class Modifiers:
    pass

@dataclass
class Observable:
    pass

@dataclass
class PaddingModifier:
    pass

@dataclass
class Point:
    pass

@dataclass
class Rect:
    pass

@dataclass
class RenderContext:
    pass

@dataclass
class Shadow:
    pass

@dataclass
class ShadowModifier:
    pass

@dataclass
class Size:
    pass

@dataclass
class State:
    pass

@dataclass
class Transform:
    pass

class ViewType(IntEnum):
    """ViewType definition"""
    Text = 0
    Button = 1
    Image = 2
    Container = 3
    List = 4
    Scroll = 5
    Stack = 6
    Custom = 7

class ViewContent(IntEnum):
    """ViewContent definition"""
    Empty = 8
    Text = 9
    Image = 10
    Children = 11

class BorderStyle(IntEnum):
    """BorderStyle definition"""
    Solid = 12
    Dashed = 13
    Dotted = 14

class Alignment(IntEnum):
    """Alignment definition"""
    TopLeading = 15
    Top = 16
    TopTrailing = 17
    Leading = 18
    Center = 19
    Trailing = 20
    BottomLeading = 21
    Bottom = 22
    BottomTrailing = 23

class GestureRecognizer(IntEnum):
    """GestureRecognizer definition"""
    Tap = 24
    Swipe = 25
    Pinch = 26
    Rotate = 27
    Pan = 28

class SwipeDirection(IntEnum):
    """SwipeDirection definition"""
    Up = 29
    Down = 30
    Left = 31
    Right = 32

class Gesture(IntEnum):
    """Gesture definition"""
    Tap = 33
    Swipe = 34
    Pinch = 35
    Rotate = 36
    Pan = 37

class AnimationCurve(IntEnum):
    """AnimationCurve definition"""
    Linear = 38
    EaseIn = 39
    EaseOut = 40
    EaseInOut = 41
    Spring = 42
    Custom = 43

class AccessibilityTrait(IntEnum):
    """AccessibilityTrait definition"""
    Button = 44
    Header = 45
    Image = 46
    StaticText = 47
    Adjustable = 48
    Selected = 49
    NotEnabled = 50
    SummaryElement = 51
    KeyboardKey = 52
    SearchField = 53
    EditableText = 54
    AllowsDirectInteraction = 55


