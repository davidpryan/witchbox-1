# witchbox1 licensing

witchbox1 is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`,
full text in `LICENSE`). Its frame is HAUNT-1's, a fork of AMB-1, which is built on Felucca's platform
layer (through X0X and FoMni), and keeps their licence. If you distribute witchbox1, or firmware
derived from it, you must give your recipients its complete corresponding source under the same
licence.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| Platform: `firmware/hal/`, `firmware/loader/`, `firmware/src/{libc,lcd,gfx,usb,storage,ota,midi_uart}.c`, `tools/` (build, package, install, rescue), `web/fm1*.js` | [Felucca](https://github.com/hugelton/Felucca), Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments, as changed for [X0X](https://github.com/charlesvestal/fm1-x0x), FoMni and [AMB-1](https://github.com/charlesvestal/fm1-amb); here unchanged from HAUNT-1 but for its name (the USB strings, the package identity, the output file names) | GPL-3.0-only |
| `firmware/src/app/{panel,plat_fm1,main_fm1}.c`, `firmware/src/app/plat.h`, `host/wb_host.c`, `web/emu/`, `web/witchbox1_installer.html`, `tests/` (the runner, the platform tests, the emulator test) | AMB-1 by Charles Vestal (itself adapted from X0X and Felucca), with HAUNT-1's and witchbox1's changes: the names, the simulator's checks | GPL-3.0-only |
| `firmware/src/dsp/fastmath.h` | X0X | GPL-3.0-only |
| `firmware/src/app/wb/` (the toys, their dice, settings and sounds), `firmware/src/app/{band,ui,project}.c`, `app.h`, `firmware/src/dsp/wb_engine.{c,h}`, `tools/gen_samples.py`, `tools/cpu.sh`, `ref/run_ref.py`, `ref/run_c.c`, `ref/conform.sh`, `ref/gen_*.py`, `tests/scenarios/`, `tests/host/engine_test.c`, `docs/` | witchbox1: a port of Playbox's Witchbox toys and sound engine to C. Playbox is the author's own project; its Python is in `ref/playbox/` for the conformance test | GPL-3.0-only |

The sound engine's design follows Playbox's `engine.cpp`, which was built on DaisySP (MIT,
Electrosmith) and Mutable Instruments' Plaits models (MIT, Emilie Gillet). witchbox1 contains none of
their code: the oscillators, filter, envelope and drum models are written out afresh in C, in their
simplest form.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| The recordings packed into the engine (crackle, hum, woosh, impact, door, vox hit, coins, ting, wood, paper, ping, glass, tick, pop, snap, skip, crow) | CC0 (freesound.org, via the set bundled with Sonic Pi) | `ref/samples/*.wav`, sources in `ref/samples/ATTRIBUTION.md` |
| Barlow Semi Condensed (The Barlow Project Authors), the UI face | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| Terminus (Dimitar Toshkov Zhekov), an alternative font set | SIL OFL 1.1 | `assets/fonts/ter-u*.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments. "M-VAVE" and "FM-1" are
trademarks of their respective owners. witchbox1 is independent firmware; it is not affiliated with,
endorsed by or supported by any of them, nor by the authors of HAUNT-1, AMB-1, X0X or Felucca.

## Radio

witchbox1 never enables the Bluetooth / Wi-Fi radio of the hardware.
