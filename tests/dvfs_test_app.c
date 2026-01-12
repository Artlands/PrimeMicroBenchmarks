/**
 * dvfs_test_app.c
 * A synthetic proxy application to validate DVFS Controllers.
 * 
 * Phases:
 * 1. COMPUTE (High Freq): Dense Matrix Multiplication (DGEMM)
 * 2. MEMORY  (Med Freq):  Random Pointer Chasing (Latency Bound)
 * 3. COMM    (Low Freq):  MPI_Allreduce with small messages (Spin/Sync Bound)
 * 4. SLEEP   (Min Freq):  OS Sleep (Idle)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <cblas.h>
#include <time.h>

// Configuration
#define MATRIX_SIZE 2048    // Enough to fit in L3 but take time
// Linked list size in bytes; converted to node count at runtime.
#define LL_BYTES    (64UL * 1024 * 1024) // 64MB Linked List (exceeds L3)
#define COMM_ITERS  100000  // Number of small MPI messages
#define SLEEP_SEC   5       // Duration of sleep phase

// Linked List Node for Memory Phase
typedef struct node {
    struct node *next;
    int pad[15]; // Pad to 64 bytes (Cache Line Size)
} Node;

// --- Phase 1: Compute (DGEMM) ---
// Expected: High IPC, High FLOPS -> HIGH FREQUENCY
void phase_compute(int rank, double *A, double *B, double *C, int n) {
    if (rank == 0) printf("[APP] >>> Entering COMPUTE Phase (DGEMM)...\n");
    
    // Perform C = alpha*A*B + beta*C
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                n, n, n, 1.0, A, n, B, n, 0.0, C, n);
                
    if (rank == 0) printf("[APP] <<< Finished COMPUTE Phase.\n");
}

// --- Phase 2: Memory (Pointer Chasing) ---
// Expected: Low IPC, High Stalls -> MED/LOW FREQUENCY
void phase_memory(int rank, Node *pool) {
    if (rank == 0) printf("[APP] >>> Entering MEMORY Phase (Latency)...\n");
    
    Node *p = pool;
    // Traverse for roughly 5 seconds worth of work
    // 5e8 accesses * ~10ns latency = ~5s
    long long steps = 400000000; 
    
    // The "volatile" prevents compiler from optimizing the loop away
    // logic: ensure p updates based on memory fetch
    for (long long i = 0; i < steps; i++) {
        p = p->next; 
    }
    
    // Prevent dead code elimination
    if (p == NULL && rank == 0) printf("You won't see this.\n");
    if (rank == 0) printf("[APP] <<< Finished MEMORY Phase.\n");
}

// --- Phase 3: Communication (MPI Sync) ---
// Expected: High IPC (Spinning), Zero FLOPS -> LOW FREQUENCY
void phase_comm(int rank) {
    if (rank == 0) printf("[APP] >>> Entering COMM Phase (Allreduce)...\n");
    
    double send_buf = 1.0;
    double recv_buf = 0.0;
    
    for (int i = 0; i < COMM_ITERS; i++) {
        MPI_Allreduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        // Small barrier to ensure we aren't just batching everything
        if (i % 100 == 0) MPI_Barrier(MPI_COMM_WORLD);
    }
    
    if (rank == 0) printf("[APP] <<< Finished COMM Phase.\n");
}

// --- Phase 4: Sleep (Idle) ---
// Expected: Zero IPC -> LOWEST FREQUENCY
void phase_sleep(int rank) {
    if (rank == 0) printf("[APP] >>> Entering SLEEP Phase (Idle)...\n");
    sleep(SLEEP_SEC);
    if (rank == 0) printf("[APP] <<< Finished SLEEP Phase.\n");
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    setvbuf(stdout, NULL, _IONBF, 0);
    if (rank == 0) {
        printf("[APP] MPI initialized. size=%d\n", size);
    }

    // --- Setup for Compute Phase ---
    double *A = malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
    double *B = malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
    double *C = malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
    // Initialize
    for (int i=0; i<MATRIX_SIZE*MATRIX_SIZE; i++) { 
        A[i] = 1.0; B[i] = 0.5; C[i] = 0.0; 
    }

    // --- Setup for Memory Phase ---
    int num_nodes = (int)(LL_BYTES / sizeof(Node));
    Node *pool = malloc((size_t)num_nodes * sizeof(Node));
    // Initialize linear linking
    for (int i=0; i < num_nodes - 1; i++) {
        pool[i].next = &pool[i+1];
    }
    pool[num_nodes-1].next = &pool[0]; // Circular
    // Shuffle to create random memory access pattern (Latency bound)
    srand(42);
    for (int i = 0; i < num_nodes; i++) {
        int swap_idx = rand() % num_nodes;
        Node *temp = pool[i].next;
        pool[i].next = pool[swap_idx].next;
        pool[swap_idx].next = temp;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // --- Main Test Loop ---
    int iteration = 0;
    while (1) {
        if (rank == 0) printf("\n=== ITERATION %d ===\n", iteration++);
        
        phase_compute(rank, A, B, C, MATRIX_SIZE);
        MPI_Barrier(MPI_COMM_WORLD); // Sync before switching phases

        phase_memory(rank, pool);
        MPI_Barrier(MPI_COMM_WORLD);

        phase_comm(rank);
        MPI_Barrier(MPI_COMM_WORLD);
        
        phase_sleep(rank);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Cleanup (Unreachable)
    free(A); free(B); free(C); free(pool);
    MPI_Finalize();
    return 0;
}
