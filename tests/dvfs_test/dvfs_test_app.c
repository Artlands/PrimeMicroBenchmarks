#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CYCLES 6
#define DEFAULT_WORK_MB 128
#define DEFAULT_MSG_MB 2

static void *xmalloc(size_t bytes, int rank) {
    void *p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Rank %d failed allocation: %zu bytes\n", rank, bytes);
        MPI_Abort(MPI_COMM_WORLD, 99);
    }
    return p;
}

static double heavy_compute(unsigned long long flops_hint, int rank) {
    double a = 1.0 + rank;
    double b = 1.00001;
    double c = 0.99999;
    double sum = 0.0;

    for (unsigned long long i = 0; i < flops_hint; ++i) {
        a = (a * b + c) * 1.00000001;
        c = (c * b + a) * 0.99999999;
        sum += a * c;
        if ((i & 4095ULL) == 0ULL) {
            sum += sqrt(fabs(sum) + 1.0);
        }
    }
    return sum;
}

static double memory_pressure(double *buf, size_t n, int repeats) {
    double acc = 0.0;

    for (int r = 0; r < repeats; ++r) {
        for (size_t i = 0; i < n; i += 16) {
            buf[i] = buf[i] * 1.0001 + (double)r;
        }
        for (size_t i = 8; i < n; i += 16) {
            acc += buf[i] * 0.000000001;
        }
    }
    return acc;
}

static double collective_phase(double *msg, int count, int rounds) {
    double acc = 0.0;
    for (int r = 0; r < rounds; ++r) {
        for (int i = 0; i < count; ++i) {
            msg[i] = (double)(i + r);
        }
        MPI_Allreduce(MPI_IN_PLACE, msg, count, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        acc += msg[r % count] * 1e-12;
    }
    return acc;
}

static void jitter_phase(int rank, int cycles) {
    for (int i = 0; i < cycles; ++i) {
        if ((rank + i) % 3 == 0) {
            for (volatile int k = 0; k < 40000; ++k) {
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

static int get_arg(int argc, char **argv, const char *name, int default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            return atoi(argv[i + 1]);
        }
    }
    return default_value;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr, "Run with at least 2 MPI ranks.\n");
        }
        MPI_Finalize();
        return 1;
    }

    int cycles = get_arg(argc, argv, "--cycles", DEFAULT_CYCLES);
    int work_mb = get_arg(argc, argv, "--work-mb", DEFAULT_WORK_MB);
    int msg_mb = get_arg(argc, argv, "--msg-mb", DEFAULT_MSG_MB);

    size_t work_elems = ((size_t)work_mb * 1024ULL * 1024ULL) / sizeof(double);
    int msg_elems = (int)(((size_t)msg_mb * 1024ULL * 1024ULL) / sizeof(double));

    double *work = (double *)xmalloc(work_elems * sizeof(double), rank);
    double *msg = (double *)xmalloc((size_t)msg_elems * sizeof(double), rank);

    for (size_t i = 0; i < work_elems; ++i) {
        work[i] = 1.0 + (double)(i % 257) * 0.01;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();
    double checksum = 0.0;

    if (rank == 0) {
        printf("dvfs_test_app start ranks=%d cycles=%d work_mb=%d msg_mb=%d\n", size, cycles, work_mb, msg_mb);
    }

    for (int c = 0; c < cycles; ++c) {
        if (rank == 0) {
            printf("PHASE_START cycle=%d name=compute\n", c);
        }
        checksum += heavy_compute(16000000ULL, rank);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            printf("PHASE_END cycle=%d name=compute t=%.6f\n", c, MPI_Wtime() - start);
            printf("PHASE_START cycle=%d name=memory\n", c);
        }

        checksum += memory_pressure(work, work_elems, 28);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            printf("PHASE_END cycle=%d name=memory t=%.6f\n", c, MPI_Wtime() - start);
            printf("PHASE_START cycle=%d name=collective\n", c);
        }

        checksum += collective_phase(msg, msg_elems, 100);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            printf("PHASE_END cycle=%d name=collective t=%.6f\n", c, MPI_Wtime() - start);
            printf("PHASE_START cycle=%d name=jitter_sync\n", c);
        }

        jitter_phase(rank, 2400);
        if (rank == 0) {
            printf("PHASE_END cycle=%d name=jitter_sync t=%.6f\n", c, MPI_Wtime() - start);
        }
    }

    double final = 0.0;
    MPI_Reduce(&checksum, &final, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        printf("dvfs_test_app done total_time=%.6f checksum=%0.12e\n", MPI_Wtime() - start, final);
    }

    free(work);
    free(msg);
    MPI_Finalize();
    return 0;
}
