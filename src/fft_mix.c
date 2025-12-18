#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

#define PI 3.14159265358979323846
#define N 16384 // Fit in L2/L3 boundary

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
    double complex *buf = malloc(N * sizeof(double complex));
    double complex *out = malloc(N * sizeof(double complex));

    for (int i = 0; i < N; i++) {
        double angle = 2.0 * PI * (double)i / (double)N;
        buf[i] = cos(angle) + I * sin(angle);
        out[i] = buf[i];
    }

    double duration = bench_parse_duration(argc, argv, 60.0);
    double start = bench_now_sec();
    while ((bench_now_sec() - start) < duration) {
        fft(buf, out, N, 1);
    }
    free(buf); free(out);
    return 0;
}
