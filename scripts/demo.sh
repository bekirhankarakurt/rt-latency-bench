#!/usr/bin/env bash
# Scripted terminal session used to record docs/demo.gif (see scripts/record-demo.sh).
set -eu
type_and_run() {
    printf '\033[1;32m$\033[0m '
    for ((i = 0; i < ${#1}; i++)); do
        printf '%s' "${1:i:1}"
        sleep 0.03
    done
    printf '\n'
    sleep 0.3
    eval "$1"
    sleep 1.5
}
type_and_run "rtlb -t 2 -a 0-1 -l cpu,mem -d 4 --histogram"
type_and_run "rtlb compare -a 0 -l cpu -d 3"
