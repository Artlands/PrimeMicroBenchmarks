#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

// Array size (adjust based on system memory, keep it larger than L3 cache)
#define N 20000000  // 20 million elements

double a[N], b[N], c[N];
double scale = 3.0;

int main(int argc, char **argv) {
    // Initialize arrays
    for (int i = 0; i < N; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
        c[i] = 3.0;
    }

    // Start timing
    double start_time = bench_now_sec();
    double duration = bench_parse_duration(argc, argv, 60.0); // seconds
    do {
        for (int i = 0; i < N; i++) {
            a[i] = b[i] + scale * c[i];  // The STREAM triad
        }
    } while ((bench_now_sec() - start_time) < duration);

    double seconds = bench_now_sec() - start_time;
    printf("STREAM Triad took %f seconds\n", seconds);

    return 0;
}
