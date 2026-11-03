#!/usr/bin/env python3
"""The reference: run one Witchbox toy from Playbox's own Python, headless,
and print every note it sounds. witchbox1's C port must print the same.

    run_ref.py MODULE SEED [--ms N] [--tick MS] [--shake SEED2] [--knobs a,b,c]
               [--scale N] [--chords N] [--tempo N] [--params]

Output, one event a line, tab separated:
    T  ms  role  degree  voice  hold        a note (voice -1 = None)
    S  ms  what  value                      a conductor changed something
    P  name lo hi value                      with --params: each knob after the seed
The clock is driven in fixed slices of --tick ms (default 5), as the device does.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "playbox"))
sys.setrecursionlimit(10000)

import settings  # noqa: E402
import dice  # noqa: E402
import gfx  # noqa: E402
import page2  # noqa: E402
import page3  # noqa: E402

LOG = []
NOW = [0]


def _heard(role, degree, voice, module=None, seat=0, hold=0):
    LOG.append("T\t%d\t%s\t%d\t%d\t%d" % (NOW[0], role, degree, -1 if voice is None else voice, hold))
    if role == "drums" and voice == 0:
        settings.kicks += 1
    elif role == "lead":
        settings.lead = degree
        settings.leads += 1


settings.heard = _heard


def find(module):
    for n, page in enumerate((page2.TOYS, page3.TOYS)):
        for i, t in enumerate(page):
            if t[3] == module:
                return 40 + n * 20 + i + 1, {"name": t[0], "glyph": t[1], "hue": t[2], "module": t[3],
                                              "toy": t[4], "words": t[5], "role": t[6], "dark": True}
    raise SystemExit("no such toy: " + module)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("seed", type=int)
    ap.add_argument("--ms", type=int, default=20000)
    ap.add_argument("--tick", type=int, default=5)
    ap.add_argument("--shake", type=int, default=None, help="press Random with this seed first")
    ap.add_argument("--knobs", default=None, help="knob values to load, comma separated")
    ap.add_argument("--scale", type=int, default=1)
    ap.add_argument("--chords", type=int, default=1)
    ap.add_argument("--tempo", type=int, default=120)
    ap.add_argument("--params", action="store_true")
    a = ap.parse_args()

    settings.SCALE.value = a.scale
    settings.TEMPO.value = a.tempo
    settings.CHORDS._own = a.chords
    settings.CHORDS.rolled = (0, 4, 5, 3)
    settings.calm()
    settings.kicks = settings.leads = 0
    settings.lead = None
    settings.band = False
    settings.drummer = False

    n, mode = find(a.module)
    cls = getattr(__import__(a.module), mode["toy"])
    toy = cls(gfx.NOWHERE, gfx.NOWHERE, mode, n, seed=a.seed)
    if a.knobs:
        toy.load([int(v) for v in a.knobs.split(",")])
    toy.enter()
    if a.shake is not None:
        toy.handle(("press", "HELP"))        # the help page: nothing drawn
        toy.shake(a.shake)
        toy.handle(("press", "HELP"))
    if a.params:
        for p in toy.own:
            print("P\t%s\t%d\t%d\t%d" % (p.name, p.lo, p.hi, p.value))
        print("P\tScale\t%d\t%d\t%d" % (0, len(settings.SCALES) - 1, settings.SCALE.value))
        print("P\tChords\t%d\t%d\t%d" % (0, settings.CHORDS.hi, settings.CHORDS._own))
        print("P\tRolled\t%d\t%d\t%d" % settings.CHORDS.rolled[1:])
    toy.handle(("press", "PLAY"))
    t = 0
    while t < a.ms:
        toy.tick(a.tick)
        t += a.tick
        NOW[0] = t
        for s in _watch(toy):
            LOG.append(s)
    for s in LOG:
        print(s)


_last = {}


def _watch(toy):
    """Report a conductor's work as it changes."""
    out = []
    cur = {"speed": settings.speed, "drop": settings.drop,
           "cuts": tuple(settings.cuts), "levels": tuple(settings.levels)}
    for k, v in cur.items():
        if _last.get(k) != v:
            _last[k] = v
            out.append("S\t%d\t%s\t%s" % (NOW[0], k, v if not isinstance(v, tuple) else ",".join(str(x) for x in v)))
    return out


if __name__ == "__main__":
    main()
