# Compiler Settings
CC = gcc
MPICC = mpicc
CFLAGS = -O3 -march=native -Wall
OMP_FLAGS = -fopenmp
# Adjust LIBS if using Intel MKL (e.g., -lmkl_intel_lp64 -lmkl_sequential -lmkl_core)
BLAS_CFLAGS = $(shell pkg-config --cflags openblas)
BLAS_LIBS   = $(shell pkg-config --libs openblas)
LIBS = -lopenblas -lm

# Change this to your desired output directory
BIN_DIR = ../bin/amd

# Targets
all: compute memory latency idle

compute: dgemm branch_mispredict icache_thrash tree_walk fft_mix
memory: l3_stencil stream spmv
latency: pointer_chase atomic_fight mpi_bandwidth
idle: mpi_barrier io_write

# --- Compute & Frontend ---
dgemm: dgemm.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/dgemm dgemm.c $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

branch_mispredict: branch_mispredict.c | $(BIN_DIR)
	# Critical: Disable vectorization to keep the branch logic intact
	$(CC) $(CFLAGS) -fno-tree-vectorize -fno-if-conversion -o $(BIN_DIR)/branch_mispredict branch_mispredict.c

icache_thrash: icache_thrash.c | $(BIN_DIR)
	# Critical: -O0 is required to prevent code folding/loop removal
	$(CC) -O0 -o $(BIN_DIR)/icache_thrash icache_thrash.c

tree_walk: tree_walk.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/tree_walk tree_walk.c

fft_mix: fft_mix.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/fft_mix fft_mix.c -lm

# --- Memory & Data ---
l3_stencil: l3_stencil.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o $(BIN_DIR)/l3_stencil l3_stencil.c

stream: stream.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/stream stream.c

spmv: spmv.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/spmv spmv.c

# --- Latency & Contention ---
pointer_chase: pointer_chase.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/pointer_chase pointer_chase.c

atomic_fight: atomic_fight.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o $(BIN_DIR)/atomic_fight atomic_fight.c

mpi_bandwidth: mpi_bandwidth.c | $(BIN_DIR)
	$(MPICC) $(CFLAGS) -o $(BIN_DIR)/mpi_bandwidth mpi_bandwidth.c

# --- Idle & Waiting ---
mpi_barrier: mpi_barrier.c | $(BIN_DIR)
	$(MPICC) $(CFLAGS) -o $(BIN_DIR)/mpi_barrier mpi_barrier.c

io_write: io_write.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/io_write io_write.c

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -f $(BIN_DIR)/dgemm $(BIN_DIR)/branch_mispredict $(BIN_DIR)/icache_thrash \
	      $(BIN_DIR)/tree_walk $(BIN_DIR)/fft_mix $(BIN_DIR)/l3_stencil \
	      $(BIN_DIR)/stream $(BIN_DIR)/spmv $(BIN_DIR)/pointer_chase \
	      $(BIN_DIR)/atomic_fight $(BIN_DIR)/mpi_bandwidth \
	      $(BIN_DIR)/mpi_barrier $(BIN_DIR)/io_write
