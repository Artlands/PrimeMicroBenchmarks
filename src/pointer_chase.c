/* 
 * Pointer chase benchmark.
 * Follows a random single-cycle permutation to serialize loads and expose
 * memory latency from caches and DRAM.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"
#define N 1000000  // Number of elements
#define DEFAULT_SEED 1u
#define DEFAULT_ITERS 50000000ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    printf("Pointer chase start\n");
    int *next = (int*)malloc(N * sizeof(int));
    int *values = (int*)malloc(N * sizeof(int));
    int *perm = (int*)malloc(N * sizeof(int));

    // Create a single-cycle random permutation for deterministic pointer chasing.
    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    for (int i = 0; i < N; i++) {
        perm[i] = i;
        values[i] = i;
    }
    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }
    for (int i = 0; i < N - 1; i++) {
        next[perm[i]] = perm[i + 1];
    }
    next[perm[N - 1]] = perm[0];

    // Start timing
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    double start_time = bench_now_sec();

    // Pointer chasing loop
    int current_index = perm[0];
    volatile int sink = 0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {
        printf("Pointer chase warmup start\n");
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            current_index = next[current_index];
            sink = values[current_index];
        }
    }

    start_time = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    printf("Pointer chase loop start\n");
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        current_index = next[current_index];
        sink = values[current_index];
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);

    double seconds = bench_now_sec() - start_time;
    printf("Sink: %d\n", sink);
    printf("Pointer chase complete\n");
    printf("Loop iterations: %llu\n", iterations);
    printf("Loop time: %f seconds\n", seconds);

    free(perm);
    free(values);
    free(next);

    return 0;
}
