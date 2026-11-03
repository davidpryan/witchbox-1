#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Instructions per output sample inside en_render, counted by valgrind (exact, run to run): a stand-in
# for the device's load. X0X measured ~29.5 host instructions per sample for each 1 % of an FM-1's
# audio budget (docs in fm1-x0x: 1221 / 2413 / 3521 instructions at 43 / 83 / 115 %).
#   tools/cpu.sh [SCENARIO]        (default tests/scenarios/load.wb, 20 s of a band of four)
set -e
cd "$(dirname "$0")/.."
S="${1:-tests/scenarios/load.wb}"
SECS=$(grep -o "^wait [0-9]*" "$S" | awk '{s+=$2} END {print s/1000}')
OUT=$(mktemp)
mkdir -p app/build/scenarios/load
valgrind --tool=callgrind --callgrind-out-file="$OUT" --toggle-collect=en_render \
    app/build/host/witchbox1_host "$S" app/build/scenarios/load 2>&1 | grep -E "Collected" | awk -v s="$SECS" \
    '{ir=$NF; gsub(",","",ir); printf "%.0f instructions per output sample, about %.0f %% of an FM-1\n", ir/(s*44100), ir/(s*44100)/29.5}'
rm -f "$OUT"
