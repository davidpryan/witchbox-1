# Building witchbox1

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `witchbox1.bin` | the firmware app |
| `loader/ota.bin` | the update loader (Felucca's, unchanged) |
| `witchbox1.fwsc` | the installable package (app + loader) |

It is witchbox1's build (AMB-1's, Felucca's, by way of X0X), with the names changed. **witchbox1 has not
been built with JieLi's compiler yet** (the machine it was written on could not reach JieLi's package
server), so the first build may need a small fix. What the build checks, and what has been checked without it, is
under "What is and isn't verified" below.

## Prerequisites

- Python 3 with Pillow: `pip3 install Pillow`
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- On Linux x86-64 the toolchain runs natively. On macOS (or anything else) the build runs each tool
  in a `linux/amd64` `debian:bookworm-slim` container: install Docker Desktop (Rosetta on Apple
  silicon) and keep the source tree in a folder Docker can share, e.g. under `/Users`. With podman
  instead of Docker, put a `docker` script that runs `exec podman "$@"` first on your PATH; `EN_JOBS`
  (default 4) limits parallel compiles.
- For the tests: a C compiler; for the browser build, clang with the wasm32 target, `wasm-ld` and
  Node.js (optional).

## Build

```
tests/run_tests.sh              # first: everything that can be tested without the hardware
./build.sh                      # build/witchbox1.fwsc
./build.sh --release 0.1        # a release build: build/witchbox1-0.1.fwsc, identity FM-1_5000100
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations (`~/.jieli/toolchain`,
`~/fw-AC79_AIoT_SDK`). Build option: `EN_CDC=1` adds Felucca's USB serial function (off by default:
one plain MIDI interface).

The build also generates `build/gen/` (the font, and the packed recordings from `ref/samples/`; `pip3 install Pillow` is for the font). It fails if a soft-double routine is linked (a
`double` crept in), if the image, the RAM or the pool is too big, if anything refers to the chip's
ROM, or if a hardware register is touched outside `firmware/hal/`.

After a build, `tests/run_tests.sh` also runs Felucca's update-path tests against the package.

### On GitHub

`.github/workflows/build.yml` does the same on a GitHub runner: push this tree to a repository of
yours, and the "build" workflow's run page offers `witchbox1-firmware` to download (the package and a
folder with the web installer). It has not been run yet either; it downloads the toolchain from
JieLi's server and the SDK from Gitee, so it fails if either is unreachable from GitHub.

## Install

Read "Before you install" in [docs/MANUAL.md](docs/MANUAL.md) first.

From the command line (needs `pip3 install mido python-rtmidi`), with the FM-1 connected by a USB data
cable:

```
python3 tools/fm1_install.py build/witchbox1.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

Or with the web installer, in Chrome or Edge:

```
tools/make_installer.py build/witchbox1.fwsc DEV build/install
python3 -m http.server -d build/install 8000        # then open http://localhost:8000
```

The same page puts M-VAVE's own firmware back ("Back to the stock firmware"). From witchbox1, hold
OCT− and OCT+ for 5 seconds for the update mode. If the FM-1 no longer starts but reaches the chip's
update mode (4C4A:8057 on USB), `tools/fm1_rescue.sh` puts stock firmware back from a Mac; otherwise
recovery needs [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## What is and isn't verified

Verified on a computer (`tests/run_tests.sh`, all passing):

- Every toy against Playbox's own Python (`ref/conform.sh`): the Python reference and the C port
  print every note a toy sounds over twenty seeds, plain and after Random, for forty seconds, and
  the two logs are identical for all forty toys.
- The engine (`tests/host/engine_test.c`): every instrument and every drum sounds, nothing is silent,
  clipped flat or a NaN. It compiles with `-Wall -Wextra -Wdouble-promotion -Werror` (no `double`).
- The whole app in the simulator (`tests/scenarios/*.wb`): the seats, the shelf, the seed, the knobs,
  the hold layers, the pads, mutes and solos, a band of four with a conductor, saving, a reboot,
  another firmware's project in the flash object.
- The browser build (the same C, compiled to WebAssembly), in Node.
- The platform pieces as AMB-1 tests them: the maths, the encoder, USB audio, the TRS jack, storage.

Estimated, not measured:

- **The audio load.** `tools/cpu.sh` counts the instructions the render runs per output sample
  (valgrind) for twenty seconds of a busy band of four (Four Floor, 808 Slide, Coven Saws, Glass
  Arp): about 1330 a sample, which by X0X's measure (29.5 host instructions a sample per 1 % of an
  FM-1) is about 45 %. HAUNT-1's engine counted the same way was 1300 to 1700 and read 28 % on one
  FM-1, so the real figure is likely lower.
- **The image.** The packed recordings are 232 KB of flash; the code is small beside them. The
  build's own checks say whether the image, the RAM and the pool fit.

Not verified at all:

- That JieLi's clang 4.0.1 compiles `wb_engine.c`, the toys and `ui.c` as they are, and that the
  build's own checks pass.
- Anything on the hardware: the sound through the FM-1's converter and speaker, the screen on its
  panel, the real audio load, the lights.

## The files

See the table in [README.md](README.md).
