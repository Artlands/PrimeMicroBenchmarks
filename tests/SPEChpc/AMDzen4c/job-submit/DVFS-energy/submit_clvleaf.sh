#!/bin/bash
#SBATCH -J spechpc_clvleaf
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[6-10]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 619.clvleaf_s/spechpc_clvleaf.%A.out
#SBATCH -e 619.clvleaf_s/spechpc_clvleaf.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "619.clvleaf_s" "${RUN_SUBDIR}"

echo "Launching 619.clvleaf with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./clvleaf \
  2> "${RESULT_DIR}/spechpc_clvleaf.${SLURM_JOB_ID}.prof"
