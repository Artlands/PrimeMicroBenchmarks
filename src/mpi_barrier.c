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
    // Run for a fixed workload by default
    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, 100000ULL);
    if (warmup_iters > 0ULL) {
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            MPI_Barrier(MPI_COMM_WORLD);
        }
    } else if (warmup > 0.0) {
        double warm_start = MPI_Wtime();
        while (MPI_Wtime() - warm_start < warmup) {
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    double start_time = MPI_Wtime();
    if (rank == 0) {
        fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    }
    if (use_duration) {
        do {
            MPI_Barrier(MPI_COMM_WORLD);  // Force synchronization
        } while ((MPI_Wtime() - start_time) < duration);
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    if (rank == 0) {
        fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    }

    MPI_Finalize();
    return 0;
}
