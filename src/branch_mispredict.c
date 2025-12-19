/* 
 * Branch misprediction benchmark.
 * Uses a random data-dependent branch with ~50% taken rate to stress
 * branch predictor accuracy and highlight misprediction penalties.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"
#define N 10000000 // 10 Million elements
#define DEFAULT_SEED 1u
#define DEFAULT_ITERS 200ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();

    printf("Branch mispredict start\n");

    // 1. Setup Data
    // Use short to keep cache pressure lower than memory benchmarks, 
    // focusing bottleneck on the Branch Unit.
    short *data = (short*)malloc(N * sizeof(short));

    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    for (int i = 0; i < N; i++) {
        data[i] = rand() % 256; // Random values 0-255
    }

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    long long sum = 0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {

        printf("Branch mispredict warmup start\n");

        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            for (int i = 0; i < N; i++) {
                if (data[i] >= 128) {
                    sum += data[i];
                }
            }
        }
        sum = 0;
    }
    double start = bench_now_sec();

    printf("Branch mispredict loop start\n");

    // 2. The Loop
    // The condition (data[i] >= 128) is true 50% of the time, randomly.
    // This is the mathematical worst-case for a branch predictor.
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < N; i++) {
            if (data[i] >= 128) {
                sum += data[i];
            }
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);

    printf("Sum: %lld\n", sum);
    printf("Branch mispredict complete\n");

    printf("Loop iterations: %llu\n", iterations);
    printf("Loop time: %f seconds\n", bench_now_sec() - start);
    
    free(data);
    return 0;
}
