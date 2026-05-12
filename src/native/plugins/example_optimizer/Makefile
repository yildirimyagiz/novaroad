# Makefile for Example Optimizer Plugin

# Detect OS
UNAME_S := $(shell uname -s)

# Plugin name
PLUGIN_NAME = example_optimizer

# Compiler
CC = gcc

# Flags
CFLAGS = -Wall -Wextra -fPIC -I../../native/include
LDFLAGS = -shared

# Output
ifeq ($(UNAME_S),Darwin)
    # macOS
    PLUGIN_EXT = dylib
    LDFLAGS += -dynamiclib
else ifeq ($(UNAME_S),Linux)
    # Linux
    PLUGIN_EXT = so
else
    # Windows
    PLUGIN_EXT = dll
endif

TARGET = $(PLUGIN_NAME).$(PLUGIN_EXT)

# Source files
SRCS = optimizer_plugin.c
OBJS = $(SRCS:.c=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Built plugin: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned plugin build artifacts"

install: $(TARGET)
	@mkdir -p ../../native/plugins
	cp $(TARGET) ../../native/plugins/
	@echo "Installed plugin to ../../native/plugins/"

.PHONY: all clean install
