# Nova Language Server Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = 

TARGET = znlsp
SOURCES = main.c lsp_protocol.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Built $(TARGET)"

%.o: %.c lsp_protocol.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJECTS)
	@echo "🧹 Cleaned"

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "📦 Installed to /usr/local/bin/$(TARGET)"

test: $(TARGET)
	@echo "🧪 Testing LSP server..."
	@echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' | \
		./$(TARGET) --stdio 2>&1 | head -20

.PHONY: vscode-extension
vscode-extension:
	@echo "📦 Building VS Code extension..."
	cd vscode-extension && npm install && npm run compile
	@echo "✅ Extension built in vscode-extension/out/"

.PHONY: package-extension
package-extension: vscode-extension
	cd vscode-extension && npm run package
	@echo "✅ Extension packaged: vscode-extension/*.vsix"
