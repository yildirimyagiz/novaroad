CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I.
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage1.a

libstage1.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libstage1.a

.PHONY: all clean
