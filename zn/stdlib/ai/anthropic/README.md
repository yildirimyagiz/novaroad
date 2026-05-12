# Anthropic AI Features for Nova

Advanced AI capabilities from Anthropic's ecosystem.

## Features

✅ **Prompt Engineering** - Best practices and patterns  
✅ **Prompt Caching** - Up to 90% cost savings  
✅ **Agent Skills** - Reusable AI capabilities  
✅ **Tool Use** - Function calling and computer use  
✅ **Best Practices** - From Anthropic's cookbook  

## Quick Start

```nova
import ai.anthropic as anthropic

# Prompt engineering
pe = anthropic.PromptEngineering()
prompt = pe.clear_and_direct("Write binary search in Nova")

# Prompt caching (90% savings!)
cache = anthropic.PromptCache()
cached = cache.create_cached_prompt(large_doc, "Summarize")

# Agent skills
skills = anthropic.AgentSkills()
result = await skills.execute_skill("file_operations", operation="read", path="file.txt")

# Tools
tools = anthropic.AnthropicTools()
result = await tools.execute_tool("bash", command="ls -la")
```

## Modules

- `prompt_engineering.zn` - Prompt patterns and optimization
- `prompt_cache.zn` - Cost-saving caching system
- `agent_skills.zn` - Reusable agent capabilities
- `tools.zn` - Tool use and computer control

## Cost Savings

| Feature | Savings | Use Case |
|---------|---------|----------|
| Prompt Caching | Up to 90% | Large repeated context |
| Few-Shot Learning | 30-50% | Reduce examples |
| Clear Prompts | 40-60% | Fewer retries |

**Example:**
- Without optimization: $100/month
- With caching + engineering: $20/month
- **Savings: 80%!**

## Examples

- `examples/anthropic_features_demo.zn` - Complete demo
- `docs/ANTHROPIC_COOKBOOK.md` - Recipes and patterns

## Documentation

See [ANTHROPIC_COOKBOOK.md](../../../docs/ANTHROPIC_COOKBOOK.md) for complete guide.

## Based On

- [Anthropic Cookbook](https://github.com/anthropics/claude-cookbooks) ⭐ 32K
- [Prompt Engineering Tutorial](https://github.com/anthropics/prompt-eng-interactive-tutorial) ⭐ 29K
- [Agent Skills](https://github.com/anthropics/skills) ⭐ 65K
- [Courses](https://github.com/anthropics/courses) ⭐ 18K
