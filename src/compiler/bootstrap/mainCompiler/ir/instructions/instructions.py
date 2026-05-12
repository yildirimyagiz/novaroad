#!/usr/bin/env python3
from enum import IntEnum
from dataclasses import dataclass, field
from typing import List, Optional, Any


class Instruction(IntEnum):
    """Instruction definition"""
    Add = 0
    Sub = 1
    Mul = 2
    Div = 3
    Rem = 4
    And = 5
    Or = 6
    Xor = 7
    Shl = 8
    Shr = 9
    Eq = 10
    Ne = 11
    Lt = 12
    Le = 13
    Gt = 14
    Ge = 15
    Load = 16
    Store = 17
    Alloca = 18
    Br = 19
    CondBr = 20
    Ret = 21
    Unreachable = 22
    Call = 23
    Trunc = 24
    ZExt = 25
    SExt = 26
    FPTrunc = 27
    FPExt = 28
    FPToUI = 29
    FPToSI = 30
    UIToFP = 31
    SIToFP = 32
    Bitcast = 33
    ExtractElement = 34
    InsertElement = 35
    ExtractValue = 36
    InsertValue = 37
    Phi = 38
    Select = 39


