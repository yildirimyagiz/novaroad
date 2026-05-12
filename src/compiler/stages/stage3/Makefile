CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I. -I../stage1 -I../stage2
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage3.a

libstage3.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libstage3.a
