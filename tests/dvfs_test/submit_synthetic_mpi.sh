#!/bin/bash
#SBATCH -J PRIME_DVFS_SYNTH
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=2:00:00
#SBATCH -o ./out/synthetic_mpi.%j.out
#SBATCH -e /dev/null

set -euo pipefail

# Usage examples:
#   sbatch tests/dvfs_test/submit_synthetic_mpi.sh
#   APP=mpi_pattern_app APP_ARGS="--phase-repeats 6 --compute-iters 22000000 --mem-mb 160" sbatch tests/dvfs_test/submit_synthetic_mpi.sh
#   APP=dvfs_test_app APP_ARGS="--cycles 10 --work-mb 192 --msg-mb 4" sbatch tests/dvfs_test/submit_synthetic_mpi.sh

APP="${APP:-mpi_pattern_app}"
APP_ARGS="${APP_ARGS:---phase-repeats 5 --compute-iters 20000000 --mem-mb 128 --comm-rounds 1500 --sync-rounds 12000}"

if [[ "${APP}" != "mpi_pattern_app" && "${APP}" != "dvfs_test_app" ]]; then
  echo "Unsupported APP='${APP}'. Use mpi_pattern_app or dvfs_test_app." >&2
  exit 1
fi

if [[ "${APP}" == "dvfs_test_app" && "${APP_ARGS}" == "--phase-repeats 5 --compute-iters 20000000 --mem-mb 128 --comm-rounds 1500 --sync-rounds 12000" ]]; then
  APP_ARGS="--cycles 8 --work-mb 192 --msg-mb 4"
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN_DIR="${ROOT_DIR}/bin/tests"
BIN="${BIN_DIR}/${APP}"
RESULT_DIR="./profiles"
OUT_DIR="./out"

mkdir -p "${RESULT_DIR}" "${OUT_DIR}"

module purge
module load likwid/5.4.1-perf mpich/4.3.2

if [[ ! -x "${BIN}" ]]; then
  echo "Binary not found: ${BIN}" >&2
  echo "Build first: make -C ${ROOT_DIR}/tests/dvfs_test" >&2
  exit 2
fi

# Read current frequency from cpufreq policy0 (kHz) and convert to MHz.
FREQ_KHZ="$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq 2>/dev/null || true)"
if [[ -n "${FREQ_KHZ}" && "${FREQ_KHZ}" =~ ^[0-9]+$ ]]; then
  FREQ_MHZ="$(( FREQ_KHZ / 1000 ))"
else
  FREQ_MHZ="NA"
fi

echo "Detected CPU frequency: ${FREQ_MHZ} MHz"
echo "Running APP=${APP}"
echo "APP_ARGS=${APP_ARGS}"

srun --cpu-bind=cores --distribution=block:block \
  bash -c '
    if [ "$SLURM_PROCID" -eq 0 ]; then
      exec taskset -c 0 likwid-perfctr -f -c 0 -g HPC_DVFS_MODEL_AMD -t 500ms -O -- '"${BIN}"' '"${APP_ARGS}"'
    else
      exec '"${BIN}"' '"${APP_ARGS}"' >/dev/null 2>&1
    fi
  ' \
  2> "${RESULT_DIR}/${APP}.${FREQ_MHZ}.${SLURM_JOB_ID}.prof"
