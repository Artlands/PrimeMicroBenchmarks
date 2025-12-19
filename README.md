# PRIME: Pure Regime-Isolated Micro-Benchmarks

PRIME (Pure Regime-Isolated Micro-Benchmarks) is a micro-benchmark suite that isolates dominant execution and power regimes commonly observed in high-performance computing (HPC) applications. It enables controlled DVFS, power, and energy-efficiency experiments by providing minimal, orthogonal kernels whose microarchitectural behavior is intentionally constrained and verifiable.

Unlike application benchmarks or mini-apps, PRIME emphasizes purity, repeatability, and interpretability. It is well suited for:
- CPU core and uncore DVFS studies
- Power and energy modeling
- Scheduler and runtime policy evaluation
- Hardware-software co-design research
- Microarchitectural analysis using LIKWID, perf, or Top-Down methods

## Design philosophy

PRIME is built around three core principles:

1. Regime isolation  
   Each benchmark dominantly exercises one execution bottleneck (e.g., compute, bandwidth, latency, front-end, synchronization), minimizing interference from other subsystems.
2. Orthogonality  
   Benchmarks span distinct and complementary regions of the HPC execution space, avoiding redundancy while covering realistic usage patterns.
3. DVFS separability  
   Each kernel exposes a clear, testable hypothesis regarding core frequency sensitivity versus uncore (IMC/LLC/interconnect) sensitivity, enabling principled DVFS and power-modeling experiments.

## Instrumentation and metrics

PRIME is designed to be lightweight and transparent under hardware performance monitoring tools.

Typical metrics include:
- FLOPS, IPC, and stall breakdowns
- LLC hit/miss rates
- CAS/IMC traffic
- TLB misses
- Lock cycles and coherency events
- UPI or interconnect traffic
- CPU idle and I/O wait time
- RAPL energy counters (PKG, DRAM, PLATFORM)

## Controlled experiment guidelines

To ensure validity and repeatability, each benchmark should be executed with:
- Pinned threads and fixed affinity
- Explicit NUMA placement where applicable
- Fixed problem sizes chosen to enforce the intended regime (e.g., L3-resident vs DRAM-resident)
- Turbo behavior explicitly enabled or disabled
- Guardrail metrics checked per run (e.g., IPC, bandwidth thresholds)

PRIME intentionally avoids adaptive behavior or input-dependent phase changes.

## Intended use cases

PRIME is not intended to replace application benchmarks. Instead, it complements them by enabling:
- Controlled DVFS sweeps without confounding effects
- Power modeling across distinct execution regimes
- Validation of scheduler or runtime heuristics
- Attribution of energy consumption to architectural subsystems
- Reproducible microarchitectural studies across platforms

## Platforms

PRIME targets modern multicore processors with:
- Separate core and uncore frequency domains
- Hardware performance counters
- NUMA architectures

Validated platforms include recent Intel Xeon and AMD EPYC systems, though the suite is architecture-agnostic by design.

## Prerequisites

- Compiler: GCC (with OpenMP support)
- MPI: OpenMPI or Intel MPI (`mpicc` must be in your path)
- BLAS: OpenBLAS or Intel MKL (for `dgemm`)
- Utilities: `numactl` (for NUMA testing)

## Compilation

Build all benchmarks from the `src` directory:

```bash
make
```

Build specific groups or targets:

```bash
make compute
make memory
make latency
make idle
make dgemm
make mpi_bandwidth
```

## Benchmark groups and DVFS guidance

A. Compute and frontend (the "Run Fast" group)

| Benchmark | Code file | Hardware bottleneck | DVFS policy |
| --- | --- | --- | --- |
| FPU bound | `dgemm.c` | Floating point units (AVX) | Max core |
| Branch bound | `branch_mispredict.c` | Branch predictor unit (BPU) | Max core |
| Frontend bound | `icache_thrash.c` | Instruction fetch/decode | Max core |
| Integer/graph | `tree_walk.c` | Integer ALU + branching | Max core |
| Mixed/complex | `fft_mix.c` | L2 cache + ALU balance | High core + high uncore |

B. Memory and data movement (the "Feed Me" group)

| Benchmark | Code file | Hardware bottleneck | DVFS policy |
| --- | --- | --- | --- |
| L3 reuse | `l3_stencil.c` | L3 cache bandwidth | High core |
| DRAM BW | `stream.c` | Memory controller (IMC) | Med core / max uncore |
| Sparse BW | `spmv.c` | TLB + gather | Med core / max uncore |
| NUMA BW | `stream.c` (with `numactl`) | Interconnect (UPI/IF) | Med core / max uncore |

C. Latency and contention (the "System" group)

| Benchmark | Code file | Hardware bottleneck | DVFS policy |
| --- | --- | --- | --- |
| Latency | `pointer_chase.c` | DRAM access latency | Low core / max uncore |
| Coherency | `atomic_fight.c` | Cache coherence (MESI) | High core / med uncore |
| Network BW | `mpi_bandwidth.c` | PCIe / NIC | Low core / max uncore |

D. Idle and waiting (the "Sleep" group)

| Benchmark | Code file | Hardware bottleneck | DVFS policy |
| --- | --- | --- | --- |
| Spinning | `mpi_barrier.c` | None (busy wait) | Min core |
| I/O wait | `io_write.c` | Disk/storage | Min core |

## Usage examples

All benchmarks accept `--duration <seconds>` (default: 60).
Optional `--warmup <seconds>` runs a pre-measurement warm-up loop before timing.

Some benchmarks also accept:
- `--seed <int>` to control randomized data generation (e.g., `spmv`, `dgemm`, `pointer_chase`)
- `--size <bytes>` to control a size parameter (e.g., MPI message size in `mpi_bandwidth`, chunk size in `io_write`)

Example commands:
```bash
./spmv --duration 45 --seed 42
mpirun -n 2 ./mpi_bandwidth --duration 20 --size 33554432
./io_write --duration 15 --size 16777216
```

## Execution instructions

Run each benchmark as shown to trigger the intended behavior.

A. Compute and frontend (single core)

These are standard single-threaded binaries. Pin them to a specific core (e.g., core 0) to get clean measurements.

```bash
# 1. DGEMM (FPU bound)
likwid-pin -c 0 ./dgemm

# 2. Branch misprediction (bad speculation)
likwid-pin -c 0 ./branch_mispredict

# 3. I-cache thrash (frontend bound)
likwid-pin -c 0 ./icache_thrash

# 4. Tree walk (integer/branch)
likwid-pin -c 0 ./tree_walk

# 5. FFT mix (complex/mixed)
likwid-pin -c 0 ./fft_mix
```

B. Memory and data movement

These require specific handling for NUMA or uncore traffic.

```bash
# 6. L3 stencil (L3 reuse)
likwid-pin -c 0 ./l3_stencil

# 7. STREAM (local DRAM BW)
likwid-pin -c 0 ./stream

# 8. SpMV (sparse/TLB)
likwid-pin -c 0 ./spmv

# 9. NUMA remote (remote DRAM BW) - CRITICAL: requires 2 sockets
# Pin thread to socket 0 (core 0), force memory from socket 1.
numactl --cpunodebind=0 --membind=1 ./stream
```

C. Latency and contention

```bash
# 10. Pointer chase (pure latency)
likwid-pin -c 0 ./pointer_chase

# 11. Atomic fight (coherency) - CRITICAL: needs multiple threads
# Run with 4 threads on physical cores 0,1,2,3 to force cache fighting.
export OMP_NUM_THREADS=4
likwid-pin -c 0-3 ./atomic_fight

# 12. MPI bandwidth (interconnect) - CRITICAL: needs 2 processes
# If on one node, pin to core 0 (socket 0) and core 10 (socket 1) to force cross-socket traffic.
mpirun -np 2 --map-by core --bind-to core ./mpi_bandwidth
# Note: for real cluster testing, run this on 2 separate nodes.
```

D. Idle and waiting

```bash
# 13. MPI barrier (spinning)
# Ensure your MPI is configured to spin (Intel MPI default).
mpirun -np 4 ./mpi_barrier

# 14. I/O write (disk wait)
# Ensure /tmp is actually a disk, not a RAM disk (tmpfs).
# If /tmp is tmpfs, change the path in the source code to a real HDD/SSD path.
likwid-pin -c 0 ./io_write
```

## License

PRIME is released under a permissive open-source license suitable for academic and industrial research. See `LICENSE` for details.
