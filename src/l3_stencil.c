#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

// Size: 2MB (Large enough to bust L2, small enough to fit in L3)
// Adjust this: typical L2 is 256KB-1MB, L3 is 10MB-64MB.
#define N (2 * 1024 * 1024 / sizeof(double)) 
#define DEFAULT_ITERS 2000ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    double *A = (double*)malloc(N * sizeof(double));
    double *B = (double*)malloc(N * sizeof(double));
    
    // Initialize
    for(int i=0; i<N; i++) { A[i] = 1.0; B[i] = 0.5; }

    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);

    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            for (int i = 1; i < N - 1; i++) {
                A[i] = (B[i-1] + B[i] + B[i+1]) * 0.33;
            }
            if (A[N/2] > 1000) break;
        }
    } else if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
            for (int i = 1; i < N - 1; i++) {
                A[i] = (B[i-1] + B[i] + B[i+1]) * 0.33;
            }
            if (A[N/2] > 1000) break;
        }
    }
    double start = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    
    // Stencil-like 3-point average (Read 2, Write 1, Spatial Locality)
    if (use_duration) {
        while ((bench_now_sec() - start) < duration) {
            for (int i = 1; i < N - 1; i++) {
                A[i] = (B[i-1] + B[i] + B[i+1]) * 0.33;
            }
            if (A[N/2] > 1000) break; 
        }
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            for (int i = 1; i < N - 1; i++) {
                A[i] = (B[i-1] + B[i] + B[i+1]) * 0.33;
            }
            if (A[N/2] > 1000) break;
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    free(A);
    free(B);
    return 0;
}
