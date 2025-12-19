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
    printf("Atomic fight start\n");
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    long shared_counter = 0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, 10000000ULL);

    if (warmup_iters > 0ULL) {
        printf("Atomic fight warmup start\n");
        #pragma omp parallel
        {
            for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
                #pragma omp atomic
                shared_counter++;
            }
        }
        shared_counter = 0;
    }

    double start = omp_get_wtime();
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    printf("Atomic fight loop start\n");

    #pragma omp parallel
    {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            // Force atomic contention
            // Threads fight for exclusive access to the cache line containing 'shared_counter'
            #pragma omp atomic
            shared_counter++;
        }
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    double end = omp_get_wtime();
    printf("Final Count: %ld\n", shared_counter);
    printf("Atomic fight complete\n");
    printf("Loop iterations: %llu\n", (unsigned long long)shared_counter);
    printf("Loop time: %f seconds\n", end - start);
    return 0;
}
