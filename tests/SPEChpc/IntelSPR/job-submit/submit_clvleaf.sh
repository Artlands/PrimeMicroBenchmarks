#!/bin/bash
#SBATCH -J spechpc_clvleaf
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --partition=h100-build
#SBATCH --ntasks-per-node=32
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 519.clvleaf_t/spechpc_clvleaf.%A.out
#SBATCH -e 519.clvleaf_t/spechpc_clvleaf.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

# Read current frequency from cpufreq policy0 (kHz) and convert to MHz
FREQ_KHZ="$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq 2>/dev/null || true)"
if [[ -n "${FREQ_KHZ}" && "${FREQ_KHZ}" =~ ^[0-9]+$ ]]; then
  FREQ_MHZ="$(( FREQ_KHZ / 1000 ))"
else
  FREQ_MHZ="NA"
fi
echo "Detected CPU frequency: ${FREQ_MHZ} MHz"

cd "${SPECHPC_DIR}"
source shrc
go "519.clvleaf_t" "${RUN_SUBDIR}"

echo "Launching 519.clvleaf with 32 MPI ranks"

likwid-perfctr -f -c 0,1 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./clvleaf \
  2> "${RESULT_DIR}/spechpc_clvleaf.${FREQ_MHZ}.${SLURM_JOB_ID}.prof"
