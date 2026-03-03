#!/bin/bash
#SBATCH -J miniQMC
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --nodelist=rpc-91-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o miniQMC/miniQMC.%A.out
#SBATCH -e miniQMC/miniQMC.%A.err

set -euo pipefail

miniQMC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/ECP-Proxy-Applications/miniqmc/build/bin"
miniQMC_BIN="${miniQMC_DIR}/miniqmc"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${miniQMC_DIR}"

echo "Launching miniQMC with 256 threads"

export OMP_NUM_THREADS=256
export OMP_PROC_BIND=true

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block "${miniQMC_BIN}" -g "8 4 4" \
  2> "${RESULT_DIR}/miniQMC.${SLURM_JOB_ID}.prof"