# Hybrid AI Agent System

Seamless Python ↔ Nova AI agents with Claude SDK.

## Features

✅ **Dual Implementation**
- Pure Python agents using Claude SDK directly
- Nova agents with Python backend bridge
- Automatic backend selection

✅ **Full Interoperability**
- Call Python agents from Nova
- Call Nova agents from Python
- Share context between both

✅ **Complete Coverage**
- Code generation
- Debugging
- All agent features in both languages

## Quick Start

### Python

```python
from ai.agents.python_agent import quick_code_sync

code = quick_code_sync("Implement binary search in Nova")
```

### Nova

```nova
import ai.agents.hybrid as hybrid

code = await hybrid.python_code("Implement quicksort")
```

### Hybrid

```nova
agent = hybrid.HybridAgent()  # Auto-selects backend
code = await agent.chat("Generate code")
```

## Files

- `python_agent.py` - Python-native implementation
- `hybrid.zn` - Nova-Python bridge
- `README_HYBRID.md` - This file

## Examples

- `examples/hybrid_agent_python.py` - Pure Python
- `examples/hybrid_agent_nova.zn` - Nova with Python
- `examples/hybrid_agent_both.py` - Both languages

## Documentation

See [HYBRID_AI_AGENT_GUIDE.md](../../docs/HYBRID_AI_AGENT_GUIDE.md) for complete guide.

## Installation

```bash
pip install claude-agent-sdk
export ANTHROPIC_API_KEY="your-key"
```

## Use Cases

1. **Python Build Tools + Nova Core**
   - Python automation
   - Nova performance

2. **Multi-Language Projects**
   - Best of both worlds
   - Flexible workflows

3. **Code Translation**
   - Python ↔ Nova
   - AI-powered migration

4. **Interactive Development**
   - Jupyter notebooks
   - Quick prototyping
