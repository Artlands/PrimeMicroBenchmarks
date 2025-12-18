#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

#define N 1000000  // Rows
#define NZ_PER_ROW 10 // Non-zeros per row
#define DEFAULT_SEED 1u

int main(int argc, char **argv) {
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
    double duration = bench_parse_duration(argc, argv, 60.0);
    double start = bench_now_sec();
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
    
    printf("SpMV Complete\n");
    free(values);
    free(col_indices);
    free(row_ptr);
    free(x);
    free(y);
    return 0;
}
