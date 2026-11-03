#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the witchbox1 host simulator (the whole app on this machine) into build/host/wb_host.
# Same sources as the firmware, -ffp-contract=off like the device (no fused multiply-add).
set -e
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
mkdir -p build/gen build/host
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/wb_samples.h ] || python3 tools/gen_samples.py build/gen/wb_samples.h >/dev/null
$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DEN_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -D_POSIX_C_SOURCE=200809L -o build/host/wb_host host/wb_host.c \
    firmware/src/dsp/wb_engine.c -lm
echo "host: build/host/wb_host"
