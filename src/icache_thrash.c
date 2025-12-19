/* 
 * Instruction cache thrash benchmark.
 * Executes a large unrolled instruction stream to overflow L1i and
 * stress frontend fetch and decode bandwidth.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"
// Macro to generate massive code volume without loops (linear execution)
#define OP a^=b; b+=a; a|=b; b^=a;
#define OP10 OP OP OP OP OP OP OP OP OP OP 
#define OP100 OP10 OP10 OP10 OP10 OP10 OP10 OP10 OP10 OP10 OP10
#define OP1000 OP100 OP100 OP100 OP100 OP100 OP100 OP100 OP100 OP100 OP100

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    volatile int a = 1, b = 2;
    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, 200000ULL);

    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            OP1000
            OP1000
            OP1000
            OP1000
        }
    } else if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
            OP1000
            OP1000
            OP1000
            OP1000
        }
    }

    double start = bench_now_sec();

    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    if (use_duration) {
        while ((bench_now_sec() - start) < duration) {
            // This block expands to thousands of instructions.
            // If this loop body > 32KB, it thrashes L1i.
            OP1000 
            OP1000
            OP1000
            OP1000
        }
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            OP1000
            OP1000
            OP1000
            OP1000
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    printf("%d\n", a);
    return 0;
}
