#!/bin/bash
#SBATCH -J pennant
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o pennant/pennant.%A.out
#SBATCH -e pennant/pennant.%A.err

set -euo pipefail

PENNANT_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/PENNANT"
PENNANT_BIN="${PENNANT_DIR}/build/pennant"
PENNANT_INPUT="${PENNANT_DIR}/test/sedovflat/sedovflat.pnt"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load openmpi/4.1.8 likwid/5.4.1-daemon

echo "Launching pennant with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block "${PENNANT_BIN}" "${PENNANT_INPUT}" \
  2> "${RESULT_DIR}/pennant.${SLURM_JOB_ID}.prof"