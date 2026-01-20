/*
 * mpi_pattern_app.c
 * MPI proxy to exercise DVFS controller with clear phases.
 * Build with -DTARGET_DEFAULT="energy" or "edp".
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef TARGET_DEFAULT
#define TARGET_DEFAULT "energy"
#endif

typedef struct node {
    struct node *next;
    int pad[15];
} Node;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9;
}

static const char *expect_label(const char *target, const char *phase,
                                const char *expect_compute,
                                const char *expect_memory,
                                const char *expect_comm,
                                const char *expect_idle) {
    if (strcmp(phase, "COMPUTE") == 0) return expect_compute;
    if (strcmp(phase, "MEMORY") == 0) return expect_memory;
    if (strcmp(phase, "COMM") == 0) return expect_comm;
    if (strcmp(phase, "IDLE") == 0) return expect_idle;
    return target;
}

static void phase_compute(double *A, double *B, double *C, int n, double seconds) {
    double start = now_sec();
    while (now_sec() - start < seconds) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double sum = 0.0;
                for (int k = 0; k < n; k++) {
                    sum += A[i * n + k] * B[k * n + j];
                }
                C[i * n + j] = sum;
            }
        }
    }
}

static void phase_memory(Node *pool, long long steps) {
    Node *p = pool;
    for (long long i = 0; i < steps; i++) {
        p = p->next;
    }
    if (p == NULL) {
        printf("unreachable\n");
    }
}

static void phase_comm(double seconds, int comm_iters) {
    double start = now_sec();
    double send_buf = 1.0;
    double recv_buf = 0.0;
    while (now_sec() - start < seconds) {
        for (int i = 0; i < comm_iters; i++) {
            MPI_Allreduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

static void phase_idle(double seconds) {
    usleep((useconds_t)(seconds * 1000000.0));
}

int main(int argc, char **argv) {
    int rank, size;
    int iterations = 3;
    int matrix_n = 256;
    long long ll_bytes = 64LL * 1024 * 1024;
    long long mem_steps = 200000000;
    double phase_sec = 2.0;
    int comm_iters = 1000;
    const char *target = TARGET_DEFAULT;
    const char *expect_compute = "HighFreq";
    const char *expect_memory = "MedFreq";
    const char *expect_comm = "LowFreq";
    const char *expect_idle = "LowFreq";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--matrix-n") == 0 && i + 1 < argc) {
            matrix_n = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ll-bytes") == 0 && i + 1 < argc) {
            ll_bytes = atoll(argv[++i]);
        } else if (strcmp(argv[i], "--mem-steps") == 0 && i + 1 < argc) {
            mem_steps = atoll(argv[++i]);
        } else if (strcmp(argv[i], "--phase-sec") == 0 && i + 1 < argc) {
            phase_sec = atof(argv[++i]);
        } else if (strcmp(argv[i], "--comm-iters") == 0 && i + 1 < argc) {
            comm_iters = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            target = argv[++i];
        } else if (strcmp(argv[i], "--expect-compute") == 0 && i + 1 < argc) {
            expect_compute = argv[++i];
        } else if (strcmp(argv[i], "--expect-memory") == 0 && i + 1 < argc) {
            expect_memory = argv[++i];
        } else if (strcmp(argv[i], "--expect-comm") == 0 && i + 1 < argc) {
            expect_comm = argv[++i];
        } else if (strcmp(argv[i], "--expect-idle") == 0 && i + 1 < argc) {
            expect_idle = argv[++i];
        }
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    setvbuf(stdout, NULL, _IONBF, 0);

    if (rank == 0) {
        printf("[MPI] size=%d target=%s iterations=%d phase_sec=%.2f\n",
               size, target, iterations, phase_sec);
    }

    double *A = malloc((size_t)matrix_n * matrix_n * sizeof(double));
    double *B = malloc((size_t)matrix_n * matrix_n * sizeof(double));
    double *C = malloc((size_t)matrix_n * matrix_n * sizeof(double));
    if (!A || !B || !C) {
        if (rank == 0) {
            fprintf(stderr, "Failed to allocate matrices\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (int i = 0; i < matrix_n * matrix_n; i++) {
        A[i] = 1.0;
        B[i] = 0.5;
        C[i] = 0.0;
    }

    int num_nodes = (int)(ll_bytes / (long long)sizeof(Node));
    if (num_nodes < 2) {
        num_nodes = 2;
    }
    Node *pool = malloc((size_t)num_nodes * sizeof(Node));
    if (!pool) {
        if (rank == 0) {
            fprintf(stderr, "Failed to allocate linked list pool\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (int i = 0; i < num_nodes - 1; i++) {
        pool[i].next = &pool[i + 1];
    }
    pool[num_nodes - 1].next = &pool[0];
    srand(42);
    for (int i = 0; i < num_nodes; i++) {
        int swap_idx = rand() % num_nodes;
        Node *tmp = pool[i].next;
        pool[i].next = pool[swap_idx].next;
        pool[swap_idx].next = tmp;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (int iter = 0; iter < iterations; iter++) {
        if (rank == 0) {
            printf("\n[APP] ITERATION %d\n", iter);
            printf("[APP] PHASE COMPUTE expected=%s\n",
                   expect_label(target, "COMPUTE", expect_compute, expect_memory, expect_comm, expect_idle));
        }
        phase_compute(A, B, C, matrix_n, phase_sec);
        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 0) {
            printf("[APP] PHASE MEMORY expected=%s\n",
                   expect_label(target, "MEMORY", expect_compute, expect_memory, expect_comm, expect_idle));
        }
        phase_memory(pool, mem_steps);
        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 0) {
            printf("[APP] PHASE COMM expected=%s\n",
                   expect_label(target, "COMM", expect_compute, expect_memory, expect_comm, expect_idle));
        }
        phase_comm(phase_sec, comm_iters);
        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 0) {
            printf("[APP] PHASE IDLE expected=%s\n",
                   expect_label(target, "IDLE", expect_compute, expect_memory, expect_comm, expect_idle));
        }
        phase_idle(phase_sec);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    free(A);
    free(B);
    free(C);
    free(pool);
    MPI_Finalize();
    return 0;
}
