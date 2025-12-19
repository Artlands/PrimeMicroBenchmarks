#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

#define N 1000000  // Number of elements
#define DEFAULT_SEED 1u

int main(int argc, char **argv) {
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
    double start_time = bench_now_sec();

    // Pointer chasing loop
    int current_index = perm[0];
    volatile int sink = 0;
    double duration = bench_parse_duration(argc, argv, 60.0);
    do {
        current_index = next[current_index]; // Chase the pointer
        sink = values[current_index]; // Access the value (force load)
    } while ((bench_now_sec() - start_time) < duration);

    double seconds = bench_now_sec() - start_time;
    printf("Pointer chasing took %f seconds (sink=%d)\n", seconds, sink);

    free(perm);
    free(values);
    free(next);

    return 0;
}
