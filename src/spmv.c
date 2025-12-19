/* 
 * Sparse matrix-vector multiply benchmark.
 * CSR SpMV with random column indices to drive irregular memory access
 * and TLB pressure from indirect gathers.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"
#define N 1000000  // Rows
#define NZ_PER_ROW 10 // Non-zeros per row
#define DEFAULT_SEED 1u
#define DEFAULT_ITERS 100ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    // 1. Setup CSR Format (Compressed Sparse Row)
    double *values = (double*)malloc(N * NZ_PER_ROW * sizeof(double));
    int *col_indices = (int*)malloc(N * NZ_PER_ROW * sizeof(int));
    int *row_ptr = (int*)malloc((N + 1) * sizeof(int));
    double *x = (double*)malloc(N * sizeof(double));
    double *y = (double*)malloc(N * sizeof(double));

    // Initialize with random data causing cache thrashing
    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    row_ptr[0] = 0;
    for (int i = 0; i < N; i++) {
        row_ptr[i+1] = row_ptr[i] + NZ_PER_ROW;
        x[i] = 1.0;
        y[i] = 0.0;
        for (int j = 0; j < NZ_PER_ROW; j++) {
            int idx = i * NZ_PER_ROW + j;
            values[idx] = 1.0;
            // Random column index forces irregular memory access
            col_indices[idx] = rand() % N; 
        }
    }

    // 2. The Loop
    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            for (int i = 0; i < N; i++) {
                double sum = 0.0;
                for (int j = row_ptr[i]; j < row_ptr[i+1]; j++) {
                    sum += values[j] * x[col_indices[j]];
                }
                y[i] = sum;
            }
        }
    } else if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
            for (int i = 0; i < N; i++) {
                double sum = 0.0;
                for (int j = row_ptr[i]; j < row_ptr[i+1]; j++) {
                    sum += values[j] * x[col_indices[j]];
                }
                y[i] = sum;
            }
        }
    }
    double start = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    if (use_duration) {
        while ((bench_now_sec() - start) < duration) {
            // SpMV Kernel
            for (int i = 0; i < N; i++) {
                double sum = 0.0;
                for (int j = row_ptr[i]; j < row_ptr[i+1]; j++) {
                    // INDIRECT ACCESS: The bottleneck is fetching x[col_indices[j]]
                    sum += values[j] * x[col_indices[j]];
                }
                y[i] = sum;
            }
        }
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            for (int i = 0; i < N; i++) {
                double sum = 0.0;
                for (int j = row_ptr[i]; j < row_ptr[i+1]; j++) {
                    sum += values[j] * x[col_indices[j]];
                }
                y[i] = sum;
            }
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    
    printf("SpMV Complete\n");
    free(values);
    free(col_indices);
    free(row_ptr);
    free(x);
    free(y);
    return 0;
}
