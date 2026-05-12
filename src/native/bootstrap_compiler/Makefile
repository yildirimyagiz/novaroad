# Bootstrap Compiler Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

SRCS = nova_bootstrap_lexer.c
OBJS = $(SRCS:.c=.o)

all: test_lexer

test_lexer: test_lexer.o nova_bootstrap_lexer.o
	$(CC) $(CFLAGS) -o $@ $^
	@echo "✅ Built test_lexer"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o test_lexer calma_lexer calma_parser bootstrap_lexer
	@echo "✅ Cleaned build artifacts"

test: test_lexer
	@echo "Running lexer test..."
	@./test_lexer

.PHONY: all clean test
