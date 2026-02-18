#!/bin/bash

exclude=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --exclude)
            shift
            while [[ $# -gt 0 && "$1" != --* ]]; do
                exclude+=("$1")
                shift
            done
            ;;
        *)
            echo "Usage: $0 [--exclude bench1 bench2 ...]" >&2
            exit 2
            ;;
    esac
done

for f in *.sh; do
    # Skip this script itself
    [[ "$f" == "submit_all.sh" ]] && continue

    base="${f%.sh}"
    bench="${base#submit_}"
    for skip in "${exclude[@]}"; do
        [[ "$skip" == "$base" || "$skip" == "$bench" || "$skip" == "${base}.sh" ]] && continue 2
    done

    sbatch "$f"
done
