CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I.
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage5.a

libstage5.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(OBJS) libstage5.a
