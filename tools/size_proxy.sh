#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# A flash and RAM estimate without JieLi's toolchain: the device build's own units compiled by clang
# for a 32-bit RISC target of similar code density (rv32imfc, -Os / -O2 as tools/build.py uses), the
# inline pi32 assembly stubbed out, linked with --gc-sections and totted up against app.ld's regions.
# The real figure comes from tools/build.py, which refuses an image that does not fit.
set -e
cd "$(dirname "$0")/.."
S=build/proxy; mkdir -p $S build/gen
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/wb_samples.h ] || python3 tools/gen_samples.py build/gen/wb_samples.h >/dev/null
printf '#define __asm__\n#define asm\n#define volatile(...)\n' > $S/stub.h
F="--target=riscv32-unknown-elf -march=rv32imfc -mabi=ilp32f -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -nostdlib -ffp-contract=off -include $S/stub.h -DEN_VERSION=\"PROXY\" -DFELUCCA_ID=\"FM-1_500\" -Ifirmware/hal -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Wno-unused-function"
clang $F -Os -c firmware/src/main.c -o $S/main.o
clang $F -O2 -c firmware/src/dsp/wb_engine.c -o $S/engine.o
U=$(grep -ho "FM1_ISR [a-z_0-9]*, [a-z_0-9]*" firmware/hal/fm1_isr.S | awk '{print "-u "$3}' | tr '\n' ' ')
ld.lld --gc-sections -e fm1_cstart $U -u fm1_fatal_common -u fm1_fault_c --unresolved-symbols=ignore-all -o $S/app.elf $S/main.o $S/engine.o 2>/dev/null
llvm-objdump -h $S/app.elf | grep -E "^ +[0-9]+ \.(text|rodata|ram_text|data|sdata|bss|sbss|pool|noinit)" |
  awk '{t[$2]=("0x" $3)+0; printf "  %-10s %7d\n", $2, t[$2]}
       END {printf "flash: %d of %d (%d%%)   RAM: %d of %d   pool: %d of %d\n",
            f=t[".text"]+t[".rodata"]+t[".ram_text"]+t[".data"]+t[".sdata"], 581564, f*100/581564,
            t[".data"]+t[".sdata"]+t[".bss"]+t[".sbss"], 98304, t[".pool"], 344064}'
