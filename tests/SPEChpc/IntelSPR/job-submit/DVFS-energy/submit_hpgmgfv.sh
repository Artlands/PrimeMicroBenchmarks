#!/bin/bash
#SBATCH -J spechpc_hpgmgfv
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --partition=h100-build
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 534.hpgmgfv_t/spechpc_hpgmgfv.%A.out
#SBATCH -e 534.hpgmgfv_t/spechpc_hpgmgfv.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "534.hpgmgfv_t" "${RUN_SUBDIR}"

echo "Launching 534.hpgmgfv with 32 MPI ranks"

likwid-perfctr -f -c 0,1 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./hpgmgfv 5 9 300 \
  2> "${RESULT_DIR}/spechpc_hpgmgfv.${SLURM_JOB_ID}.prof"
