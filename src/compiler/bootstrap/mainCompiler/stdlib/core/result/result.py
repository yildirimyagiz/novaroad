#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


class Result(IntEnum):
    """Result definition"""
    Ok = 0
    Err = 1


