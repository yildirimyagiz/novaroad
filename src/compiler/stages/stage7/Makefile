CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I. -I../stage1 -I../stage4
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage7.a

libstage7.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libstage7.a
