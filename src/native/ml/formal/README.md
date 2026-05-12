# Nova Formal Verification Subsystem

This directory contains the native logic for formal methods, proof checking, and
symbolic execution within the Nova compiler.

## Key Components

- **`nova_proof_cache.h/c`**: Manages the persistence and lookup of verified
  theorems to speed up compilation.
- **`symbolic_engine`**: Tools for symbolic execution and constraint solving.
- **`safety_guarantees`**: Implementation of proof-carrying code patterns.

## Purpose

The Formal Verification Subsystem is responsible for ensuring the mathematical
correctness of code blocks marked with `verified` or `guarantee!`, fulfilling
the "Sovereign" security promise.
