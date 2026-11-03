"""Toy 49 (Witchbox, bass): Reese.

A growling bass that sits above the sub: two buzzing notes a hair apart,
rubbing against each other. It plays a slow phrase of one, two or three
notes from the current chord across a bar, then waits before the next.

Screen 1 is two thick lines running across the screen that weave over
and under each other. They weave faster as Detune rises, sit closer
together as the tone gets darker, and both turn blue while a note sounds.
"""
from math import sin, pi
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_N = 32                         # each line is this many short pieces
_W = theme.W // _N
_T = 5                          # line thickness
_CY = 123
_SIN = tuple(int(64 * sin(2 * pi * i / 64)) for i in range(64))
_STARTS = ((0,), (0, 8), (0, 6, 12))    # where a phrase's notes fall in its bar
_WAIT = (0, 1, 2, 4)


class Reese(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.detune = Param("Detune", 0, 10, 4, help="From a slow swirl to a rough growl")
        self.phrase = Param("Phrase", 1, 3, 2, help="How many notes in each phrase")
        self.dark = Param("Dark", 0, 10, 5, help="How muffled the tone is")
        self.rest = Param("Rest", 0, 3, 1, choices=("None", "1 bar", "2 bars", "4 bars"),
                          help="How long it waits between phrases")
        self._on = False            # a note is sounding
        self._ph = 0                # how far the weave has run
        self._ys = [[0] * _N, [0] * _N]     # each piece's y on screen, per line
        self._cs = [None, None]     # each line's colour on screen
        self._turn = 0              # the two lines take turns to redraw
        return (self.detune, self.phrase, self.dark, self.rest)

    def randomise(self):
        self.detune.value = int(rand() * 11)
        self.phrase.value = 1 + int(rand() * 3)
        self.dark.value = int(rand() * 11)
        self.rest.value = int(rand() * 4)

    def draw_main(self):
        self._ys = [[0] * _N, [0] * _N]
        self._weave(0)
        self._weave(1)

    def play_changed(self):
        self._on = False

    def on_step(self):
        i = self.steps % BAR
        if self.bar_no() % (1 + _WAIT[self.rest.value]):
            self._on = False                    # a bar of waiting
        elif i in _STARTS[self.phrase.value - 1]:
            if i == 0:
                deg = tune.nearest(self.root(), self.root())
            else:
                t = tune.tones(self.root(), -3, 4)
                deg = t[int(rand() * len(t))]
            self.note(deg, 0)
            self._on = True

    def frame(self, ms):
        if self.playing:
            self._ph += ms * (1 + self.detune.value)
        self._turn ^= 1
        self._weave(self._turn)

    def _weave(self, k):
        """Line k: move only the pieces that are not where they should be.
        The other line is the same curve upside down, so the two cross."""
        d = self.d1
        c = theme.HUES[self.hue][0] if self._on and self.playing else theme.WHITE
        fresh = c != self._cs[k]
        self._cs[k] = c
        ys = self._ys[k]
        other = self._ys[1 - k]
        amp = 44 - self.dark.value * 3
        if k:
            amp = -amp
        ph = self._ph // 50
        for i in range(_N):
            y = _CY + amp * _SIN[(i * 3 + ph) & 63] // 64
            was = ys[i]
            if y == was and not fresh:
                continue
            x = i * _W
            if was and was != y:
                d.fill_rect(x, was - 2, _W, _T, theme.GROUND)
                o = other[i]
                if o and abs(o - was) < _T:         # rubbing out nicked the other line
                    d.fill_rect(x, o - 2, _W, _T, self._cs[1 - k])
            d.fill_rect(x, y - 2, _W, _T, c)
            ys[i] = y
