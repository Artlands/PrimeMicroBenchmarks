#!/bin/bash
#SBATCH -J spechpc_sph_exa
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --partition=h100-build
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 532.sph_exa_t/spechpc_sph_exa.%A.out
#SBATCH -e 532.sph_exa_t/spechpc_sph_exa.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "532.sph_exa_t" "${RUN_SUBDIR}"

echo "Launching 532.sph_exa with 32 MPI ranks"

likwid-perfctr -f -c 0,1 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./sph_exa -n 210 -s 80 -w -1 \
  2> "${RESULT_DIR}/spechpc_sph_exa.${SLURM_JOB_ID}.prof"