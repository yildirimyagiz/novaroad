#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


@dataclass
class Menu:
    pass

@dataclass
class MenuBar:
    pass

@dataclass
class MenuEventDispatcher:
    pass

@dataclass
class MenuId:
    pass

@dataclass
class MenuItem:
    pass

@dataclass
class MenuItemConfig:
    pass

@dataclass
class MenuItemId:
    pass

@dataclass
class MenuItemState:
    pass

@dataclass
class MenuShortcut:
    pass

class MenuItemType(IntEnum):
    """MenuItemType definition"""
    Normal = 0
    Separator = 1
    CheckBox = 2
    Radio = 3
    Submenu = 4

class MenuError(IntEnum):
    """MenuError definition"""
    CreationFailed = 5
    InvalidConfig = 6
    PlatformError = 7


