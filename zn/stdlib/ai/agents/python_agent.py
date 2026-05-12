"""
Python-Native AI Agent Implementation
Direct integration with Claude Agent SDK
"""

import asyncio
import logging
from typing import AsyncIterator, List, Dict, Optional, Any
from pathlib import Path

try:
    from claude_agent_sdk import ClaudeSDKClient, ClaudeAgentOptions, query
    CLAUDE_SDK_AVAILABLE = True
except ImportError:
    CLAUDE_SDK_AVAILABLE = False
    logging.warning("Claude Agent SDK not installed. Install with: pip install claude-agent-sdk")


class PythonNovaAgent:
    """
    Python-native AI agent for Nova development
    
    This is the Python implementation that can be called from both Python and Nova.
    """
    
    def __init__(
        self,
        name: str = "PythonNovaAgent",
        system_prompt: Optional[str] = None,
        allowed_tools: Optional[List[str]] = None,
        cwd: str = ".",
        max_turns: int = 10,
        permission_mode: str = "manual",
        api_key: Optional[str] = None,
    ):
        if not CLAUDE_SDK_AVAILABLE:
            raise ImportError("claude-agent-sdk is required. Install with: pip install claude-agent-sdk")
        
        self.name = name
        self.system_prompt = system_prompt or self._default_system_prompt()
        self.allowed_tools = allowed_tools or ["Read", "Write", "Bash"]
        self.cwd = Path(cwd)
        self.max_turns = max_turns
        self.permission_mode = permission_mode
        
        # Create options
        self.options = ClaudeAgentOptions(
            system_prompt=self.system_prompt,
            allowed_tools=self.allowed_tools,
            cwd=str(self.cwd),
            max_turns=self.max_turns,
            permission_mode=self.permission_mode,
        )
        
        # Initialize client
        self.client = ClaudeSDKClient(options=self.options)
        
        # Context storage
        self.context: List[Dict[str, Any]] = []
        
        logging.info(f"{self.name} initialized (Python-native)")
    
    def _default_system_prompt(self) -> str:
        """Default system prompt for Nova coding"""
        return f"""You are {self.name}, an expert AI coding assistant for Nova programming language.

Nova Language Overview:
- Modern, high-performance language combining Python syntax with Rust performance
- Native GPU acceleration and SIMD support
- Built-in AI/ML capabilities
- Memory safety without garbage collection overhead
- Zero-cost abstractions
- First-class async/await support

Your Expertise:
1. Writing idiomatic Nova code
2. Performance optimization
3. Memory management and ownership
4. Debugging and testing
5. AI/ML integration
6. Best practices and design patterns

Code Generation Guidelines:
- Use type annotations
- Add comprehensive docstrings
- Handle errors gracefully
- Write modular, reusable code
- Optimize for clarity first, performance second (unless specified)
- Follow Nova naming conventions

Always provide clear explanations and consider trade-offs."""
    
    async def query(self, prompt: str) -> AsyncIterator[str]:
        """
        Send a query to Claude and stream responses
        
        Args:
            prompt: The query/task for the agent
            
        Yields:
            Response messages from Claude
        """
        logging.info(f"{self.name} processing query: {prompt[:100]}...")
        
        async for message in query(prompt=prompt, options=self.options):
            # Extract text content
            if hasattr(message, 'content'):
                for block in message.content:
                    if hasattr(block, 'text'):
                        yield block.text
            else:
                yield str(message)
    
    async def chat(self, message: str) -> str:
        """
        Single chat interaction (convenience method)
        
        Args:
            message: User message
            
        Returns:
            Agent's complete response
        """
        self.context.append({"role": "user", "content": message})
        
        response_parts = []
        async for text in self.query(message):
            response_parts.append(text)
        
        response = "".join(response_parts)
        self.context.append({"role": "assistant", "content": response})
        
        return response
    
    def reset_context(self):
        """Clear conversation history"""
        self.context = []
        logging.info(f"{self.name} context cleared")
    
    def get_context(self) -> List[Dict[str, Any]]:
        """Get conversation history"""
        return self.context.copy()


class PythonCodeAgent(PythonNovaAgent):
    """Python-native code generation agent"""
    
    def __init__(self, **kwargs):
        kwargs['name'] = kwargs.get('name', 'PythonCodeAgent')
        kwargs['system_prompt'] = self._code_system_prompt()
        super().__init__(**kwargs)
    
    def _code_system_prompt(self) -> str:
        return """You are PythonCodeAgent, a Nova code generation specialist.

Focus on:
- Writing clean, efficient Nova code
- Comprehensive documentation
- Type safety and error handling
- Performance optimization when appropriate
- Best practices and design patterns

Always include:
- Type annotations
- Docstrings with examples
- Error handling
- Usage examples"""
    
    async def generate_function(
        self,
        description: str,
        function_name: Optional[str] = None,
        return_type: Optional[str] = None,
        parameters: Optional[List[str]] = None,
    ) -> str:
        """
        Generate a Nova function
        
        Args:
            description: What the function should do
            function_name: Optional function name
            return_type: Optional return type
            parameters: Optional parameter list
            
        Returns:
            Generated function code
        """
        prompt = f"Generate a Nova function: {description}\n"
        
        if function_name:
            prompt += f"Function name: {function_name}\n"
        if return_type:
            prompt += f"Return type: {return_type}\n"
        if parameters:
            prompt += f"Parameters: {', '.join(parameters)}\n"
        
        prompt += "\nInclude: docstring, type annotations, error handling, example usage"
        
        return await self.chat(prompt)
    
    async def generate_class(
        self,
        description: str,
        class_name: Optional[str] = None,
        base_classes: Optional[List[str]] = None,
    ) -> str:
        """Generate a Nova class"""
        prompt = f"Generate a Nova class: {description}\n"
        
        if class_name:
            prompt += f"Class name: {class_name}\n"
        if base_classes:
            prompt += f"Inherit from: {', '.join(base_classes)}\n"
        
        prompt += "\nInclude: constructor, methods, properties, docstrings, examples"
        
        return await self.chat(prompt)
    
    async def generate_module(
        self,
        description: str,
        module_name: str,
        components: Optional[List[str]] = None,
    ) -> str:
        """Generate a complete Nova module"""
        prompt = f"Generate a complete Nova module: {description}\n"
        prompt += f"Module name: {module_name}\n"
        
        if components:
            prompt += f"Include: {', '.join(components)}\n"
        
        prompt += "\nCreate production-ready module with: docstring, imports, exports, examples"
        
        return await self.chat(prompt)


class PythonDebugAgent(PythonNovaAgent):
    """Python-native debugging agent"""
    
    def __init__(self, **kwargs):
        kwargs['name'] = kwargs.get('name', 'PythonDebugAgent')
        kwargs['system_prompt'] = self._debug_system_prompt()
        super().__init__(**kwargs)
    
    def _debug_system_prompt(self) -> str:
        return """You are PythonDebugAgent, a Nova debugging specialist.

Debugging approach:
1. Understand the error message
2. Identify the problematic code
3. Analyze root cause
4. Suggest clear fix
5. Explain prevention

Always provide:
- Clear error explanation
- Root cause analysis
- Step-by-step fix
- Prevention tips"""
    
    async def analyze_error(
        self,
        error_message: str,
        code: Optional[str] = None,
    ) -> str:
        """Analyze error and suggest fix"""
        prompt = f"Analyze this Nova error:\n{error_message}\n"
        
        if code:
            prompt += f"\nCode context:\n```nova\n{code}\n```\n"
        
        prompt += "\nProvide: error explanation, root cause, fix, prevention"
        
        return await self.chat(prompt)
    
    async def find_bugs(self, code: str) -> str:
        """Find potential bugs in code"""
        prompt = f"Find bugs in this Nova code:\n```nova\n{code}\n```\n"
        prompt += "Check for: logic errors, type issues, memory issues, edge cases"
        
        return await self.chat(prompt)


# Quick helper functions
async def quick_code_python(prompt: str, save_to: Optional[str] = None) -> str:
    """
    Quick code generation from Python
    
    Args:
        prompt: What to generate
        save_to: Optional file to save to
        
    Returns:
        Generated code
    """
    agent = PythonCodeAgent()
    code = await agent.chat(prompt)
    
    if save_to:
        Path(save_to).parent.mkdir(parents=True, exist_ok=True)
        Path(save_to).write_text(code)
        print(f"✅ Code saved to {save_to}")
    
    return code


def quick_code_sync(prompt: str, save_to: Optional[str] = None) -> str:
    """Synchronous wrapper for quick_code_python"""
    return asyncio.run(quick_code_python(prompt, save_to))


# Factory function
def create_python_agent(
    agent_type: str = "code",
    **kwargs
) -> PythonNovaAgent:
    """
    Create Python-native agent
    
    Args:
        agent_type: Type of agent (general, code, debug)
        **kwargs: Agent options
        
    Returns:
        Agent instance
    """
    agents = {
        "general": PythonNovaAgent,
        "code": PythonCodeAgent,
        "debug": PythonDebugAgent,
    }
    
    AgentClass = agents.get(agent_type, PythonNovaAgent)
    return AgentClass(**kwargs)
