#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


class Option(IntEnum):
    """Option definition"""
    Some = 0
    None = 1


