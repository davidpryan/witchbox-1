"""Toy 75 (Witchbox, voice): Stutter Chop.

One voice note fired again and again in a fast burst. Each repeat can step
on through the notes of the chord. A burst starts every half bar when the
last one is over. Swell makes each repeat fade in, like a sound played
backwards.

Screen 1 is a white wave cut into twelve slices. The slice being played
jumps up and turns orange (dark orange first, while it fades in). Between
bursts a few slices swap places.
"""
import theme, tune
from settings import Param
from dark import Dark
from toy import rand

_N = 12
_WAVE = (30, 62, 96, 124, 104, 70, 44, 82, 116, 90, 52, 26)    # slice heights
_X0, _PITCH, _SW = 30, 22, 18
_MID = 130                      # the wave's centre line
_JUMP = 14                      # how far the played slice jumps


class Chop(Dark):
    FRAME_MS = 20

    def make_params(self):
        self.rate = Param("Rate", 0, 1, 1, choices=("16ths", "32nds"),
                          help="How fast the repeats come")
        self.burst = Param("Burst", 2, 16, 8, help="How many repeats in a burst")
        self.move = Param("Step", 0, 4, 1, help="How far the pitch moves each repeat")
        self.swell = Param("Swell", 0, 10, 4, help="How much each repeat fades in")
        self._order = list(range(_N))   # which piece of the wave sits in each place
        self._left = 0              # repeats still to come in this burst
        self._count = 0             # repeats done in this burst
        self._wait = 0              # ms until the next repeat
        self._lit = -1              # the slice that is up, or -1
        self._fade = 0              # ms until the lit slice is at full strength
        self._full = True
        self._tones = (0,)
        self._from = 0
        return (self.rate, self.burst, self.move, self.swell)

    def randomise(self):
        self.rate.value = int(rand() * 2)
        self.burst.value = 2 + int(rand() * 15)
        self.move.value = int(rand() * 5)
        self.swell.value = int(rand() * 11)

    def _slice(self, i):
        d = self.d1
        x = _X0 + i * _PITCH
        h = _WAVE[self._order[i]]
        d.fill_rect(x, _MID - 62 - _JUMP, _SW, 124 + _JUMP, theme.GROUND)
        if i == self._lit:
            c = theme.HUES[self.hue][0 if self._full else 1]
            d.fill_rect(x, _MID - h // 2 - _JUMP, _SW, h, c)
        else:
            d.fill_rect(x, _MID - h // 2, _SW, h, theme.WHITE)

    def draw_main(self):
        for i in range(_N):
            self._slice(i)

    def _light(self, i):
        was = self._lit
        self._lit = i
        if was >= 0 and was != i:
            self._slice(was)
        if i >= 0:
            self._slice(i)

    def play_changed(self):
        if not self.playing:
            self._left = 0
            self._light(-1)

    def on_step(self):
        if self._left or self.steps % 8:
            return
        if self._lit >= 0:                      # between bursts: put the slice
            self._light(-1)                     # down and swap a few around
        for k in range(2):
            a, b = int(rand() * _N), int(rand() * _N)
            if a != b:
                o = self._order
                o[a], o[b] = o[b], o[a]
                self._slice(a)
                self._slice(b)
        self._tones = tune.tones(self.root(), 0, 14)
        self._from = int(rand() * _N)
        self._left = self.burst.value
        self._count = 0
        self._wait = 0

    def frame(self, ms):
        if not self.playing:
            return
        if self._fade:                          # the repeat reaches full strength
            self._fade = max(0, self._fade - ms)
            if not self._fade:
                self._full = True
                self._slice(self._lit)
        if not self._left:
            return
        gap = self.step_ms // (2 if self.rate.value else 1)     # 32nds are half a step
        self._wait -= ms
        while self._left and self._wait <= 0:
            k = self._count
            t = self._tones
            self.note(t[(k * self.move.value) % len(t)], 0)
            self._fade = gap * self.swell.value // 12
            self._full = not self._fade
            self._light((self._from + k) % _N)
            self._count += 1
            self._left -= 1
            self._wait += gap
