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
    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    double start_time = bench_now_sec();

    // Pointer chasing loop
    int current_index = perm[0];
    volatile int sink = 0;
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            current_index = next[current_index];
            sink = values[current_index];
        }
    } else if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        do {
            current_index = next[current_index];
            sink = values[current_index];
        } while ((bench_now_sec() - warm_start) < warmup);
    }

    start_time = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    if (use_duration) {
        do {
            current_index = next[current_index]; // Chase the pointer
            sink = values[current_index]; // Access the value (force load)
        } while ((bench_now_sec() - start_time) < duration);
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            current_index = next[current_index];
            sink = values[current_index];
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);

    double seconds = bench_now_sec() - start_time;
    printf("Pointer chasing took %f seconds (sink=%d)\n", seconds, sink);

    free(perm);
    free(values);
    free(next);

    return 0;
}
