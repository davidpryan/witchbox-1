#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# witchbox1's tests: the sound engine (tests/host/engine_test.c), every ported toy against Playbox's own Python
# (ref/conform.sh), the whole app in fm1-base's simulator (tests/scenarios/*.wb: the panel, the band, the
# arrangements, saves, every shelf page), with screenshots and audio in app/build/scenarios/; the browser build
# when emcc is here; and the update path against witchbox1's package (./build.sh first). fm1-base's own tests:
# base/tests/run_tests.sh.
#   tests/run_tests.sh            (QUICK=1 shortens the conformance sweep)
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
PY="${PYTHON:-python3}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
B=app/build
OUT=$B/tests
FAIL=0
mkdir -p "$OUT" $B/scenarios $B/gen
[ -f $B/gen/wb_samples.h ] || $PY app/tools/gen_samples.py $B/gen/wb_samples.h >/dev/null
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
run engine sh -c "$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wdouble-promotion -Werror -DEN_HOST -Iapp/dsp -Ibase/include -I$B/gen \
    -o $OUT/engine_test tests/host/engine_test.c app/dsp/wb_engine.c -lm && $OUT/engine_test"
if [ -z "${QUICK:-}" ]; then
    run conformance sh -c "MS=20000 SEEDS='0 7 1234 9999' ref/conform.sh"
else
    run conformance-quick sh -c "MS=8000 SEEDS='1234' ref/conform.sh"
fi
if [ -f $B/witchbox1.fwsc ]; then
    run ota-entry sh -c "$CC -w -o $OUT/ota_test base/tests/ota_test.c && $OUT/ota_test $B/witchbox1.fwsc"
    run update-loader sh -c "head -c 100000 $B/witchbox1.bin > $OUT/old_app.bin && \
        $PY base/tools/fm1pkg_make.py $OUT/old_app.bin $B/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -w -o $OUT/ldr_test base/tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc $B/witchbox1.fwsc"
else
    echo "  skip update path (no $B/witchbox1.fwsc: ./build.sh)"
fi
run host-build sh build.sh --host
if command -v emcc >/dev/null 2>&1; then
    run emu sh build.sh --web
else
    echo "  skip emu (no emcc)"
fi
for s in tests/scenarios/*.wb; do
    n=$(basename "$s" .wb)
    mkdir -p "$B/scenarios/$n"
    run "scenario-$n" $B/host/witchbox1_host "$s" "$B/scenarios/$n"
done
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
