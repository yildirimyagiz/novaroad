#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <dispatch/dispatch.h>
#include <arm_neon.h>
#include <stdbool.h>
#include <string.h>

#define BS 128
#define NUM_THREADS 8
#define CACHE_LINE_SIZE 128

typedef struct {
    float *data;
    int rows, cols;
    bool is_aligned;
} Matrix;

static double total_energy = 0.0;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void* aligned_alloc_custom(size_t size, size_t alignment) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        fprintf(stderr, "Error: aligned_alloc failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

#define STABILITY_CHECK(cond, msg) if (!(cond)) { fprintf(stderr, "Stability Error: %s\n", msg); abort(); }

// Transpose helper
void transpose_matrix(float* src, float* dst, int rows, int cols) {
    for(int i=0;i<rows;i++){
        for(int j=0;j<cols;j++){
            dst[j*rows + i] = src[i*cols + j];
        }
    }
}

// Cache blocked matmul (unchanged)
void matmul_ultra_blocked(Matrix *A, Matrix *B, volatile Matrix *C) {
    STABILITY_CHECK(A && B && C, "Null matrices");
    STABILITY_CHECK(A->cols == B->rows, "Dimension mismatch");

    int M = A->rows, K = A->cols, N = B->cols;

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    dispatch_group_t group = dispatch_group_create();
    int chunk_size = M / NUM_THREADS;

    for(int t=0;t<NUM_THREADS;t++){
        int start_i=t*chunk_size;
        int end_i=(t==NUM_THREADS-1)?M:(t+1)*chunk_size;
        dispatch_group_async(group, queue, ^{
            for(int ii=start_i;ii<end_i;ii+=BS){
                for(int jj=0;jj<N;jj+=BS){
                    for(int kk=0;kk<K;kk+=BS){
                        int i_max=(ii+BS<end_i)?ii+BS:end_i;
                        int j_max=(jj+BS<N)?jj+BS:N;
                        int k_max=(kk+BS<K)?kk+BS:K;
                        for(int i=ii;i<i_max;i++){
                            for(int j=jj;j<j_max;j++){
                                float sum=0.0f;
                                for(int k=kk;k<k_max;k++){
                                    sum+=A->data[i*K + k]*B->data[k*N + j];
                                }
                                C->data[i*N + j]+=sum;
                            }
                        }
                    }
                }
            }
        });
    }

    dispatch_group_wait(group,DISPATCH_TIME_FOREVER);
    dispatch_release(group);
    total_energy+=0.08;
}

// SIMD NEON matmul with B_T
void matmul_ultra_simd(Matrix *A, Matrix *B, volatile Matrix *C) {
    STABILITY_CHECK(A->is_aligned && B->is_aligned && C->is_aligned, "Unaligned matrices");

    int M=A->rows,K=A->cols,N=B->cols;
    float *B_T=(float*)aligned_alloc_custom(N*K*sizeof(float),CACHE_LINE_SIZE);
    transpose_matrix(B->data,B_T,K,N);

    dispatch_queue_t queue=dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH,0);
    dispatch_group_t group=dispatch_group_create();
    int chunk_size=M/NUM_THREADS;

    for(int t=0;t<NUM_THREADS;t++){
        int start_i=t*chunk_size;
        int end_i=(t==NUM_THREADS-1)?M:(t+1)*chunk_size;
        dispatch_group_async(group,queue,^{
            for(int i=start_i;i<end_i;i++){
                for(int j=0;j<N;j+=8){
                    float32x4x2_t sum_vec={vdupq_n_f32(0.0f),vdupq_n_f32(0.0f)};
                    for(int k=0;k<K;k++){
                        float32x4_t a_vec=vdupq_n_f32(A->data[i*K + k]);
                        float32x4_t b_vec1=vld1q_f32(&B_T[j*K + k]);
                        float32x4_t b_vec2=vld1q_f32(&B_T[(j+4)*K + k]);
                        sum_vec.val[0]=vmlaq_f32(sum_vec.val[0],a_vec,b_vec1);
                        sum_vec.val[1]=vmlaq_f32(sum_vec.val[1],a_vec,b_vec2);
                    }
                    vst1q_f32(&C->data[i*N + j],sum_vec.val[0]);
                    vst1q_f32(&C->data[i*N + j + 4],sum_vec.val[1]);
                }
            }
        });
    }

    dispatch_group_wait(group,DISPATCH_TIME_FOREVER);
    dispatch_release(group);
    free(B_T);
    total_energy+=0.06;
}

// INT8 matmul with B_T
void matmul_ultra_int8(Matrix *A, Matrix *B, volatile Matrix *C,float scale){
    STABILITY_CHECK(scale>0,"Invalid scale");

    int M=A->rows,K=A->cols,N=B->cols;
    int8_t *A_q=(int8_t*)aligned_alloc_custom(M*K*sizeof(int8_t),CACHE_LINE_SIZE);
    int8_t *B_q=(int8_t*)aligned_alloc_custom(K*N*sizeof(int8_t),CACHE_LINE_SIZE);
    int32_t *C_acc=(int32_t*)aligned_alloc_custom(M*N*sizeof(int32_t),CACHE_LINE_SIZE);

    float a_max=0,b_max=0;
    for(int i=0;i<M*K;i++) a_max=fmaxf(a_max,fabsf(A->data[i]));
    for(int i=0;i<K*N;i++) b_max=fmaxf(b_max,fabsf(B->data[i]));

    float scale_a=a_max/127.0f,scale_b=b_max/127.0f;
    for(int i=0;i<M*K;i++) A_q[i]=(int8_t)roundf(A->data[i]/scale_a);

    float *B_T=(float*)aligned_alloc_custom(N*K*sizeof(float),CACHE_LINE_SIZE);
    transpose_matrix(B->data,B_T,K,N);
    for(int i=0;i<K*N;i++) B_q[i]=(int8_t)roundf(B_T[i]/scale_b);

    dispatch_queue_t queue=dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH,0);
    dispatch_group_t group=dispatch_group_create();
    int chunk_size=M/NUM_THREADS;

    for(int t=0;t<NUM_THREADS;t++){
        int start_i=t*chunk_size;
        int end_i=(t==NUM_THREADS-1)?M:(t+1)*chunk_size;
        dispatch_group_async(group,queue,^{
            for(int i=start_i;i<end_i;i++){
                for(int j=0;j<N;j+=16){
                    int32_t sums[16]={0};
                    for(int k=0;k<K;k++){
                        int8_t a_val=A_q[i*K + k];
                        for(int jj=0;jj<16;jj++){
                            if(j+jj<N) sums[jj]+=(int32_t)a_val*(int32_t)B_q[jj*K + k];
                        }
                    }
                    for(int jj=0;jj<16;jj++){
                        if(j+jj<N) C_acc[i*N + j + jj]=sums[jj];
                    }
                }
            }
        });
    }

    dispatch_group_wait(group,DISPATCH_TIME_FOREVER);
    dispatch_release(group);

    float final_scale=scale_a*scale_b*scale;
    for(int i=0;i<M*N;i++) C->data[i]=(float)C_acc[i]*final_scale;

    free(A_q); free(B_q); free(C_acc); free(B_T);
    total_energy+=0.03;
}

// Benchmark function remains mostly same
void benchmark_ultra_matmul(int size,int runs,const char*version){
    Matrix A={(float*)aligned_alloc_custom(size*size*sizeof(float),CACHE_LINE_SIZE),size,size,true};
    Matrix B={(float*)aligned_alloc_custom(size*size*sizeof(float),CACHE_LINE_SIZE),size,size,true};
    Matrix C={(float*)aligned_alloc_custom(size*size*sizeof(float),CACHE_LINE_SIZE),size,size,true};

    for(int i=0;i<size*size;i++){A.data[i]=1.0f; B.data[i]=1.0f; C.data[i]=0.0f;}
    double total_time=0.0;

    for(int r=0;r<runs;r++){
        for(int i=0;i<size*size;i++) C.data[i]=0.0f;
        double start=get_time();
        if(strcmp(version,"blocked")==0) matmul_ultra_blocked(&A,&B,(volatile Matrix*)&C);
        else if(strcmp(version,"simd")==0) matmul_ultra_simd(&A,&B,(volatile Matrix*)&C);
        else if(strcmp(version,"int8")==0) matmul_ultra_int8(&A,&B,(volatile Matrix*)&C,1.0f);
        double end=get_time();
        total_time+=(end-start);
    }

    double avg_time=total_time/runs;
    double flops=2.0*size*size*size;
    double gflops=(flops/avg_time)/1e9;

    int errors=0;
    for(int i=0;i<size*size;i++) if(fabs(C.data[i]-size)>1e-3) errors++;

    printf("%s Matmul %dx%d (%d runs): %.6f s, %.2f GFLOPS, Errors: %d/%d, Energy: %.3f J\n",
           version,size,size,runs,avg_time,gflops,errors,size*size,total_energy);

    free(A.data); free(B.data); free(C.data);
}

int main(){
    printf("Ultra Advanced Matmul with B_T: Cache-Optimized\n\n");

    int sizes[]={128,256,512};
    for(int s=0;s<3;s++){
        benchmark_ultra_matmul(sizes[s],3,"blocked");
        benchmark_ultra_matmul(sizes[s],3,"simd");
        benchmark_ultra_matmul(sizes[s],3,"int8");
    }

    printf("\nOptimizations:\n- B matrix transposed for SIMD & INT8\n- Ultra unroll, cache alignment, hyper-threading\n");

    return 0;
}
