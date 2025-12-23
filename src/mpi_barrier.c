/* 
 * MPI barrier benchmark.
 * Tight MPI_Barrier loop to measure synchronization latency and
 * idle spinning across ranks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "bench_args.h"
int main(int argc, char *argv[]) {
    int rank, size;
    double t0 = bench_now_sec();
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0) {

        BENCH_PRINTF("MPI barrier start\n");

    }
    // Run for a fixed workload by default
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 3000ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, 30000000ULL);
    if (warmup_iters > 0ULL) {
        if (rank == 0) {

            BENCH_PRINTF("MPI barrier warmup start\n");

        }
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    double start_time = MPI_Wtime();
    if (rank == 0) {

        BENCH_PRINTF("MPI barrier loop start\n");

        BENCH_EPRINTF("LOOP_START_REL %f\n", bench_now_sec() - t0);
    }
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if (rank == 0) {
        BENCH_EPRINTF("LOOP_END_REL %f\n", bench_now_sec() - t0);

        BENCH_PRINTF("MPI barrier complete\n");

        BENCH_PRINTF("Loop iterations: %llu\n", iterations);
        BENCH_PRINTF("Loop time: %f seconds\n", MPI_Wtime() - start_time);
    }

    MPI_Finalize();
    return 0;
}
