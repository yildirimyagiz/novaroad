CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I. -I../stage1
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage2.a

libstage2.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libstage2.a
