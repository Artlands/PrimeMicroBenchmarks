#!/bin/bash
#SBATCH -J speccpu_xalancbmk
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --nodelist=rpc-91-[1-20],rpc-92-[1-20],rpc-94-[1-20],rpc-95-[1-10]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 623.xalancbmk_s/speccpu_xalancbmk.%A.out
#SBATCH -e 623.xalancbmk_s/speccpu_xalancbmk.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu"
RUN_SUBDIR="run/run_base_refspeed_cpufreq-m64.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "623.xalancbmk_s" "${RUN_SUBDIR}"

echo "Launching 623.xalancbmk with 256 MPI instances"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./xalancbmk_s -v t5.xml xalanc.xsl \
  2> "${RESULT_DIR}/speccpu_xalancbmk.${SLURM_JOB_ID}.prof"
