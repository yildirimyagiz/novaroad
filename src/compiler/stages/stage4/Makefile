CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I. -I../stage1 -I../stage2 -I../stage3
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage4.a

libstage4.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libstage4.a
