#!/usr/bin/env bash
set -euo pipefail

SPEC_ROOT_DEFAULT="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu"

usage() {
  cat <<'EOF'
Usage: cleanup-spec-benchmarks.sh [SPEC_ROOT]

Remove generated binaries and run directories for all SPEC CPU benchmarks.

Arguments:
  SPEC_ROOT  Optional path to the SPECcpu install directory.
             Defaults to /mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-CPU/SPECcpu
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

  local cpu_dir
  cpu_dir="${spec_root}/benchspec/CPU"

  if [[ ! -d "${cpu_dir}" ]]; then
    echo "SPEC CPU benchmark directory not found: ${cpu_dir}" >&2
    exit 1
  fi

  shopt -s nullglob
  local -a targets=("build" "exe" "run")
  local cleaned_any=false

  for benchmark_dir in "${cpu_dir}"/*; do
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
    echo "No run or exe directories found under ${cpu_dir}"
  fi
}

main "$@"
