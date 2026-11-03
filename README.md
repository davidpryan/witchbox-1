# witchbox1

The eighty Witchbox toys of Playbox, as firmware for the M-VAVE FM-1. Playbox is a hand-built
generative music instrument on a Raspberry Pi Pico 2; its Witchbox shelf is witch house and dark
electronics: limping garage beats, sub bass, detuned saws, glass arpeggios, screwed voices, record
crackle, and conductors that cut the band or stop the tape. witchbox1 puts those toys on the FM-1's
four knobs, fourteen buttons and twenty-seven keys, eight at a time, with one screen.

Installing is at your own risk: the web installer's "Return to official V15", or M-VAVE's updater,
takes you back.

- [docs/MANUAL.md](app/docs/MANUAL.md): the panel, the keys, the screen, the toys, saving, rescue
- [LICENSING.md](LICENSING.md): where the code comes from

## What it is

- **Eighty toys** on four shelf pages: Witch I and II are pages three and four of Playbox's shelf,
  ported note for note; Witch III is the twenty toys Playbox v0.12 added (Heartbeat, Chopped Break,
  Acid Drip, Vespers, Graveyard Guitar, Cursed Cartridge, Storm, Toll Bell ...), made again from
  their descriptions; Witch IV is twenty of witchbox1's own, one classic generative method each
  (a Turing-machine shift register, Markov chains, Wolfram's automata, Euclidean rhythms, the
  logistic map, L-systems, a canon, a fractal motif, the Fibonacci word, 1/f noise, Poisson bursts,
  a random walk, a dying echo chain, lots drawn for who plays, the key moving phase by phase). Each
  has its four knobs and a four-digit seed; the same seed and knobs play the same notes every time.
- **A band of eight**: eight seats on one clock and one run of chords, with a mixer, mutes and
  solos, and seven conductors (Blackout, Screw, Arc, Fog, Dub Out, Culling, Moon) seated like any toy.
- **Arrangements**: a Tape (a bar grid, a lane per seat, 4 to 64 bars) or a Clock (each seat on a
  Euclidean pattern of bars) says who plays when; the Band page shows the singers (where the
  chords, the lead and the kick are coming from); REC held fills a band or an arrangement at random.
- **The FM-1's panel as Playbox never had it**: the four knobs are the toy's four, always; the
  shelf, the seed, the global strip, the effects, the mixer and a slow drift on the knobs live under
  the buttons; the keys are pads: steer the chord root, mute and solo, dice, half and double time,
  freeze, brake, filter, crush, drive, echo.
- **Pictures**: every toy has its own moving picture on the Toy page — Playbox's own for the forty
  ported toys, and one for each toy's mechanism for the forty new — and the band page's members are
  ghosts that sing.
- **Sounds**: Playbox's own sound engine written out in C: its instruments (analog, FM, plucked
  string, organ, formant voice, pitched 808), drums and kits, and the CC0 recordings it used, packed
  to fit. Each toy plays the sounds Playbox gave it.
- **Saving**: sixteen toy slots, four band slots (arrangement included), and the band as you left it.

## What is and isn't verified

- **Every Witch I and II toy against Playbox's Python.** `ref/` holds the Playbox source the port
  was made from (its v0.9 toys, with the seeds of v0.12 wired in the way that version's transcript
  describes) and a harness that runs a toy headless and prints every note. `ref/conform.sh` runs
  each of those forty toys in Python and in C over twenty seeds, plain and after Random, for forty
  seconds, and the two note logs are identical. (If you have a later `playbox.zip`, drop its toys
  into `ref/playbox/` and run the sweep again: that is how to confirm the device's version still
  matches.)
- **Witch III and IV are new code.** The v0.12 source of the Witch III toys was not to hand; only
  their names, descriptions and knob help survived (in the v0.12 image's strings), and the toys here
  are written to those. Witch IV is witchbox1's own. All forty play, are deterministic per seed, sit
  at the same levels as the ported toys, and bands of them sound sane (`tests/scenarios/witch3.wb`,
  `witch4.wb`), but no Pico plays the same notes.
- **The engine** plays every instrument and every drum on a computer without silence, overload or
  a bad number (`tests/host/engine_test.c`); the mix was balanced by measurement, not by ear.
  Expect to want level changes.
- **The whole app** runs in a simulator and in the browser from the same source the device build
  uses: the panel, the pages, the layers, the pads, saving and restarting, bands of eight with Tape
  and Clock arrangements (`tests/scenarios/`), and all of it passes. The audio load of a busy band of four counts at about
  45 % of an FM-1 by host instructions (`tools/cpu.sh`), near what HAUNT-1 measured as 28 % on a
  device.
- **The device build.** The GitHub workflow builds witchbox1 with JieLi's own compiler (clang 4.0.1
  for pi32v2) and the build's checks pass: no soft-double routine linked, nothing refers to the chip's
  ROM, no register touched outside `firmware/hal/`, and the image is 460 KB of the 582 KB slot, RAM
  27 KB of 96 KB, the pool 108 KB of 336 KB. Felucca's update-path tests run against the package.
- **The platform is fm1-base, current and in use on hardware.** Under the app is
  [fm1-base](https://github.com/charlesvestal/fm1-base) at a pinned commit (with felucca-bsp inside),
  the layer AMB-1, CTL-1, Radio and others run on; its boot records where a start failed and shows
  it in safe mode, its installer offers m-bootloader (a recovery menu that works even when a
  firmware will not start), and its update path is the one those firmwares update through. An
  earlier witchbox1 carried a private snapshot of that layer taken from HAUNT-1; an interrupted
  update under that snapshot left an FM-1 needing a hardware recovery, which is why this one does
  not.
- The deepest stack frame is under 500 bytes against stacks of 24 KB and 8 KB, with no
  variable-length frames anywhere; the project record (1.3 KB) fits its 3.8 KB flash object, and
  the build refuses one that does not. A busy band of eight counts at about 49 % of the audio
  budget by host instructions (`tools/cpu.sh`), and an overrun only drops a block, it does not stop.
- **Not verified**: witchbox1 itself has not run on an FM-1. The sound through the FM-1's converter
  and speaker, the screen on its panel, the real audio load and the lights are unseen.

## Building and installing

witchbox1 is an app on [fm1-base](https://github.com/charlesvestal/fm1-base) (`base/`, a git submodule,
with felucca-bsp inside it): the platform layer (screen, panel, audio, USB, storage, the update loader,
safe mode, m-bootloader, backup), the host simulator, the browser emulator and the build and install
tools are its. Clone with `--recursive`, then:

```
pip3 install Pillow
base/tools/get_toolchain.sh            # JieLi's compiler, to ~/.jieli
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
tests/run_tests.sh                     # everything that can be tested without the hardware
./build.sh --release 0.4               # app/build/witchbox1-0.4.fwsc
python3 base/tools/make_site.py app app/build/witchbox1-0.4.fwsc 0.4 app/build/site   # the web installer
```

The toolchain runs on Linux x86-64; on a Mac, fm1-base's build runs it in a container. Or let GitHub
build it: the `build` workflow runs on every push and leaves `witchbox1-firmware` (the package and
the site with the installer) on the run's page. Install from the site's `install/` page in Chrome or
Edge, or `python3 base/tools/fm1_install.py app/build/witchbox1-0.4.fwsc`. Read "Before you
install" in the manual first: it says why to take m-bootloader with it.

## For developers

| Path | What |
| --- | --- |
| `app/wb/` | the toys: `dice.c` (Playbox's PRNG), `settings.c` (scales, chords, what toys share), `toy.c` (knobs, clock, restart, Random), `sound.c` (which sound each toy plays), `toys/*.c` (one file a toy, `toys/all.c` lists them) |
| `app/band.c`, `ui.c`, `project.c`, `app.h` | the band, the panel and screen, what is saved |
| `app/app.c`, `host.c`, `fm1.toml` | fm1-base's contract: the app's entry points, its host-simulator checks, its build settings |
| `app/dsp/wb_engine.{c,h}` | the sound engine (its own unit, -O2) |
| `app/tools/gen_samples.py`, `app/samples/` | the CC0 recordings, packed into the engine's header at build time |
| `base/` | fm1-base and felucca-bsp: the platform, simulator, emulator, build and install tools |
| `ref/` | the Playbox reference and the conformance harness (`run_ref.py`, `run_c.c`, `conform.sh`) |
| `tests/` | `run_tests.sh` runs everything; `scenarios/*.wb` are whole-app tests in fm1-base's simulator |

How a toy is ported: the Python rolled `rand()` (a 16-bit fraction) in a handful of shapes, and each
has an exact integer twin (`wb/dice.h`); every sum is done in the same order on the same integers,
Python's floor division included. Where Playbox computed something in floating point that the
device cannot (Snare Rush's roll, Screw's waver), the values are baked into tables
(`ref/gen_rush.py`, `ref/gen_screw.py`). No `double` anywhere in the firmware.

## Credits and licence

witchbox1 is the Witchbox shelf of Playbox, ported to C. Its frame is HAUNT-1's, a fork of
[AMB-1](https://github.com/charlesvestal/fm1-amb) by Charles Vestal, on the platform of
[Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments), by way of
[X0X](https://github.com/charlesvestal/fm1-x0x) and FoMni-1. The sound engine's design is Playbox's
(itself built on DaisySP and Mutable Instruments' models, MIT; written out afresh here). The
recordings are CC0, from the set bundled with Sonic Pi (`ref/samples/ATTRIBUTION.md`).

GPL-3.0-only. See [LICENSING.md](LICENSING.md).
