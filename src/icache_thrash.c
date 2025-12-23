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

    BENCH_PRINTF("I-cache thrash start\n");

    volatile int a = 1, b = 2;
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 10000ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, 1000000ULL);

    if (warmup_iters > 0ULL) {

        BENCH_PRINTF("I-cache thrash warmup start\n");

        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            OP1000
            OP1000
            OP1000
            OP1000
        }
    }

    double start = bench_now_sec();

    BENCH_PRINTF("I-cache thrash loop start\n");

    BENCH_EPRINTF("LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        // This block expands to thousands of instructions.
        // If this loop body > 32KB, it thrashes L1i.
        OP1000 
        OP1000
        OP1000
        OP1000
    }
    BENCH_EPRINTF("LOOP_END_REL %f\n", bench_now_sec() - t0);

    BENCH_PRINTF("%d\n", a);
    BENCH_PRINTF("I-cache thrash complete\n");

    BENCH_PRINTF("Loop iterations: %llu\n", iterations);
    BENCH_PRINTF("Loop time: %f seconds\n", bench_now_sec() - start);
    
    return 0;
}
