// nova Groq Compute: Tile Sweep Metal GPU Matmul Shader
// Target: 500-1000 GFLOPS via tile optimization
// Kernels: 16x16x16, 16x16x32, 32x32x8, 32x32x16

#include <metal_stdlib>
using namespace metal;

struct MatmulParams {
    uint M;
    uint N;
    uint K;
    uint lda;
    uint ldb;
    uint ldc;
};

// 16x16x16 tiled
kernel void matmul_tiled_fp32_16x16x16(
    device const float* A       [[buffer(0)]],
    device const float* B       [[buffer(1)]],
    device float*       C       [[buffer(2)]],
    constant MatmulParams& p    [[buffer(3)]],
    ushort2 tg_pos              [[threadgroup_position_in_grid]],
    ushort2 th_pos              [[thread_position_in_threadgroup]]
) {
    const uint row = uint(tg_pos.y) * 16 + uint(th_pos.y);
    const uint col = uint(tg_pos.x) * 16 + uint(th_pos.x);

    threadgroup float As[16][16];
    threadgroup float Bs[16][16];

    float acc = 0.0f;

    for (uint k0 = 0; k0 < p.K; k0 += 16) {
        const uint tlin = uint(th_pos.y) * 16 + uint(th_pos.x);
        const uint a_loads = 16 * 16;
        const uint b_loads = 16 * 16;

        for (uint idx = tlin; idx < a_loads; idx += 16 * 16) {
            const uint ar = idx / 16;
            const uint ac = idx - ar * 16;
            const uint r = uint(tg_pos.y) * 16 + ar;
            const uint c = k0 + ac;
            As[ar][ac] = (r < p.M && c < p.K) ? A[r * p.lda + c] : 0.0f;
        }

        for (uint idx = tlin; idx < b_loads; idx += 16 * 16) {
            const uint br = idx / 16;
            const uint bc = idx - br * 16;
            const uint r = k0 + br;
            const uint c = uint(tg_pos.x) * 16 + bc;
            Bs[br][bc] = (r < p.K && c < p.N) ? B[r * p.ldb + c] : 0.0f;
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);

        if (row < p.M && col < p.N) {
            for (uint k = 0; k < 16; ++k) {
                acc += As[uint(th_pos.y)][k] * Bs[k][uint(th_pos.x)];
            }
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (row < p.M && col < p.N) {
        C[row * p.ldc + col] = acc;
    }
}

// 16x16x32 tiled
kernel void matmul_tiled_fp32_16x16x32(
    device const float* A       [[buffer(0)]],
    device const float* B       [[buffer(1)]],
    device float*       C       [[buffer(2)]],
    constant MatmulParams& p    [[buffer(3)]],
    ushort2 tg_pos              [[threadgroup_position_in_grid]],
    ushort2 th_pos              [[thread_position_in_threadgroup]]
) {
    const uint row = uint(tg_pos.y) * 16 + uint(th_pos.y);
    const uint col = uint(tg_pos.x) * 16 + uint(th_pos.x);

    threadgroup float As[16][32];
    threadgroup float Bs[32][16];

    float acc = 0.0f;

    for (uint k0 = 0; k0 < p.K; k0 += 32) {
        const uint tlin = uint(th_pos.y) * 16 + uint(th_pos.x);
        const uint a_loads = 16 * 32;
        const uint b_loads = 32 * 16;

        for (uint idx = tlin; idx < a_loads; idx += 16 * 16) {
            const uint ar = idx / 32;
            const uint ac = idx - ar * 32;
            const uint r = uint(tg_pos.y) * 16 + ar;
            const uint c = k0 + ac;
            As[ar][ac] = (r < p.M && c < p.K) ? A[r * p.lda + c] : 0.0f;
        }

        for (uint idx = tlin; idx < b_loads; idx += 16 * 16) {
            const uint br = idx / 16;
            const uint bc = idx - br * 16;
            const uint r = k0 + br;
            const uint c = uint(tg_pos.x) * 16 + bc;
            Bs[br][bc] = (r < p.K && c < p.N) ? B[r * p.ldb + c] : 0.0f;
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);

        if (row < p.M && col < p.N) {
            for (uint k = 0; k < 32; ++k) {
                acc += As[uint(th_pos.y)][k] * Bs[k][uint(th_pos.x)];
            }
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (row < p.M && col < p.N) {
        C[row * p.ldc + col] = acc;
    }
}

// 32x32x8 tiled for better occupancy
kernel void matmul_tiled_fp32_32x32x8(
    device const float* A       [[buffer(0)]],
    device const float* B       [[buffer(1)]],
    device float*       C       [[buffer(2)]],
    constant MatmulParams& p    [[buffer(3)]],
    ushort2 tg_pos              [[threadgroup_position_in_grid]],
    ushort2 th_pos              [[thread_position_in_threadgroup]]
) {
    const uint row = uint(tg_pos.y) * 32 + uint(th_pos.y);
    const uint col = uint(tg_pos.x) * 32 + uint(th_pos.x);

    threadgroup float As[32][8];
    threadgroup float Bs[8][32];

    float acc = 0.0f;

    for (uint k0 = 0; k0 < p.K; k0 += 8) {
        const uint tlin = uint(th_pos.y) * 32 + uint(th_pos.x);
        const uint a_loads = 32 * 8;
        const uint b_loads = 8 * 32;

        for (uint idx = tlin; idx < a_loads; idx += 32 * 32) {
            const uint ar = idx / 8;
            const uint ac = idx - ar * 8;
            const uint r = uint(tg_pos.y) * 32 + ar;
            const uint c = k0 + ac;
            As[ar][ac] = (r < p.M && c < p.K) ? A[r * p.lda + c] : 0.0f;
        }

        for (uint idx = tlin; idx < b_loads; idx += 32 * 32) {
            const uint br = idx / 32;
            const uint bc = idx - br * 32;
            const uint r = k0 + br;
            const uint c = uint(tg_pos.x) * 32 + bc;
            Bs[br][bc] = (r < p.K && c < p.N) ? B[r * p.ldb + c] : 0.0f;
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);

        if (row < p.M && col < p.N) {
            acc += As[uint(th_pos.y)][0] * Bs[0][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][1] * Bs[1][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][2] * Bs[2][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][3] * Bs[3][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][4] * Bs[4][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][5] * Bs[5][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][6] * Bs[6][uint(th_pos.x)];
            acc += As[uint(th_pos.y)][7] * Bs[7][uint(th_pos.x)];
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (row < p.M && col < p.N) {
        C[row * p.ldc + col] = acc;
    }
}

// 32x32x16 tiled for balanced performance
kernel void matmul_tiled_fp32_32x32x16(
    device const float* A       [[buffer(0)]],
    device const float* B       [[buffer(1)]],
    device float*       C       [[buffer(2)]],
    constant MatmulParams& p    [[buffer(3)]],
    ushort2 tg_pos              [[threadgroup_position_in_grid]],
    ushort2 th_pos              [[thread_position_in_threadgroup]]
) {
    const uint row = uint(tg_pos.y) * 32 + uint(th_pos.y);
    const uint col = uint(tg_pos.x) * 32 + uint(th_pos.x);

    threadgroup float As[32][16];
    threadgroup float Bs[16][32];

    float acc = 0.0f;

    for (uint k0 = 0; k0 < p.K; k0 += 16) {
        const uint tlin = uint(th_pos.y) * 32 + uint(th_pos.x);
        const uint a_loads = 32 * 16;
        const uint b_loads = 16 * 32;

        for (uint idx = tlin; idx < a_loads; idx += 32 * 32) {
            const uint ar = idx / 16;
            const uint ac = idx - ar * 16;
            const uint r = uint(tg_pos.y) * 32 + ar;
            const uint c = k0 + ac;
            As[ar][ac] = (r < p.M && c < p.K) ? A[r * p.lda + c] : 0.0f;
        }

        for (uint idx = tlin; idx < b_loads; idx += 32 * 32) {
            const uint br = idx / 16;
            const uint bc = idx - br * 16;
            const uint r = k0 + br;
            const uint c = uint(tg_pos.x) * 32 + bc;
            Bs[br][bc] = (r < p.K && c < p.N) ? B[r * p.ldb + c] : 0.0f;
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);

        if (row < p.M && col < p.N) {
            for (uint k = 0; k < 16; ++k) {
                acc += As[uint(th_pos.y)][k] * Bs[k][uint(th_pos.x)];
            }
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (row < p.M && col < p.N) {
        C[row * p.ldc + col] = acc;
    }
}
