#!/bin/bash
#SBATCH -J hpcg
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[11-15]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o hpcg/hpcg.%A.out
#SBATCH -e hpcg/hpcg.%A.err

set -euo pipefail

HPCG_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/hpcg/build/bin"
HPCG="${HPCG_DIR}/xhpcg"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${HPCG_DIR}"

echo "Launching hpcg with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block "${HPCG}" \
  2> "${RESULT_DIR}/hpcg.${SLURM_JOB_ID}.prof"