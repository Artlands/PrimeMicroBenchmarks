#!/bin/bash
#SBATCH -J amg
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[16-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o amg/amg.%A.out
#SBATCH -e amg/amg.%A.err

set -euo pipefail

AMG_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/ECP-Proxy-Applications/AMG"
AMG_BIN="${AMG_DIR}/test/amg"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

echo "Launching amg with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block "${AMG_BIN}" \
  -problem 2 \
  -n 128 128 64 \
  -P 8 8 4 \
  2> "${RESULT_DIR}/amg.${SLURM_JOB_ID}.prof"