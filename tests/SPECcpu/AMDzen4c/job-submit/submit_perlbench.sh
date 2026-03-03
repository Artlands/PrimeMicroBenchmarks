#!/bin/bash
#SBATCH -J speccpu_perlbench
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 600.perlbench_s/speccpu_perlbench.%A.out
#SBATCH -e 600.perlbench_s/speccpu_perlbench.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu"
RUN_SUBDIR="run/run_base_refspeed_cpufreq-m64.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "600.perlbench_s" "${RUN_SUBDIR}"

echo "Launching 600.perlbench with 256 MPI instances"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./perlbench_s -I./lib splitmail.pl 6400 12 26 16 100 0 \
  2> "${RESULT_DIR}/speccpu_perlbench.${SLURM_JOB_ID}.prof"
