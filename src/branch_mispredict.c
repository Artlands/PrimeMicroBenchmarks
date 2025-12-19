#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

#define N 10000000 // 10 Million elements
#define DEFAULT_SEED 1u
#define DEFAULT_ITERS 200ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    // 1. Setup Data
    // Use short to keep cache pressure lower than memory benchmarks, 
    // focusing bottleneck on the Branch Unit.
    short *data = (short*)malloc(N * sizeof(short));
    
    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    for (int i = 0; i < N; i++) {
        data[i] = rand() % 256; // Random values 0-255
    }

    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    long long sum = 0;
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            for (int i = 0; i < N; i++) {
                if (data[i] >= 128) {
                    sum += data[i];
                }
            }
        }
        sum = 0;
    } else if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
            for (int i = 0; i < N; i++) {
                if (data[i] >= 128) {
                    sum += data[i];
                }
            }
        }
        sum = 0;
    }
    double start = bench_now_sec();
    
    // 2. The Loop
    // The condition (data[i] >= 128) is true 50% of the time, randomly.
    // This is the mathematical worst-case for a branch predictor.
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    if (use_duration) {
        while ((bench_now_sec() - start) < duration) {
            for (int i = 0; i < N; i++) {
                if (data[i] >= 128) {
                    sum += data[i];
                }
            }
        }
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            for (int i = 0; i < N; i++) {
                if (data[i] >= 128) {
                    sum += data[i];
                }
            }
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    
    printf("Sum: %lld\n", sum);
    free(data);
    return 0;
}
