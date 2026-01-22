#!/usr/bin/env bash
set -euo pipefail

PIDS="$(pgrep -f dvfs_controller || true)"

if [ -z "${PIDS}" ]; then
  echo "No dvfs_controller processes running."
  exit 0
fi

echo "Stopping dvfs_controller: ${PIDS}"
kill ${PIDS}

sleep 2

# Force kill if still running
REMAINING="$(pgrep -f dvfs_controller || true)"
if [ -n "${REMAINING}" ]; then
  echo "Processes still running, sending SIGKILL: ${REMAINING}"
  kill -9 ${REMAINING}
fi