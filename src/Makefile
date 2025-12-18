# Compiler Settings
CC = gcc
MPICC = mpicc
CFLAGS = -O3 -march=native -Wall
OMP_FLAGS = -fopenmp
# Adjust LIBS if using Intel MKL (e.g., -lmkl_intel_lp64 -lmkl_sequential -lmkl_core)
LIBS = -lopenblas -lm

# Targets
all: compute memory latency idle

compute: dgemm branch_mispredict icache_thrash tree_walk fft_mix
memory: l3_stencil stream spmv
latency: pointer_chase atomic_fight mpi_bandwidth
idle: mpi_barrier io_write

# --- Compute & Frontend ---
dgemm: dgemm.c
	$(CC) $(CFLAGS) -o dgemm dgemm.c $(LIBS)

branch_mispredict: branch_mispredict.c
	# Critical: Disable vectorization to keep the branch logic intact
	$(CC) -O3 -fno-tree-vectorize -fno-if-conversion -o branch_mispredict branch_mispredict.c

icache_thrash: icache_thrash.c
	# Critical: -O0 is required to prevent code folding/loop removal
	$(CC) -O0 -o icache_thrash icache_thrash.c

tree_walk: tree_walk.c
	$(CC) $(CFLAGS) -o tree_walk tree_walk.c

fft_mix: fft_mix.c
	$(CC) $(CFLAGS) -o fft_mix fft_mix.c -lm

# --- Memory & Data ---
l3_stencil: l3_stencil.c
	$(CC) $(CFLAGS) -o l3_stencil l3_stencil.c

stream: stream.c
	$(CC) $(CFLAGS) -o stream stream.c

spmv: spmv.c
	$(CC) $(CFLAGS) -o spmv spmv.c

# --- Latency & Contention ---
pointer_chase: pointer_chase.c
	$(CC) $(CFLAGS) -o pointer_chase pointer_chase.c

atomic_fight: atomic_fight.c
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o atomic_fight atomic_fight.c

mpi_bandwidth: mpi_bandwidth.c
	$(MPICC) $(CFLAGS) -o mpi_bandwidth mpi_bandwidth.c

# --- Idle & Waiting ---
mpi_barrier: mpi_barrier.c
	$(MPICC) $(CFLAGS) -o mpi_barrier mpi_barrier.c

io_write: io_write.c
	$(CC) $(CFLAGS) -o io_write io_write.c

clean:
	rm -f dgemm branch_mispredict icache_thrash tree_walk fft_mix \
	      l3_stencil stream spmv pointer_chase atomic_fight \
	      mpi_bandwidth mpi_barrier io_write
