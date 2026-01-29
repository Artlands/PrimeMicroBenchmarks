#!/bin/bash
#SBATCH -J spechpc_soma
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[11-15]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 613.soma_s/spechpc_soma.%A.out
#SBATCH -e 613.soma_s/spechpc_soma.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "613.soma_s" "${RUN_SUBDIR}"

echo "Launching 613.soma with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./soma -r 42 -t 400 --npoly=25000000 --gen-state-file \
  2> "${RESULT_DIR}/spechpc_soma.${SLURM_JOB_ID}.prof"