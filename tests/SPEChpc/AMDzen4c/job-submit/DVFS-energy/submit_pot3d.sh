#!/bin/bash
#SBATCH -J spechpc_pot3d
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[6-10]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 628.pot3d_s/spechpc_pot3d.%A.out
#SBATCH -e 628.pot3d_s/spechpc_pot3d.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "628.pot3d_s" "${RUN_SUBDIR}"

echo "Launching 628.pot3d with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./pot3d 1 \
  2> "${RESULT_DIR}/spechpc_pot3d.${SLURM_JOB_ID}.prof"