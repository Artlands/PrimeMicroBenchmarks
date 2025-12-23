/* 
 * DGEMM (Double-precision General Matrix Multiply) benchmark.
 * BLAS dense matrix multiply to stress floating-point throughput and
 * compute-bound execution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <cblas.h> // Requires BLAS library (e.g., OpenBLAS, MKL)
#include "bench_args.h"
// Matrix dimensions (adjust for L3 cache size, e.g., 256MB / sizeof(double))
#define N 2048
#define DEFAULT_SEED 1u
#define DEFAULT_ITERS 250ULL

double A[N*N], B[N*N], C[N*N]; // Static allocation

void init_matrix(double *matrix, int n) {
    for (int i = 0; i < n*n; i++) {
        matrix[i] = (double)rand() / RAND_MAX; // Fill with random numbers
    }
}

int main(int argc, char **argv) {
    double t0 = bench_now_sec();

    BENCH_PRINTF("DGEMM start\n");

    // Seed the random number generator
    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));

    // Initialize matrices
    init_matrix(A, N);
    init_matrix(B, N);
    init_matrix(C, N);

    // DGEMM parameters
    double alpha = 1.0;
    double beta = 0.0;
    CBLAS_LAYOUT layout = CblasRowMajor;
    CBLAS_TRANSPOSE transA = CblasNoTrans;
    CBLAS_TRANSPOSE transB = CblasNoTrans;

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 20ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {

        BENCH_PRINTF("DGEMM warmup start\n");

        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            cblas_dgemm(layout, transA, transB, N, N, N, alpha, A, N, B, N, beta, C, N);
        }
    }

    double start_time = bench_now_sec();

    BENCH_PRINTF("DGEMM loop start\n");

    BENCH_EPRINTF("LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        cblas_dgemm(layout, transA, transB, N, N, N, alpha, A, N, B, N, beta, C, N);
    }
    BENCH_EPRINTF("LOOP_END_REL %f\n", bench_now_sec() - t0);

    double seconds = bench_now_sec() - start_time;
    BENCH_PRINTF("DGEMM complete\n");

    BENCH_PRINTF("Loop iterations: %llu\n", iterations);
    BENCH_PRINTF("Loop time: %f seconds\n", seconds);

    return 0;
}
