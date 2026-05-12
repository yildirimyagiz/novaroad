# Nova Ultra Matmul Engine v3.0 — Makefile
# macOS (Apple Silicon): clang + Metal shader compilation

CC      = clang
CFLAGS  = -O3 -march=native -ffast-math -funroll-loops \
           -Wall -Wextra \
           -framework Foundation \
           -framework Metal \
           -framework MetalKit \
           -arch arm64 \
           -fblocks

TARGET  = zenith_ultra
SRCS    = main.c zenith_matmul.c zenith_metal_dispatch.m
METAL   = zenith_matmul.metal
METALLIB= default.metallib

.PHONY: all clean run

all: $(TARGET) $(METALLIB)

$(TARGET): $(SRCS) zenith_matmul.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)
	@echo "✅ CPU binary + Metal dispatch built: $(TARGET)"

$(METALLIB): $(METAL)
	xcrun -sdk macosx metal -O2 -c $(METAL) -o zenith_matmul.air
	xcrun -sdk macosx metallib zenith_matmul.air -o $(METALLIB)
	@echo "✅ Metal GPU library built: $(METALLIB)"

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) *.air $(METALLIB)

# Performance profiling target
profile: all
	xcrun xctrace record --template "Metal System Trace" \
	    --launch -- ./$(TARGET)
