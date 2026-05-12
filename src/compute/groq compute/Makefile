CC = gcc
CFLAGS = -O2 -std=c11 -Wall
LDFLAGS = -lm

SRC = main.c groq_compute_kernel.c groq_compute_energy.c groq_compute_scheduler.c groq_compute_global.c
OBJ = $(SRC:.c=.o)
TARGET = groq_compute

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
