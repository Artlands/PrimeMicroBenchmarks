#!/bin/bash
for f in *.sh; do
    # Skip this script itself
    [[ "$f" == "submit_all.sh" ]] && continue
    sbatch "$f"
done