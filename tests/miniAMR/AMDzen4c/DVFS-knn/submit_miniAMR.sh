#!/bin/bash
#SBATCH -J miniAMR
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o miniAMR/miniAMR.%A.out
#SBATCH -e miniAMR/miniAMR.%A.err

set -euo pipefail

MINIAMR_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/ECP-Proxy-Applications/miniAMR/openmp"
MINIAMR_BIN="${MINIAMR_DIR}/miniAMR.x"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${MINIAMR_DIR}"

echo "Launching miniAMR with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block "${MINIAMR_BIN}" \
  --npx 8 --npy 8 --npz 4 \
  --nx 16 --ny 16 --nz 16 \
  --init_x 1 --init_y 1 --init_z 1 \
  --num_refine 4 --uniform_refine 0 \
  --max_blocks 3000 \
  --num_vars 20 \
  --stages_per_ts 500 \
  --num_tsteps 1000 \
  2> "${RESULT_DIR}/miniAMR.${SLURM_JOB_ID}.prof"