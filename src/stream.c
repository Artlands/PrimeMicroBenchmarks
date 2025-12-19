/* 
 * STREAM triad benchmark.
 * Large arrays with the triad kernel to drive sustained DRAM bandwidth
 * and saturate the memory controllers.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"
// Array size (adjust based on system memory, keep it larger than L3 cache)
#define N 20000000  // 20 million elements
#define DEFAULT_ITERS 200ULL

double a[N], b[N], c[N];
double scale = 3.0;

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    // Initialize arrays
    for (int i = 0; i < N; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
        c[i] = 3.0;
    }

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            for (int i = 0; i < N; i++) {
                a[i] = b[i] + scale * c[i];
            }
        }
    }
    double start_time = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < N; i++) {
            a[i] = b[i] + scale * c[i];
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);

    double seconds = bench_now_sec() - start_time;
    printf("Loop iterations: %llu\n", iterations);
    printf("Loop time: %f seconds\n", seconds);

    return 0;
}
