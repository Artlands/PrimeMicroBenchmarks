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
    // Run for a fixed duration
    double warmup = bench_parse_warmup(argc, argv, 0.0);
    double duration = bench_parse_duration(argc, argv, 60.0);
    if (warmup > 0.0) {
        double warm_start = MPI_Wtime();
        while (MPI_Wtime() - warm_start < warmup) {
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    double start_time = MPI_Wtime();
    if (rank == 0) {
        fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    }
    do {
        MPI_Barrier(MPI_COMM_WORLD);  // Force synchronization
    } while ((MPI_Wtime() - start_time) < duration);
    if (rank == 0) {
        fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    }

    MPI_Finalize();
    return 0;
}
