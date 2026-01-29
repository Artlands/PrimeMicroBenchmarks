#!/bin/bash
#SBATCH -J spechpc_lbm
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --partition=h100-build
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 505.lbm_t/spechpc_lbm.%A.out
#SBATCH -e 505.lbm_t/spechpc_lbm.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "505.lbm_t" "${RUN_SUBDIR}"

echo "Launching 505.lbm with 32 MPI ranks"

likwid-perfctr -f -c 0,1 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./lbm \
  2> "${RESULT_DIR}/spechpc_lbm.${SLURM_JOB_ID}.prof"
