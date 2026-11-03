"""Toy 67 (Witchbox, chords): Swell.

Chords that fade in like a sound played backwards, then cut dead on the
first beat of a bar. Each swell plays the chord that is about to arrive.
Tail lets the cut chord come back as one to three short, fading echoes.

Screen 1 is a wedge that grows from a point on the left to full height on
the right over the length of the swell, pink while it grows. Its outline
shows the Curve: a steady rise, or flat and then a late rush. It vanishes
on the downbeat; echoes blink at its right-hand end. Paused, the whole
wedge is shown in white.
"""
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_LEN = (4, 8, 16)               # steps
_EVERY = (1, 2, 4)              # bars
_COLS, _W = 56, 5
_X0, _CY, _HALF = 20, 123, 78   # left edge, middle line, half the full height


class Swell(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.size = Param("Length", 0, 2, 1, choices=("1/4 bar", "1/2 bar", "1 bar"),
                          help="How long each swell takes")
        self.every = Param("Every", 0, 2, 0, choices=("1 bar", "2 bars", "4 bars"),
                           help="How often a swell comes")
        self.curve = Param("Curve", 0, 10, 4, help="From a steady rise to a late rush")
        self.tail = Param("Tail", 0, 10, 3, help="From a dead cut to a short echo")
        self._all = 0               # ms this swell takes; 0 = no swell
        self._t = 0                 # ms into it
        self._cols = 0              # columns of the wedge on screen
        self._chord = 0             # the chord being swelled
        self._echo = 0              # echoes still to come
        self._wait = 0              # ms until the next
        self._blip = 0              # height of the echo mark on screen
        self._shape = None          # the curve as drawn while paused
        return (self.size, self.every, self.curve, self.tail)

    def randomise(self):
        self.size.value = int(rand() * 3)
        self.every.value = int(rand() * 3)
        self.curve.value = int(rand() * 11)
        self.tail.value = int(rand() * 11)

    def param_changed(self):
        if not self.playing and self.curve.value != self._shape:
            self.draw_main()

    def _column(self, i, c):
        """Column i of the wedge. The curve bends how its height climbs."""
        f = ((i + 1) / _COLS) ** (1 + self.curve.value * 0.35)
        h = max(1, int(_HALF * f))
        self.d1.fill_rect(_X0 + i * _W, _CY - h, _W, 2 * h, c)

    def _wipe(self):
        d = self.d1
        d.fill_rect(_X0, _CY - _HALF, _COLS * _W, 2 * _HALF, theme.GROUND)
        d.fill_rect(_X0, _CY - 1, _COLS * _W, 2, theme.SAND)        # the middle line
        self._cols = self._blip = 0

    def draw_main(self):
        self._wipe()
        if self.playing:
            n = self._t * _COLS // self._all if self._all else 0
            c = theme.HUES[self.hue][0]
        else:
            n, c = _COLS, theme.WHITE
            self._shape = self.curve.value
        for i in range(n):
            self._column(i, c)
        self._cols = n

    def on_step(self):
        period = _EVERY[self.every.value] * BAR
        at = (self.bar_no() * BAR + self.steps % BAR) % period
        if at == 0 and self._all:           # the downbeat: cut dead
            self._all = 0
            self._wipe()
            self._echo = (self.tail.value + 2) // 4
            self._wait = self.step_ms * 3 // 2
        if at == period - _LEN[self.size.value]:
            self._chord = self.root(1)      # the chord that is coming
            self._all = _LEN[self.size.value] * self.step_ms
            self._t = self._echo = 0
            if self._blip:
                self._wipe()
            for v in range(3):
                self.note(self._chord + 2 * v, v)

    def frame(self, ms):
        if not self.playing:
            return
        if self._all:                       # grow: add the columns that are due
            self._t = min(self._all, self._t + ms)
            n = min(_COLS, self._t * _COLS // self._all)
            while self._cols < n:
                self._column(self._cols, theme.HUES[self.hue][0])
                self._cols += 1
        elif self._echo or self._blip:
            self._wait -= ms
            if self._wait <= 0:
                self._mark(0)
                if self._echo:              # each echo is smaller than the last
                    self._echo -= 1
                    self.note(self._chord, 3)
                    self._mark(_HALF * (self._echo + 1) // 4)
                    self._wait += self.step_ms * 3 // 2

    def _mark(self, h):
        """The echo's blink at the wedge's right end; 0 takes it away."""
        x = _X0 + (_COLS - 3) * _W
        if self._blip:
            self.d1.fill_rect(x, _CY - self._blip, 3 * _W, 2 * self._blip, theme.GROUND)
            self.d1.fill_rect(x, _CY - 1, 3 * _W, 2, theme.SAND)
        if h:
            self.d1.fill_rect(x, _CY - h, 3 * _W, 2 * h, theme.WHITE)
        self._blip = h

    def play_changed(self):
        self._all = self._t = self._echo = 0
        self.draw_main()
