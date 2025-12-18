#include <omp.h>
#include <stdio.h>
#include "bench_args.h"

int main(int argc, char **argv) {
    long shared_counter = 0;
    double start = omp_get_wtime();
    double duration = bench_parse_duration(argc, argv, 60.0);

    #pragma omp parallel
    {
        while (omp_get_wtime() - start < duration) {
            // Force atomic contention
            // Threads fight for exclusive access to the cache line containing 'shared_counter'
            #pragma omp atomic
            shared_counter++;
        }
    }
    printf("Final Count: %ld\n", shared_counter);
    return 0;
}
