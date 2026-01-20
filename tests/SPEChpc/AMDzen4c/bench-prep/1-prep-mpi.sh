#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BENCHMARK_FILE="${SCRIPT_DIR}/small.txt"
SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RANK=256

HOSTNAME_SHORT="$(hostname -s)"
if [[ "${HOSTNAME_SHORT}" != rpc-97-* ]]; then
  echo "This script must be executed on an rpc-97 node; current host: ${HOSTNAME_SHORT}" >&2
  exit 1
fi

if [[ ! -f "${BENCHMARK_FILE}" ]]; then
  echo "Benchmark list not found: ${BENCHMARK_FILE}" >&2
  exit 1
fi

mapfile -t BENCHMARKS < <(grep -Ev '^\s*(#|$)' "${BENCHMARK_FILE}")

cd "${SPECHPC_DIR}"
source shrc
ml load mpich/4.3.2

mkdir -p "${SCRIPT_DIR}/mpi-amd-logs"

for benchmark in "${BENCHMARKS[@]}"; do
  echo "Starting benchmark ${benchmark}"
  runhpc \
    --fake \
    --loose \
    --size ref \
    --tune base \
    --config REPACSS_GPU \
    --define model=mpi \
    --ranks "${RANK}" \
    "${benchmark}" \
    >"${SCRIPT_DIR}/mpi-amd-logs/${benchmark}_fake_mpi.log" 2>&1
  echo "Completed benchmark ${benchmark}"
done
