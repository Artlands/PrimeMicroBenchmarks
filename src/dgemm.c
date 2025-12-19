#include <stdio.h>
#include <stdlib.h>
#include <cblas.h> // Requires BLAS library (e.g., OpenBLAS, MKL)
#include "bench_args.h"

// Matrix dimensions (adjust for L3 cache size, e.g., 256MB / sizeof(double))
#define N 2048
#define DEFAULT_SEED 1u

double A[N*N], B[N*N], C[N*N]; // Static allocation

void init_matrix(double *matrix, int n) {
    for (int i = 0; i < n*n; i++) {
        matrix[i] = (double)rand() / RAND_MAX; // Fill with random numbers
    }
}

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
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

    double warmup = bench_parse_warmup(argc, argv, 0.0);
    // Start timing
    double duration = bench_parse_duration(argc, argv, 60.0); // seconds
    if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
            cblas_dgemm(layout, transA, transB, N, N, N, alpha, A, N, B, N, beta, C, N);
        }
    }
    double start_time = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    do {
        // DGEMM call (C = alpha*A*B + beta*C)
        cblas_dgemm(layout, transA, transB, N, N, N, alpha, A, N, B, N, beta, C, N);
    } while ((bench_now_sec() - start_time) < duration);
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);

    double seconds = bench_now_sec() - start_time;
    printf("DGEMM took %f seconds\n", seconds);

    return 0;
}
