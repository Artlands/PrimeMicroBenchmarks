#!/bin/bash
#SBATCH -J speccpu_bwaves
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 603.bwaves_s/speccpu_bwaves.%A.out
#SBATCH -e 603.bwaves_s/speccpu_bwaves.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu"
RUN_SUBDIR="run/run_base_refspeed_cpufreq-m64.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "603.bwaves_s" "${RUN_SUBDIR}"

echo "Launching 603.bwaves with 256 instances"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./speed_bwaves bwaves_2 < bwaves_2.in \
  2> "${RESULT_DIR}/speccpu_bwaves.${SLURM_JOB_ID}.prof"
