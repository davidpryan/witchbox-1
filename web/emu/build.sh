#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# witchbox1 in the browser: the host simulator compiled to WebAssembly, plus its page.
#   [EN_VERSION=0.1] web/emu/build.sh  ->  build/emu/{index.html, worklet.js, witchbox1.wasm}
# Same sources and flags as host/build_host.sh (-ffp-contract=off, like the device). Needs only clang
# with the wasm32 target and wasm-ld (no Emscripten, no libc: the build is freestanding).
set -e
cd "$(dirname "$0")/../.."
CLANG="${CLANG:-clang}"
mkdir -p build/gen build/emu
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/wb_samples.h ] || python3 tools/gen_samples.py build/gen/wb_samples.h >/dev/null
$CLANG --target=wasm32 -O2 -ffp-contract=off -std=gnu99 -ffreestanding -fno-builtin -nostdlib -mnontrapping-fptoint \
    -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-incompatible-library-redeclaration \
    -DEN_HOST -DEN_WEB "-DEN_VERSION=\"$(printf %s "${EN_VERSION:-DEV}" | tr a-z A-Z)\"" -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -Wl,--no-entry -Wl,--export-dynamic -Wl,-z,stack-size=1048576 -Wl,--initial-memory=8388608 \
    -o build/emu/witchbox1.wasm web/emu/wb_web.c firmware/src/dsp/wb_engine.c
cp web/emu/index.html web/emu/worklet.js build/emu/
echo "emu: build/emu ($(wc -c < build/emu/witchbox1.wasm) B wasm)"
