#!/bin/bash
#SBATCH -J snap
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o snap/snap.%A.out
#SBATCH -e snap/snap.%A.err

set -euo pipefail

SNAP_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SNAP"

SNAP_BIN="${SNAP_DIR}/src/gsnap"
SNAP_INPUT="${SNAP_DIR}/qasnap/benchmark/inp_dvfs"
SNAP_OUTPUT="${SNAP_DIR}/qasnap/benchmark/out_dvfs_${SLURM_JOB_ID}.out"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

echo "Launching snap with 256 MPI ranks"

export OMP_NUM_THREADS=256

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ${SNAP_BIN} ${SNAP_INPUT} ${SNAP_OUTPUT} \
  2> "${RESULT_DIR}/snap.${SLURM_JOB_ID}.prof"