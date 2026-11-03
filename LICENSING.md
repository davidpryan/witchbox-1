# witchbox1 licensing

witchbox1 is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`,
full text in `LICENSE`). Its frame is HAUNT-1's and AMB-1's. The platform under it, fm1-base on felucca-bsp
(Felucca's layer, through X0X and FoMni), is MPL-2.0 and stays in its own tree (`base/`). If you distribute witchbox1, or firmware
derived from it, you must give your recipients its complete corresponding source under the same
licence.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| `base/` (fm1-base, a git submodule, with felucca-bsp in `base/bsp/`): the platform (chip layer, screen, panel, audio, USB, storage, the update loader, safe mode), the host simulator, the browser emulator, the build, package, install and site tools, m-bootloader's package | [fm1-base](https://github.com/charlesvestal/fm1-base) by Charles Vestal and [felucca-bsp](https://github.com/hugelton/felucca-bsp) by Leo Kuroshita (@kurogedelic), Hügelton Instruments; m-bootloader by Julius Vitkauskas (MIT). Not part of this tree: see their own LICENSING | MPL-2.0 (m-bootloader MIT) |
| `tests/run_tests.sh`, `build.sh`, `app/host.c`, `app/app.c` | from fm1-base's template and AMB-1 (Charles Vestal, MPL-2.0), changed for witchbox1 | GPL-3.0-only |
| `app/wb/` (the toys, their dice, settings and sounds), `app/{band,ui,project,app,host}.c`, `app.h`, `app/dsp/wb_engine.{c,h}`, `app/tools/gen_samples.py`, `tools/cpu.sh`, `ref/run_ref.py`, `ref/run_c.c`, `ref/conform.sh`, `ref/gen_*.py`, `tests/scenarios/`, `tests/host/engine_test.c`, `docs/` | witchbox1: a port of Playbox's Witchbox toys and sound engine to C. Playbox is the author's own project; its Python is in `ref/playbox/` for the conformance test | GPL-3.0-only |

The sound engine's design follows Playbox's `engine.cpp`, which was built on DaisySP (MIT,
Electrosmith) and Mutable Instruments' Plaits models (MIT, Emilie Gillet). witchbox1 contains none of
their code: the oscillators, filter, envelope and drum models are written out afresh in C, in their
simplest form.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| The recordings packed into the engine (crackle, hum, woosh, impact, door, vox hit, coins, ting, wood, paper, ping, glass, tick, pop, snap, skip, crow) | CC0 (freesound.org, via the set bundled with Sonic Pi) | `app/samples/*.wav`, sources in `app/samples/ATTRIBUTION.md` |
| Barlow Semi Condensed (The Barlow Project Authors), the UI face | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| Terminus (Dimitar Toshkov Zhekov), an alternative font set | SIL OFL 1.1 | `assets/fonts/ter-u*.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments. "M-VAVE" and "FM-1" are
trademarks of their respective owners. witchbox1 is independent firmware; it is not affiliated with,
endorsed by or supported by any of them, nor by the authors of HAUNT-1, AMB-1, X0X or Felucca.

## Radio

witchbox1 never enables the Bluetooth / Wi-Fi radio of the hardware.
