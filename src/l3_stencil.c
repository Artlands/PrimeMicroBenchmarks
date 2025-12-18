#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

// Size: 2MB (Large enough to bust L2, small enough to fit in L3)
// Adjust this: typical L2 is 256KB-1MB, L3 is 10MB-64MB.
#define N (2 * 1024 * 1024 / sizeof(double)) 

int main(int argc, char **argv) {
    double *A = (double*)malloc(N * sizeof(double));
    double *B = (double*)malloc(N * sizeof(double));
    
    // Initialize
    for(int i=0; i<N; i++) { A[i] = 1.0; B[i] = 0.5; }

    double duration = bench_parse_duration(argc, argv, 60.0);
    double start = bench_now_sec();
    
    // Stencil-like 3-point average (Read 2, Write 1, Spatial Locality)
    while ((bench_now_sec() - start) < duration) {
        for (int i = 1; i < N - 1; i++) {
            A[i] = (B[i-1] + B[i] + B[i+1]) * 0.33;
        }
        // Prevent compiler optimizing the loop away by using the result
        if (A[N/2] > 1000) break; 
    }
    free(A);
    free(B);
    return 0;
}
