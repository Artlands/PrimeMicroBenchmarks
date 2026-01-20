#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Ensure the module function is available in non-interactive shells.
if ! type module >/dev/null 2>&1; then
  if [ -r /etc/profile.d/modules.sh ]; then
    # shellcheck disable=SC1091
    source /etc/profile.d/modules.sh
  fi
fi

module load likwid/5.4.1-perf

nohup "${SCRIPT_DIR}/dvfs_controller" -m 0 -c 255 -p amd_edp \
  > /var/log/dvfs_controller.log 2>&1 &
