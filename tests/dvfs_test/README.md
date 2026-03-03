# Synthetic MPI DVFS Tests

This folder contains synthetic MPI applications with explicit phase behavior for DVFS-controller validation.

## Programs

- `mpi_pattern_app`: configurable repeating phases: compute -> memory -> ring communication -> synchronization.
- `dvfs_test_app`: fixed mixed pattern with compute, memory pressure, collective communication, and jittered barrier synchronization.

Both emit phase markers (`PHASE_START` / `PHASE_END`) on rank 0 for timeline alignment with performance or power logs.

## Build

```bash
cd tests/dvfs_test
make
```

Binaries are written to `bin/tests/`.

## Run examples

```bash
mpirun -np 8 ../../bin/tests/mpi_pattern_app --phase-repeats 5 --compute-iters 20000000 --mem-mb 128 --comm-rounds 1500 --sync-rounds 12000

mpirun -np 8 ../../bin/tests/dvfs_test_app --cycles 8 --work-mb 192 --msg-mb 4
```

## Slurm + LIKWID profiling

Use `submit_synthetic_mpi.sh` (modeled after existing AMD Zen4 scripts).

```bash
sbatch tests/dvfs_test/submit_synthetic_mpi.sh

APP=mpi_pattern_app APP_ARGS="--phase-repeats 6 --compute-iters 22000000 --mem-mb 160 --comm-rounds 1800 --sync-rounds 14000" \
  sbatch tests/dvfs_test/submit_synthetic_mpi.sh

APP=dvfs_test_app APP_ARGS="--cycles 10 --work-mb 192 --msg-mb 4" \
  sbatch tests/dvfs_test/submit_synthetic_mpi.sh
```

Profile output is written to `./profiles/<app>.<freq_mhz>.<jobid>.prof`.

## Notes

- Start with at least 2 MPI ranks.
- Increase compute/memory/communication settings to make phase durations longer and easier for a DVFS policy to track.
