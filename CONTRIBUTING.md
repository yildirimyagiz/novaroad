# Contributing to Nova

Thank you for your interest in contributing to Nova! This document provides guidelines and instructions for contributors.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help create a welcoming environment

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/nova.git`
3. Create a branch: `git checkout -b feature/your-feature`
4. Make your changes
5. Run tests: `cd build && ctest`
6. Commit: `git commit -m "Description of changes"`
7. Push: `git push origin feature/your-feature`
8. Open a Pull Request

## Code Style

### C Code

- Follow Linux kernel style (with some modifications)
- Use `clang-format` with provided `.clang-format`
- Maximum line length: 100 characters
- Use snake_case for functions and variables
- Use CamelCase for types

### Example

```c
/**
 * @brief Function description
 * @param size Size parameter
 * @return Pointer to allocated memory
 */
void *nova_alloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    
    return malloc(size);
}
```

### Naming Conventions

- **Functions**: `nova_subsystem_action()` (e.g., `nova_gc_collect()`)
- **Types**: `nova_type_t` (e.g., `nova_tensor_t`)
- **Macros**: `NOVA_CONSTANT` (e.g., `NOVA_PAGE_SIZE`)
- **Files**: `lowercase.c/.h` (e.g., `scheduler.c`)

## Project Structure

```
src/
├── kernel/     # OS kernel code
├── runtime/    # Runtime system
├── compiler/   # Compiler implementation
├── ai/         # AI/ML subsystem
├── security/   # Security features
└── std/        # Standard library
```

## Areas to Contribute

### 🟢 Good First Issues

- Documentation improvements
- Test coverage
- Code formatting
- Simple bug fixes

### 🟡 Intermediate

- Compiler optimizations
- Runtime performance
- New standard library features
- Platform support

### 🔴 Advanced

- Kernel development
- Type system enhancements
- JIT compiler
- LLVM backend integration

## Testing

### Unit Tests

Place tests in `tests/unit/`:

```c
#include "std/collections.h"
#include <assert.h>

void test_vec_push_pop(void)
{
    nova_vec_t *vec = nova_vec_new();
    
    int value = 42;
    nova_vec_push(vec, &value);
    
    assert(nova_vec_len(vec) == 1);
    assert(*(int *)nova_vec_pop(vec) == 42);
    
    nova_vec_destroy(vec);
}
```

### Integration Tests

Place in `tests/integration/` and test full pipelines.

## Documentation

- Use Doxygen-style comments for public APIs
- Update README.md when adding features
- Add architecture docs to ARCHITECTURE.md
- Include examples in `examples/` directory

## Performance

- Profile before optimizing
- Benchmark performance-critical code
- Document performance characteristics
- Consider algorithmic complexity

## Security

- No hardcoded secrets
- Validate all inputs
- Use safe string functions
- Report vulnerabilities privately

## Pull Request Process

1. **Description**: Clearly describe what and why
2. **Tests**: Add tests for new functionality
3. **Documentation**: Update relevant docs
4. **CI**: Ensure all checks pass
5. **Review**: Address review comments
6. **Merge**: Maintainer will merge when ready

## Commit Messages

Follow conventional commits:

```
type(scope): short description

Longer description if needed

Fixes #123
```

**Types**: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

**Examples**:
- `feat(compiler): add constant folding optimization`
- `fix(runtime): prevent memory leak in GC`
- `docs(readme): update build instructions`

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

- Open an issue for questions
- Join our Discord/Slack (coming soon)
- Email: dev@nova.org (placeholder)

Thank you for contributing to Nova! 🚀
