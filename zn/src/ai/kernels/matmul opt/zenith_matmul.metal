#include <metal_stdlib>
#include <metal_simdgroup_matrix>

using namespace metal;

// ─── THE ARMY KERNEL (V17.0) ──────────────────────────────────
// Target: >5.3 TFLOPS (Absolute M1 Saturation)
// Strategic Architecture: High-Occupancy + No-Bank-Conflict SMEM
// ──────────────────────────────────────────────────────────────

[[kernel]]
void zenith_convert_f32_to_f16(
    device const float* in  [[buffer(0)]],
    device half*        out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]
) {
    out[gid] = (half)in[gid];
}

kernel void zenith_v17_army(
    device const half*  A [[buffer(0)]],
    device const half*  B [[buffer(1)]],
    device half*        C [[buffer(2)]],
    constant int&       M [[buffer(3)]],
    constant int&       N [[buffer(4)]],
    constant int&       K [[buffer(5)]],
    uint2 block_id  [[threadgroup_position_in_grid]],
    uint2 thread_id [[thread_position_in_threadgroup]]
) {
    // 128x32 A tile + 32x128 B tile = 16KB SMEM
    // Using 128x128 output tile to saturate ALUs
    threadgroup half Asub[128][32 + 8]; // +8 to kill bank conflicts
    threadgroup half Bsub[32][128 + 8];

    const int tid = (int)thread_id.y * 16 + (int)thread_id.x; // 128 threads (16x8)
    const int sg_id = tid / 32;                             // 4 simdgroups (0..3)

    // SG calculates 64x64 output per TG? No, 128x128 divided by 4 SGs = 64x64 per SG
    simdgroup_matrix<half, 8, 8> acc[8][8];
    for(int i=0; i<8; i++) for(int j=0; j<8; j++) acc[i][j] = simdgroup_matrix<half, 8, 8>(0.0h);

    const int num_tiles = (K + 31) / 32;

    for (int t = 0; t < num_tiles; t++) {
        const int k_base = t * 32;

        // HIGH-SPEED MANIFEST (128 threads loading 8192 elements)
        // Load Asub (128x32)
        int gr = block_id.y * 128 + tid;
        if (gr < M) {
            for(int c=0; c<32; c+=8) {
                if (k_base + c + 7 < K) {
                    *(threadgroup half4*)&Asub[tid][c]   = *(device const half4*)&A[gr * K + k_base + c];
                    *(threadgroup half4*)&Asub[tid][c+4] = *(device const half4*)&A[gr * K + k_base + c + 4];
                } else {
                    for(int k=0; k<8; k++) Asub[tid][c+k] = (k_base+c+k < K) ? A[gr * K + k_base + c + k] : 0.0h;
                }
            }
        } else {
            for(int c=0; c<32; c++) Asub[tid][c] = 0.0h;
        }

        // Load Bsub (32x128) - 128 threads, 4096 elements. Each loads 32 elements.
        // Each thread loads 1/4 of a row.
        int r_b = tid / 4; // 0..31
        int c_b = (tid % 4) * 32; // 0, 32, 64, 96
        int gr_b = k_base + r_b;
        if (gr_b < K) {
            for(int c=0; c<32; c+=8) {
                int gc = block_id.x * 128 + c_b + c;
                if (gc + 7 < N) {
                    *(threadgroup half4*)&Bsub[r_b][c_b + c]   = *(device const half4*)&B[gr_b * N + gc];
                    *(threadgroup half4*)&Bsub[r_b][c_b + c + 4] = *(device const half4*)&B[gr_b * N + gc + 4];
                } else {
                    for(int k=0; k<8; k++) Bsub[r_b][c_b+c+k] = (gc+k < N) ? B[gr_b*N + gc+k] : 0.0h;
                }
            }
        } else {
            for(int c=0; c<32; c++) Bsub[r_b][c_b+c] = 0.0h;
        }

        threadgroup_barrier(mem_flags::mem_threadgroup);

        // AMX Computation: SG handles 64x64 area within 128x128
        // SG0: (0,0), SG1: (0,64), SG2: (64,0), SG3: (64,64)
        int sg_row_off = (sg_id / 2) * 64;
        int sg_col_off = (sg_id % 2) * 64;

        #pragma unroll
        for (int k = 0; k < 32; k += 8) {
            simdgroup_matrix<half, 8, 8> ma[8];
            simdgroup_matrix<half, 8, 8> mb[8];

            for(int i=0; i<8; i++) simdgroup_load(ma[i], &Asub[sg_row_off + i*8][k], 32 + 8);
            for(int j=0; j<8; j++) simdgroup_load(mb[j], &Bsub[k][sg_col_off + j*8], 128 + 8);

            for(int i=0; i<8; i++) {
                for(int j=0; j<8; j++) {
                    simdgroup_multiply_accumulate(acc[i][j], ma[i], mb[j], acc[i][j]);
                }
            }
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    // Results Store
    int sg_row_off = (sg_id / 2) * 64;
    int sg_col_off = (sg_id % 2) * 64;
    for(int i=0; i<8; i++) {
        for(int j=0; j<8; j++) {
            int gr = block_id.y * 128 + sg_row_off + i*8;
            int gc = block_id.x * 128 + sg_col_off + j*8;
            if (gr < M && gc < N) {
                simdgroup_store(acc[i][j], &C[gr * N + gc], N);
            }
        }
    }
}
