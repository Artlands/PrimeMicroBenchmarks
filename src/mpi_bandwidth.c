#include <mpi.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "bench_args.h"

#define SIZE (1024 * 1024 * 10) // 10MB Message
#define DEFAULT_MSG_SIZE SIZE

int main(int argc, char** argv) {
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

    double duration = bench_parse_duration(argc, argv, 60.0);
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();
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
    
    free(buf);
    MPI_Finalize();
    return 0;
}
