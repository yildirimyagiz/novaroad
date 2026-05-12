CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I. -I../stage1 -I../stage4
SRCS = $(shell find . -name '*.c')
OBJS = $(SRCS:.c=.o)

all: libstage6.a nova6

libstage6.a: $(OBJS)
	ar rcs $@ $^

nova6: stage6_cli.c libstage6.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) libstage6.a nova6
