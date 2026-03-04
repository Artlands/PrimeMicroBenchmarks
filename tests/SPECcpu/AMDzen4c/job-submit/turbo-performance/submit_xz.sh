#!/bin/bash
#SBATCH -J speccpu_xz
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --nodelist=rpc-91-[1-20],rpc-92-[1-20],rpc-94-[1-20],rpc-95-[1-10]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 657.xz_s/speccpu_xz.%A.out
#SBATCH -e 657.xz_s/speccpu_xz.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu"
RUN_SUBDIR="run/run_base_refspeed_cpufreq-m64.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "657.xz_s" "${RUN_SUBDIR}"

echo "Launching 657.xz with 256 MPI instances"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./xz_s cld.tar.xz 1400 19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474 536995164 539938872 8 \
  2> "${RESULT_DIR}/speccpu_xz.${SLURM_JOB_ID}.prof"
