#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

// Macro to generate massive code volume without loops (linear execution)
#define OP a^=b; b+=a; a|=b; b^=a;
#define OP10 OP OP OP OP OP OP OP OP OP OP 
#define OP100 OP10 OP10 OP10 OP10 OP10 OP10 OP10 OP10 OP10 OP10
#define OP1000 OP100 OP100 OP100 OP100 OP100 OP100 OP100 OP100 OP100 OP100

int main(int argc, char **argv) {
    volatile int a = 1, b = 2;
    double start = bench_now_sec();
    double duration = bench_parse_duration(argc, argv, 60.0);

    while ((bench_now_sec() - start) < duration) {
        // This block expands to thousands of instructions.
        // If this loop body > 32KB, it thrashes L1i.
        OP1000 
        OP1000
        OP1000
        OP1000
    }
    printf("%d\n", a);
    return 0;
}
