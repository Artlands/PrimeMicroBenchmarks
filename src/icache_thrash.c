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
    printf("I-cache thrash start\n");
    volatile int a = 1, b = 2;
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, 200000ULL);

    if (warmup_iters > 0ULL) {
        printf("I-cache thrash warmup start\n");
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            OP1000
            OP1000
            OP1000
            OP1000
        }
    }

    double start = bench_now_sec();
    printf("I-cache thrash loop start\n");

    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        // This block expands to thousands of instructions.
        // If this loop body > 32KB, it thrashes L1i.
        OP1000 
        OP1000
        OP1000
        OP1000
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    printf("%d\n", a);
    printf("I-cache thrash complete\n");
    printf("Loop iterations: %llu\n", iterations);
    printf("Loop time: %f seconds\n", bench_now_sec() - start);
    return 0;
}
