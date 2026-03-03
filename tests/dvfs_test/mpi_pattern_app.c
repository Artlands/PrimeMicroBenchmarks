#include <mpi.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PHASE_REPEATS 4
#define DEFAULT_COMPUTE_ITERS 15000000ULL
#define DEFAULT_MEMORY_ITERS 12
#define DEFAULT_COMM_ITERS 1200
#define DEFAULT_SYNC_ITERS 8000
#define DEFAULT_MEM_MB 96
#define DEFAULT_MSG_KB 512

static void fill_array(double *a, size_t n, double seed) {
    for (size_t i = 0; i < n; ++i) {
        a[i] = seed + (double)(i % 97) * 0.001;
    }
}

static double run_compute_phase(unsigned long long iters, int rank) {
    double x = 1.0 + rank * 0.01;
    double y = 0.5 + rank * 0.02;
    double acc = 0.0;

    for (unsigned long long i = 0; i < iters; ++i) {
        x = x * 1.0000001192092896 + 0.00000001;
        y = y * 0.9999998807907104 + 0.00000002;
        acc += x * y + sqrt(x + 1.0);
        if ((i & 1023ULL) == 0ULL) {
            x += acc * 1e-15;
            y += acc * 1e-16;
        }
    }

    return acc;
}

static double run_memory_phase(double *a, double *b, size_t n, int passes) {
    const size_t stride = 8;
    double acc = 0.0;

    for (int p = 0; p < passes; ++p) {
        for (size_t i = 0; i < n; i += stride) {
            b[i] = b[i] * 1.0000001 + a[i] * 0.9999999 + (double)p;
            a[i] = a[i] + b[i] * 0.5;
            acc += a[i] * 1e-12;
        }
    }

    return acc;
}

static double run_comm_phase(double *buf, int count, int rounds, int rank, int size) {
    int left = (rank - 1 + size) % size;
    int right = (rank + 1) % size;
    double acc = 0.0;

    for (int r = 0; r < rounds; ++r) {
        for (int i = 0; i < count; ++i) {
            buf[i] = rank + r * 0.001 + i * 1e-6;
        }

        MPI_Sendrecv_replace(buf, count, MPI_DOUBLE, right, 100, left, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < count; i += 32) {
            acc += buf[i] * 1e-9;
        }
    }

    return acc;
}

static void run_sync_phase(int rounds) {
    for (int i = 0; i < rounds; ++i) {
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

static int get_arg_int(int argc, char **argv, const char *name, int default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            return atoi(argv[i + 1]);
        }
    }
    return default_value;
}

static unsigned long long get_arg_ull(int argc, char **argv, const char *name, unsigned long long default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (strcmp(argv[i], name) == 0) {
            return strtoull(argv[i + 1], NULL, 10);
        }
    }
    return default_value;
}

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [options]\n"
            "  --phase-repeats N      default %d\n"
            "  --compute-iters N      default %llu\n"
            "  --memory-passes N      default %d\n"
            "  --mem-mb N             default %d\n"
            "  --comm-rounds N        default %d\n"
            "  --msg-kb N             default %d\n"
            "  --sync-rounds N        default %d\n",
            prog,
            DEFAULT_PHASE_REPEATS,
            (unsigned long long)DEFAULT_COMPUTE_ITERS,
            DEFAULT_MEMORY_ITERS,
            DEFAULT_MEM_MB,
            DEFAULT_COMM_ITERS,
            DEFAULT_MSG_KB,
            DEFAULT_SYNC_ITERS);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr, "This benchmark expects at least 2 MPI ranks.\n");
        }
        MPI_Finalize();
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            if (rank == 0) {
                print_usage(argv[0]);
            }
            MPI_Finalize();
            return 0;
        }
    }

    int phase_repeats = get_arg_int(argc, argv, "--phase-repeats", DEFAULT_PHASE_REPEATS);
    unsigned long long compute_iters = get_arg_ull(argc, argv, "--compute-iters", DEFAULT_COMPUTE_ITERS);
    int memory_passes = get_arg_int(argc, argv, "--memory-passes", DEFAULT_MEMORY_ITERS);
    int mem_mb = get_arg_int(argc, argv, "--mem-mb", DEFAULT_MEM_MB);
    int comm_rounds = get_arg_int(argc, argv, "--comm-rounds", DEFAULT_COMM_ITERS);
    int msg_kb = get_arg_int(argc, argv, "--msg-kb", DEFAULT_MSG_KB);
    int sync_rounds = get_arg_int(argc, argv, "--sync-rounds", DEFAULT_SYNC_ITERS);

    size_t mem_bytes = (size_t)mem_mb * 1024ULL * 1024ULL;
    size_t mem_elems = mem_bytes / sizeof(double);
    int msg_elems = (msg_kb * 1024) / (int)sizeof(double);

    double *a = (double *)malloc(mem_elems * sizeof(double));
    double *b = (double *)malloc(mem_elems * sizeof(double));
    double *msg = (double *)malloc((size_t)msg_elems * sizeof(double));

    if (!a || !b || !msg) {
        fprintf(stderr, "Rank %d failed to allocate buffers.\n", rank);
        free(a);
        free(b);
        free(msg);
        MPI_Finalize();
        return 2;
    }

    fill_array(a, mem_elems, 1.0 + rank);
    fill_array(b, mem_elems, 2.0 + rank);

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    if (rank == 0) {
        printf("mpi_pattern_app start ranks=%d phase_repeats=%d\n", size, phase_repeats);
        printf("config: compute_iters=%llu memory_passes=%d mem_mb=%d comm_rounds=%d msg_kb=%d sync_rounds=%d\n",
               compute_iters, memory_passes, mem_mb, comm_rounds, msg_kb, sync_rounds);
    }

    double sink = 0.0;
    for (int p = 0; p < phase_repeats; ++p) {
        double ts = MPI_Wtime();
        if (rank == 0) {
            printf("PHASE_START cycle=%d type=compute t=%.6f\n", p, ts - t0);
        }
        sink += run_compute_phase(compute_iters, rank);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            printf("PHASE_END cycle=%d type=compute t=%.6f\n", p, MPI_Wtime() - t0);
        }

        ts = MPI_Wtime();
        if (rank == 0) {
            printf("PHASE_START cycle=%d type=memory t=%.6f\n", p, ts - t0);
        }
        sink += run_memory_phase(a, b, mem_elems, memory_passes);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            printf("PHASE_END cycle=%d type=memory t=%.6f\n", p, MPI_Wtime() - t0);
        }

        ts = MPI_Wtime();
        if (rank == 0) {
            printf("PHASE_START cycle=%d type=comm t=%.6f\n", p, ts - t0);
        }
        sink += run_comm_phase(msg, msg_elems, comm_rounds, rank, size);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            printf("PHASE_END cycle=%d type=comm t=%.6f\n", p, MPI_Wtime() - t0);
        }

        ts = MPI_Wtime();
        if (rank == 0) {
            printf("PHASE_START cycle=%d type=sync t=%.6f\n", p, ts - t0);
        }
        run_sync_phase(sync_rounds);
        if (rank == 0) {
            printf("PHASE_END cycle=%d type=sync t=%.6f\n", p, MPI_Wtime() - t0);
        }
    }

    double local_sink = sink;
    double global_sink = 0.0;
    MPI_Reduce(&local_sink, &global_sink, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("mpi_pattern_app done total_time=%.6f checksum=%0.12e\n", MPI_Wtime() - t0, global_sink);
    }

    free(a);
    free(b);
    free(msg);
    MPI_Finalize();
    return 0;
}
