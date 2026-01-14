#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BENCHMARK_FILE="${SCRIPT_DIR}/tiny.txt"
SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEChpc"
BUILD_SUBDIR="build/build_base_gnu_mpi.0000"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

HOSTNAME_SHORT="$(hostname -s)"
if [[ "${HOSTNAME_SHORT}" != rpg-93* ]]; then
  echo "This script must be executed on an rpg-93 node; current host: ${HOSTNAME_SHORT}" >&2
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

declare -A BINARY_OVERRIDES=(
  ["521.miniswp"]="sweep"
)

for benchmark in "${BENCHMARKS[@]}"; do
  base_name="${benchmark%_t}"
  binary_name="${BINARY_OVERRIDES[${base_name}]:-${base_name#*.}}"

  echo "Processing benchmark ${base_name}"

  go "${base_name}" "${BUILD_SUBDIR}"
  specmake clean >/dev/null 2>&1
  specmake >/dev/null 2>&1

  go "${base_name}" "${RUN_SUBDIR}"

  binary_source="../../${BUILD_SUBDIR}/${binary_name}"
  if [[ -f "${binary_source}" ]]; then
    cp "${binary_source}" .
  else
    echo "Warning: binary not found for ${base_name} at ${binary_source}" >&2
  fi
done
