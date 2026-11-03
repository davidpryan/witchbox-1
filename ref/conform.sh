#!/bin/sh
# Conformance: every toy, every seed listed, Python reference against the C port.
#   ref/conform.sh [toy ...]        (default: every toy in toys/all.c that Playbox v0.9 has: the
#                                    Witch III toys are witchbox1's own, with no reference)
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
mkdir -p build/ref
$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Werror -o build/ref/run_c ref/run_c.c || exit 1
TOYS="$*"
[ -z "$TOYS" ] && TOYS=$(grep -o '&toy_[a-z0-9]*' app/wb/toys/all.c | sed 's/&toy_//' | while read t; do [ -f ref/playbox/$t.py ] && echo $t; done)
SEEDS="${SEEDS:-0 1 7 42 77 1234 2024 3333 4096 5555 6789 7777 8080 9000 9999 123 456 789 31 64}"
MS="${MS:-30000}"
fail=0
for toy in $TOYS; do
    bad=0
    for seed in $SEEDS; do
        for mode in plain shake; do
            if [ $mode = shake ]; then X="--shake $((seed * 7 % 10000)) --params"; else X="--params"; fi
            python3 ref/run_ref.py $toy $seed --ms $MS $X > build/ref/ref.txt 2>build/ref/ref.err || { echo "  ref error $toy $seed: $(tail -1 build/ref/ref.err)"; bad=1; continue; }
            build/ref/run_c $toy $seed --ms $MS $X > build/ref/c.txt 2>&1
            if ! cmp -s build/ref/ref.txt build/ref/c.txt; then
                bad=1
                echo "  MISMATCH $toy seed $seed $mode: $(diff build/ref/ref.txt build/ref/c.txt | head -3 | tr '\n' ' ')"
                break
            fi
        done
        [ $bad = 1 ] && break
    done
    if [ $bad = 0 ]; then echo "  ok   $toy ($(wc -l < build/ref/c.txt) events on the last seed)"; else fail=1; fi
done
[ $fail = 0 ] && echo "all toys match" || echo "MISMATCHES"
exit $fail
