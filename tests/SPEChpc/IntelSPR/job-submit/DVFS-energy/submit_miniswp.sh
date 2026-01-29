#!/bin/bash
#SBATCH -J spechpc_miniswp
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --partition=h100-build
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 521.miniswp_t/spechpc_miniswp.%A.out
#SBATCH -e 521.miniswp_t/spechpc_miniswp.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "521.miniswp_t" "${RUN_SUBDIR}"

echo "Launching 521.miniswp with 32 MPI ranks"

likwid-perfctr -f -c 0,1 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./sweep --niterations 40 --ncell_x 96 --ncell_y 64 --ncell_z 64 --ne 64 --na 32 --nblock_z 8 --nthread_e 1 \
  2> "${RESULT_DIR}/spechpc_miniswp.${SLURM_JOB_ID}.prof"
