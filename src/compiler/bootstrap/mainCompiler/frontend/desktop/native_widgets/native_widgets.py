#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Button:
    pass

@dataclass
class Container:
    pass

@dataclass
class Font:
    pass

@dataclass
class Label:
    pass

@dataclass
class Rect:
    pass

@dataclass
class TextField:
    pass

@dataclass
class WidgetConfig:
    pass

@dataclass
class WidgetId:
    pass

class TextAlignment(IntEnum):
    """TextAlignment definition"""
    Left = 0
    Center = 1
    Right = 2

class LayoutType(IntEnum):
    """LayoutType definition"""
    Absolute = 3
    Vertical = 4
    Horizontal = 5
    Grid = 6

class FontWeight(IntEnum):
    """FontWeight definition"""
    Thin = 7
    Light = 8
    Regular = 9
    Medium = 10
    Bold = 11
    Black = 12

class FontStyle(IntEnum):
    """FontStyle definition"""
    Normal = 13
    Italic = 14
    Oblique = 15

class WidgetError(IntEnum):
    """WidgetError definition"""
    CreationFailed = 16
    InvalidConfig = 17
    PlatformError = 18


