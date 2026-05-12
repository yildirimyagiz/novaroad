# ============================================================
# Nova Execution Efficiency Layer + Performance Stabilizer
# ============================================================
CC       = cc
CFLAGS   = -std=c11 -O2 -Wall -Wextra -Wpedantic -march=native \
           -fstrict-aliasing -D_POSIX_C_SOURCE=200809L
LDFLAGS  = -lm

NOVA_SRCS = nova_graph.c nova_fusion.c nova_memory.c \
              nova_scheduler.c nova_backend.c nova_trace.c
STABILIZER_SRCS = performance_stabilizer.c variance_analyzer.c \
                  pacing_controller.c thermal_detector.c
NOVA_OBJS = $(NOVA_SRCS:.c=.o)
STAB_OBJS   = $(STABILIZER_SRCS:.c=.o)

.PHONY: all clean example bench stabilizer_demo

all: example bench stabilizer_demo

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

example: example.o $(NOVA_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

bench: nova_bench.o $(NOVA_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

stabilizer_demo: stabilizer_demo.o $(NOVA_OBJS) $(STAB_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o example bench stabilizer_demo
