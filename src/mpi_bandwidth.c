/* 
 * MPI bandwidth benchmark.
 * Ping-pong send/recv between ranks to stress interconnect bandwidth
 * and message latency under steady traffic.
 */

#include <mpi.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "bench_args.h"
#define SIZE (1024 * 1024 * 10) // 10MB Message
#define DEFAULT_MSG_SIZE SIZE
#define DEFAULT_ITERS 1000ULL

int main(int argc, char** argv) {
    double t0 = bench_now_sec();
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        MPI_Finalize();
        return 0;
    }

    size_t parsed_size = bench_parse_size(argc, argv, DEFAULT_MSG_SIZE);
    int msg_size = (parsed_size > (size_t)INT_MAX) ? INT_MAX : (int)parsed_size;
    char *buf = (char*)malloc((size_t)msg_size);

    double warmup = bench_parse_warmup(argc, argv, 0.0);
    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    int use_duration = bench_has_arg(argc, argv, "--duration");
    double duration = use_duration ? bench_parse_duration(argc, argv, 60.0) : 0.0;
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);

    if (warmup_iters > 0ULL) {
        MPI_Barrier(MPI_COMM_WORLD);
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            if (rank % 2 == 0) {
                if (rank + 1 < size) {
                    MPI_Send(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
                    MPI_Recv(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            } else {
                MPI_Recv(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
            }
        }
    } else if (warmup > 0.0) {
        MPI_Barrier(MPI_COMM_WORLD);
        double warm_start = MPI_Wtime();
        while (MPI_Wtime() - warm_start < warmup) {
            if (rank % 2 == 0) {
                if (rank + 1 < size) {
                    MPI_Send(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
                    MPI_Recv(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            } else {
                MPI_Recv(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();
    if (rank == 0) {
        fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    }
    if (use_duration) {
        while (MPI_Wtime() - start_time < duration) {
            if (rank % 2 == 0) {
                if (rank + 1 < size) {
                    MPI_Send(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
                    MPI_Recv(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            } else {
                MPI_Recv(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        for (unsigned long long iter = 0; iter < iterations; iter++) {
            if (rank % 2 == 0) {
                if (rank + 1 < size) {
                    MPI_Send(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
                    MPI_Recv(buf, msg_size, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            } else {
                MPI_Recv(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, msg_size, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
            }
        }
    }
    if (rank == 0) {
        fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    }
    
    free(buf);
    MPI_Finalize();
    return 0;
}
