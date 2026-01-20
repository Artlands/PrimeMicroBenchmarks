#!/usr/bin/env bash
set -euo pipefail

# Stop all running DVFS controller instances on the node.
if pgrep -x dvfs_controller >/dev/null 2>&1; then
  pkill -x dvfs_controller
fi
