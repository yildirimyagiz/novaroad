# Nova Language Support for VS Code

Official Visual Studio Code extension for the Nova programming language.

## Features

- **Syntax Highlighting** - Full syntax highlighting for `.zn` and `.nova` files
- **IntelliSense** - Code completion with keyword and type suggestions
- **Hover Information** - View type and documentation on hover
- **Go to Definition** - Jump to symbol definitions
- **Find All References** - Find all usages of a symbol
- **Code Formatting** - Format code on save
- **Error Detection** - Real-time error and warning reporting

## Installation

### From Marketplace (Coming Soon)

```
ext install nova.nova-language
```

### From VSIX

1. Download latest `.vsix` from releases
2. Run: `code --install-extension nova-language-*.vsix`

### From Source

```bash
cd vscode-extension
npm install
npm run compile
npm run package
code --install-extension *.vsix
```

## Requirements

- **Nova Language Server** (`znlsp`) must be installed
- Install from: https://github.com/nova/nova

## Extension Settings

This extension contributes the following settings:

* `nova.lsp.enable`: Enable/disable Nova Language Server
* `nova.lsp.path`: Path to `znlsp` executable
* `nova.lsp.trace`: LSP trace level (off/messages/verbose)
* `nova.format.onSave`: Format code on save

## Commands

- `Nova: Restart Language Server` - Restart the LSP server
- `Nova: Show Server Status` - Show LSP server status

## Quick Start

1. Install the extension
2. Open a `.zn` file
3. Start coding with full IDE support!

## Screenshots

![Syntax Highlighting](images/syntax-highlighting.png)
![Code Completion](images/completion.png)
![Hover Information](images/hover.png)

## Known Issues

- Semantic highlighting not yet implemented
- Some advanced refactorings in progress

## Release Notes

### 0.1.0

Initial release:
- Syntax highlighting
- Basic LSP features
- Code completion
- Navigation features

## Contributing

Contributions welcome! See [CONTRIBUTING.md](../../CONTRIBUTING.md)

## License

MIT
