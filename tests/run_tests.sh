#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# witchbox1 host tests: the maths, the platform pieces kept from Felucca / X0X / AMB-1 / HAUNT-1, the
# sound engine, every toy against Playbox's own Python (ref/conform.sh), and the whole firmware app in
# the simulator (tests/scenarios/*.wb: the panel, the band, saves), with screenshots and audio in
# build/scenarios/; then the browser build, if clang can make it.
#   tests/run_tests.sh            (QUICK=1 skips the long conformance sweep)
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
FAIL=0
OUT=build/host
mkdir -p "$OUT" build/scenarios build/gen
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/wb_samples.h ] || python3 tools/gen_samples.py build/gen/wb_samples.h >/dev/null
run() {
    name="$1"; shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        echo "  ok   $name"
    else
        echo "  FAIL $name (see $OUT/$name.log)"
        tail -15 "$OUT/$name.log" | sed 's/^/       /'
        FAIL=1
    fi
}
run fastmath sh -c "$CC -O2 -ffp-contract=off -Wall -Wextra -Werror -o $OUT/fastmath_test tests/host/fastmath_test.c -lm && $OUT/fastmath_test"
run encoder sh -c "$CC -O2 -w -Ifirmware/hal -o $OUT/encoder_test tests/host/encoder_test.c && $OUT/encoder_test"
run uac sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/uac_test tests/host/uac_test.c -lm && $OUT/uac_test"
run trs sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/trs_test tests/host/trs_test.c && $OUT/trs_test"
run engine sh -c "$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wdouble-promotion -Werror -DEN_HOST -Ifirmware/src/dsp -Ibuild/gen \
    -o $OUT/engine_test tests/host/engine_test.c firmware/src/dsp/wb_engine.c -lm && $OUT/engine_test"
run storage sh -c "$CC -O2 -o $OUT/storage_test tests/storage_test.c && $OUT/storage_test"
if [ -z "${QUICK:-}" ]; then
    run conformance sh -c "MS=20000 SEEDS='0 7 1234 9999' ref/conform.sh"
else
    run conformance-quick sh -c "MS=8000 SEEDS='1234' ref/conform.sh"
fi
# the update path, against the firmware package (Felucca's tests; needs ./build.sh)
if [ -f build/witchbox1.fwsc ]; then
    run ota-entry sh -c "$CC -o $OUT/ota_test tests/ota_test.c && $OUT/ota_test build/witchbox1.fwsc"
    if [ -z "${AC79_SDK:-}" ]; then
        echo "  skip update-loader (needs AC79_SDK, as the build)"
    else
    run update-loader sh -c "head -c 100000 build/witchbox1.bin > $OUT/old_app.bin && \
        python3 tools/fm1pkg_make.py $OUT/old_app.bin build/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -o $OUT/ldr_test tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc build/witchbox1.fwsc"
    fi
    run installer python3 tests/install_test.py
else
    echo "  skip update-path tests (no build/witchbox1.fwsc: run ./build.sh)"
fi
run host-build sh host/build_host.sh
if echo 'int x;' | ${CLANG:-clang} --target=wasm32 -c -x c -o /dev/null - 2>/dev/null && command -v wasm-ld >/dev/null 2>&1 &&
    command -v node >/dev/null 2>&1; then
    run emu sh -c "sh web/emu/build.sh >/dev/null && node tests/host/emu_test.mjs build/emu/witchbox1.wasm"
else
    echo "  skip emu (needs clang with the wasm32 target, wasm-ld and node)"
fi
for s in tests/scenarios/*.wb; do
    n=$(basename "$s" .wb)
    mkdir -p "build/scenarios/$n"
    run "scenario-$n" build/host/wb_host "$s" "build/scenarios/$n"
done
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
