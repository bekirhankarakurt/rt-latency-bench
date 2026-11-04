#!/usr/bin/env bash
# Records docs/demo.cast from a real run and renders docs/demo.gif.
# Requires: asciinema (2.x), agg, a built ./build/rtlb.
set -eu
cd "$(dirname "$0")/.."
PATH="$PWD/build:$PATH" asciinema rec --overwrite --cols 110 --rows 30 \
    -c "bash scripts/demo.sh" docs/demo.cast
agg --font-size 16 --speed 1.3 --last-frame-duration 3 docs/demo.cast docs/demo.gif
