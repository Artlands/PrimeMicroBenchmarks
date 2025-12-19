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

    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0; // seconds
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            for (int i = 0; i < N; i++) {
                a[i] = b[i] + scale * c[i];
            }
        }
    } else if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
            for (int i = 0; i < N; i++) {
                a[i] = b[i] + scale * c[i];
            }
        }
    }
    double start_time = bench_now_sec();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    if (use_duration) {
        do {
            for (int i = 0; i < N; i++) {
                a[i] = b[i] + scale * c[i];  // The STREAM triad
            }
        } while ((bench_now_sec() - start_time) < duration);
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            for (int i = 0; i < N; i++) {
                a[i] = b[i] + scale * c[i];
            }
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);

    double seconds = bench_now_sec() - start_time;
    printf("STREAM Triad took %f seconds\n", seconds);

    return 0;
}
