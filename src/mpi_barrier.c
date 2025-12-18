#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "bench_args.h"

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Run for a fixed duration
    double duration = bench_parse_duration(argc, argv, 60.0);
    double start_time = MPI_Wtime();
    do {
        MPI_Barrier(MPI_COMM_WORLD);  // Force synchronization
    } while ((MPI_Wtime() - start_time) < duration);

    MPI_Finalize();
    return 0;
}
