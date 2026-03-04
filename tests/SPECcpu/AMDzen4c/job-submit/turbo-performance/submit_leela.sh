#!/bin/bash
#SBATCH -J speccpu_leela
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --nodelist=rpc-91-[1-20],rpc-92-[1-20],rpc-94-[1-20],rpc-95-[1-10]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 641.leela_s/speccpu_leela.%A.out
#SBATCH -e 641.leela_s/speccpu_leela.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu"
RUN_SUBDIR="run/run_base_refspeed_cpufreq-m64.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "641.leela_s" "${RUN_SUBDIR}"

echo "Launching 641.leela with 256 MPI instances"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./leela_s ref.sgf \
  2> "${RESULT_DIR}/speccpu_leela.${SLURM_JOB_ID}.prof"
