/* 
 * Atomic increment contention benchmark.
 * Multiple threads increment a single shared counter to force cache-line ping-pong
 * and highlight coherence traffic under heavy atomic serialization.
 */

#include <omp.h>
#include <stdio.h>
#include "bench_args.h"
int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    long shared_counter = 0;
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, 10000000ULL);

    if (warmup_iters > 0ULL) {
        #pragma omp parallel
        {
            for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
                #pragma omp atomic
                shared_counter++;
            }
        }
        shared_counter = 0;
    } else if (warmup > 0.0) {
        double warm_start = omp_get_wtime();
        #pragma omp parallel
        {
            while (omp_get_wtime() - warm_start < warmup) {
                #pragma omp atomic
                shared_counter++;
            }
        }
        shared_counter = 0;
    }

    double start = omp_get_wtime();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);

    #pragma omp parallel
    {
        if (use_duration) {
            while (omp_get_wtime() - start < duration) {
                // Force atomic contention
                // Threads fight for exclusive access to the cache line containing 'shared_counter'
                #pragma omp atomic
                shared_counter++;
            }
        } else {
            for (unsigned long long iter = 0; iter < iterations; iter++) {
                #pragma omp atomic
                shared_counter++;
            }
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    printf("Final Count: %ld\n", shared_counter);
    return 0;
}
