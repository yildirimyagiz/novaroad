# Nova AI Agent Framework

AI-powered coding assistants for Nova development.

## Features

✅ **5 Specialized Agents**
- CodeAgent - Generate new code
- DebugAgent - Find and fix bugs
- RefactorAgent - Improve code quality
- TestAgent - Write comprehensive tests
- AnalysisAgent - Code review and analysis

✅ **Built on Claude Agent SDK**
- Powered by Claude AI
- Async/streaming responses
- Custom tools support
- Context-aware conversations

✅ **Nova-Native**
- Understands Nova syntax
- Knows Nova best practices
- Generates idiomatic code
- Performance-aware

## Quick Start

```nova
import ai.agents as agents
import asyncio

async def main():
    # Quick code generation
    code = await agents.quick_code("binary search in Nova")
    print(code)

asyncio.run(main())
```

## Installation

```bash
pip install claude-agent-sdk
export ANTHROPIC_API_KEY="your-key"
```

## Agents

### CodeAgent
Generate functions, classes, modules, algorithms.

### DebugAgent
Analyze errors, find bugs, suggest fixes.

### RefactorAgent
Improve readability, optimize performance, apply patterns.

### TestAgent
Generate unit tests, integration tests, edge cases.

### AnalysisAgent
Code review, performance analysis, security checks.

## Examples

- `examples/ai_agent_simple.zn` - Quick start
- `examples/ai_agent_demo.zn` - All agents showcase
- `examples/ai_agent_interactive.zn` - Chat interface
- `examples/ai_agent_project.zn` - Project builder

## Documentation

See [AI_AGENT_GUIDE.md](../../docs/AI_AGENT_GUIDE.md) for complete guide.

## Requirements

- Python 3.10+
- claude-agent-sdk
- Anthropic API key
