#!/usr/bin/env bash

set -euo pipefail

AVAILABLE_FREQS=("2.25" "1.80" "1.50")

usage() {
    cat <<'EOF'
Usage: change-cpu-freq.sh -n <node_list> [-f <freq>] [-g <governor>]

Options:
  -n  Node list compatible with pdsh's -w argument (required).
  -f  Desired CPU frequency in GHz. Valid values: 2.25, 1.80, 1.50.
  -g  Desired CPU frequency governor (e.g., conservative, ondemand, powersave, performance, schedutil, userspace).
  -h  Show this help message.

If both -f and -g are omitted, the script prompts you to select one of the supported frequencies.
EOF
}

require_root() {
    if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
        echo "Error: this script must be run as root." >&2
        exit 1
    fi
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: required command '$1' not found in PATH." >&2
        exit 1
    fi
}

verify_frequency() {
    local nodelist="$1"
    local freq="$2"

    echo "Verifying frequency on nodes '$nodelist'..."
    local verify_cmd=$'current_line=$(cpupower frequency-info | awk -F: \'/current CPU frequency/ {sub(/\\(.*/, "", $2); gsub(/^[ \\t]+|[ \\t]+$/, "", $2); print $2; exit}\'); \
if [ -z "$current_line" ]; then \
    echo "MISMATCH: unable to read current CPU frequency"; \
    exit 1; \
fi; \
if [ "$current_line" = "${target_freq} GHz" ]; then \
    echo "OK: current CPU frequency $current_line"; \
else \
    echo "MISMATCH: reported $current_line (expected ${target_freq} GHz)"; \
    exit 1; \
fi'

    local remote_cmd
    printf -v remote_cmd "target_freq=%q; %s" "$freq" "$verify_cmd"

    if ! pdsh -w "$nodelist" "$remote_cmd"; then
        echo "Error: not all nodes report the requested frequency (${freq}GHz)." >&2
        exit 1
    fi

    echo "Frequency verified successfully."
}

verify_governor() {
    local nodelist="$1"
    local governor="$2"

    echo "Verifying governor on nodes '$nodelist'..."
    local verify_cmd=$'gov_path=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor; \
if [ ! -r "$gov_path" ]; then \
    echo "MISMATCH: unable to read $gov_path"; \
    exit 1; \
fi; \
current=$(tr -d "\\r\\n" < "$gov_path"); \
if [ "$current" = "$target_governor" ]; then \
    echo "OK: current governor $current"; \
else \
    echo "MISMATCH: reported $current (expected $target_governor)"; \
    exit 1; \
fi'

    local remote_cmd
    printf -v remote_cmd "target_governor=%q; %s" "$governor" "$verify_cmd"

    if ! pdsh -w "$nodelist" "$remote_cmd"; then
        echo "Error: not all nodes report the requested governor ($governor)." >&2
        exit 1
    fi

    echo "Governor verified successfully."
}

select_frequency() {
    echo "Select target CPU frequency:"
    local idx=1
    for freq in "${AVAILABLE_FREQS[@]}"; do
        printf "  [%d] %s GHz\n" "$idx" "$freq"
        ((idx++))
    done

    while true; do
        read -rp "Enter choice (1-${#AVAILABLE_FREQS[@]}): " choice
        if [[ "$choice" =~ ^[0-9]+$ ]] && ((choice >= 1 && choice <= ${#AVAILABLE_FREQS[@]})); then
            echo "${AVAILABLE_FREQS[choice-1]}"
            return
        fi
        echo "Invalid choice. Try again."
    done
}

validate_frequency() {
    local value="$1"
    for freq in "${AVAILABLE_FREQS[@]}"; do
        if [[ "$value" == "$freq" ]]; then
            echo "$value"
            return
        fi
    done
    echo "Error: unsupported frequency '$value'. Allowed: ${AVAILABLE_FREQS[*]}." >&2
    exit 1
}

main() {
    local nodelist=""
    local freq=""
    local governor=""

    require_root

    while getopts ":n:f:g:h" opt; do
        case "$opt" in
            n) nodelist="$OPTARG" ;;
            f) freq="$OPTARG" ;;
            g) governor="$OPTARG" ;;
            h)
                usage
                exit 0
                ;;
            :)
                echo "Error: option -$OPTARG requires an argument." >&2
                usage
                exit 1
                ;;
            \?)
                echo "Error: invalid option -$OPTARG." >&2
                usage
                exit 1
                ;;
        esac
    done

    if [[ -z "$nodelist" ]]; then
        echo "Error: node list (-n) is required." >&2
        usage
        exit 1
    fi

    if [[ -z "$freq" && -z "$governor" ]]; then
        freq="$(select_frequency)"
    fi

    if [[ -n "$freq" ]]; then
        freq="$(validate_frequency "$freq")"
    fi

    require_command pdsh

    local cmd="cpupower frequency-set"
    if [[ -n "$freq" ]]; then
        cmd+=" -f ${freq}GHz"
    fi
    if [[ -n "$governor" ]]; then
        cmd+=" -g $governor"
    fi

    echo "Applying settings to nodes '$nodelist'..."
    pdsh -w "$nodelist" "$cmd"

    if [[ -n "$freq" ]]; then
        verify_frequency "$nodelist" "$freq"
    fi
    if [[ -n "$governor" ]]; then
        verify_governor "$nodelist" "$governor"
    fi
}

main "$@"
