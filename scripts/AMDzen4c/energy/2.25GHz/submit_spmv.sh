#!/bin/bash
#SBATCH -J PRIME
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=2:00:00
#SBATCH -o ./out/spmv.%j.out
#SBATCH -e /dev/null

set -euo pipefail

BIN="/mnt/REPACSS/home/li29729/Research2025/CPUfreq/PrimeMicroBenchmarks/bin/amd"
RESULT_DIR="./profiles"

mkdir -p "${RESULT_DIR}"

module load likwid/5.4.1-daemon

# Read current frequency from cpufreq policy0 (kHz) and convert to MHz
FREQ_KHZ="$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq 2>/dev/null || true)"
if [[ -n "${FREQ_KHZ}" && "${FREQ_KHZ}" =~ ^[0-9]+$ ]]; then
  FREQ_MHZ="$(( FREQ_KHZ / 1000 ))"
else
  FREQ_MHZ="NA"
fi

echo "Detected CPU frequency: ${FREQ_MHZ} MHz"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --cpu-bind=cores --distribution=block:block "${BIN}/spmv" \
  2> "${RESULT_DIR}/spmv.${FREQ_MHZ}.${SLURM_JOB_ID}.prof"