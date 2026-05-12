# Nova Ultra Matmul Engine v3.0 — Makefile
# macOS (Apple Silicon): clang + Metal shader compilation

CC      = clang
CFLAGS  = -O3 -march=native -ffast-math -funroll-loops \
           -Wall -Wextra \
           -framework Foundation \
           -framework Metal \
           -framework MetalKit \
           -arch arm64

TARGET  = nova_ultra
SRCS    = main.c nova_matmul.c
METAL   = nova_matmul.metal
METALLIB= default.metallib

.PHONY: all clean run

all: $(TARGET) $(METALLIB)

$(TARGET): $(SRCS) nova_matmul.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)
	@echo "✅ CPU binary built: $(TARGET)"

$(METALLIB): $(METAL)
	xcrun -sdk macosx metal -O2 -c $(METAL) -o nova_matmul.air
	xcrun -sdk macosx metallib nova_matmul.air -o $(METALLIB)
	@echo "✅ Metal GPU library built: $(METALLIB)"

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) *.air $(METALLIB)

# Performance profiling target
profile: all
	xcrun xctrace record --template "Metal System Trace" \
	    --launch -- ./$(TARGET)
