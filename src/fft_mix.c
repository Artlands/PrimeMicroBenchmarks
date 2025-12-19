/* 
 * FFT mix benchmark.
 * Runs a recursive FFT on fixed-size arrays to mix compute intensity with
 * cache behavior and twiddle-factor access patterns.
 */

#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"
#define PI 3.14159265358979323846
#define N 16384 // Fit in L2/L3 boundary
#define DEFAULT_ITERS 2000ULL

void fft(double complex *buf, double complex *out, int n, int step) {
    if (step < n) {
        fft(out, buf, n, step * 2);
        fft(out + step, buf + step, n, step * 2);
        for (int i = 0; i < n; i += 2 * step) {
            double complex t = cexp(-I * PI * i / n) * out[i + step];
            buf[i / 2] = out[i] + t;
            buf[(i + n) / 2] = out[i] - t;
        }
    }
}

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    printf("FFT mix start\n");
    double complex *buf = malloc(N * sizeof(double complex));
    double complex *out = malloc(N * sizeof(double complex));

    for (int i = 0; i < N; i++) {
        double angle = 2.0 * PI * (double)i / (double)N;
        buf[i] = cos(angle) + I * sin(angle);
        out[i] = buf[i];
    }

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);

    if (warmup_iters > 0ULL) {
        printf("FFT mix warmup start\n");
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            fft(buf, out, N, 1);
        }
    }

    double start = bench_now_sec();
    printf("FFT mix loop start\n");
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        fft(buf, out, N, 1);
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    printf("FFT mix complete\n");
    printf("Loop iterations: %llu\n", iterations);
    printf("Loop time: %f seconds\n", bench_now_sec() - start);
    free(buf); free(out);
    return 0;
}
