#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

#define N 10000000 // 10 Million elements
#define DEFAULT_SEED 1u

int main(int argc, char **argv) {
    // 1. Setup Data
    // Use short to keep cache pressure lower than memory benchmarks, 
    // focusing bottleneck on the Branch Unit.
    short *data = (short*)malloc(N * sizeof(short));
    
    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    for (int i = 0; i < N; i++) {
        data[i] = rand() % 256; // Random values 0-255
    }

    long long sum = 0;
    double start = bench_now_sec();
    double duration = bench_parse_duration(argc, argv, 60.0);
    
    // 2. The Loop
    // The condition (data[i] >= 128) is true 50% of the time, randomly.
    // This is the mathematical worst-case for a branch predictor.
    while ((bench_now_sec() - start) < duration) {
        for (int i = 0; i < N; i++) {
            if (data[i] >= 128) {
                sum += data[i];
            }
        }
    }
    
    printf("Sum: %lld\n", sum);
    free(data);
    return 0;
}
