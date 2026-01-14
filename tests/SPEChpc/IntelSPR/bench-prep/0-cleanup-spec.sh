#!/usr/bin/env bash
set -euo pipefail

SPEC_ROOT_DEFAULT="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEChpc"

usage() {
  cat <<'EOF'
Usage: cleanup-spec-benchmarks.sh [SPEC_ROOT]

Remove generated binaries and run directories for all SPEC HPC benchmarks.

Arguments:
  SPEC_ROOT  Optional path to the SPEChpc install directory.
             Defaults to /mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEChpc
EOF
}

main() {
  local spec_root
  spec_root="${1:-${SPEC_ROOT_DEFAULT}}"

  if [[ "${spec_root}" == "-h" || "${spec_root}" == "--help" ]]; then
    usage
    exit 0
  fi

  if [[ ! -d "${spec_root}" ]]; then
    echo "SPEC root directory not found: ${spec_root}" >&2
    exit 1
  fi

  local hpc_dir
  hpc_dir="${spec_root}/benchspec/HPC"

  if [[ ! -d "${hpc_dir}" ]]; then
    echo "SPEC HPC benchmark directory not found: ${hpc_dir}" >&2
    exit 1
  fi

  shopt -s nullglob
  local -a targets=("build" "exe" "run")
  local cleaned_any=false

  for benchmark_dir in "${hpc_dir}"/*; do
    [[ -d "${benchmark_dir}" ]] || continue

    for target in "${targets[@]}"; do
      local target_path="${benchmark_dir}/${target}"

      if [[ -d "${target_path}" ]]; then
        echo "Removing ${target_path}"
        rm -rf "${target_path}"
        cleaned_any=true
      fi
    done
  done

  if [[ "${cleaned_any}" == false ]]; then
    echo "No run or exe directories found under ${hpc_dir}"
  fi
}

main "$@"
