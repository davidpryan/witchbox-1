"""Toy 47 (Witchbox, bass): 808 Slide.

A booming bass. It plays where the kick plays: if a drum toy is in the
band it follows that toy's kicks, and on its own it keeps a kick pattern
of its own. Notes come from the current chord, some glide from the note
before, and the last note of the bar may fall an octave.

For the sound engine: self.glide_ms is how long the newest note takes to
glide in (0 = no glide) and self.boom_ms is how long it rings.

Screen 1 is drawn like an oscilloscope. A pen crosses the screen once a
bar, wiping a little way ahead of itself. A note is a level run, a glide
is a curve between runs. The newest note is red, older ones white.
"""
import theme, tune
import settings
from settings import Param, BAR
from dark import Dark
from toy import rand

_W = theme.W
_OWN = (0, 6, 10)               # where it plays when there is no drummer
_GAP = 10                       # columns wiped ahead of the pen


class Slide808(Dark):
    FRAME_MS = 25

    def make_params(self):
        self.glide = Param("Glide", 0, 10, 5, help="How long a glide takes, instant to slow")
        self.slides = Param("Slides", 0, 10, 5, help="How often a note glides to the next")
        self.decay = Param("Decay", 1, 10, 6, help="From a short punch to a long boom")
        self.drops = Param("Drops", 0, 10, 5, help="How often the bar ends an octave down")
        # The trace: y of each column, 0 = nothing. It starts as a level line.
        self._ys = bytearray([tune.height(-7, -14, 0, tune.BOT - 24, tune.TOP + 24)] * _W)
        self._x = 0                 # the pen
        self._t = 0                 # ms into the bar
        self._n0 = self._nn = 0     # the newest note: first column, columns so far
        self._y = 0                 # the pen's height now (0 = silent)
        self._y0 = self._y1 = 0     # a glide runs from y0 to y1
        self._gt = 0                # ms into the glide
        self._ring = 0              # ms the note still sounds
        self._last = -7
        self.glide_ms = 0
        self.boom_ms = 0
        return (self.glide, self.slides, self.decay, self.drops)

    def randomise(self):
        self.glide.value = int(rand() * 11)
        self.slides.value = int(rand() * 11)
        self.decay.value = 1 + int(rand() * 10)
        self.drops.value = int(rand() * 11)

    def draw_main(self):
        self._runs(0, _W, theme.WHITE)
        self._runs(self._n0, self._nn, theme.HUES[self.hue][0])

    def play_changed(self):
        if self.playing:                    # the pen starts again from the left
            self._x = self._t = 0
            self._wipe(0, _GAP)
        self._ring = self._y = 0

    def _runs(self, x0, n, c):
        """Draw n columns of the trace from x0 (wrapping round), each level
        stretch as one rectangle, with a riser where the height changes."""
        d = self.d1
        ys = self._ys
        a = w = 0
        py = ys[x0 - 1] if x0 else 0
        for k in range(n + 1):
            x = (x0 + k) % _W
            y = ys[x] if k < n else 0
            if w and (y != py or x == 0):
                d.fill_rect(a, py - 2, w, 4, c)
                w = 0
            if y:
                if not w:
                    a = x
                w += 1
                if py and py != y and x:
                    d.fill_rect(x, min(py, y) - 2, 1, abs(py - y) + 4, c)
            py = y

    def _wipe(self, x, n):
        n = min(n, _W - x)
        self.d1.fill_rect(x, tune.TOP, n, tune.BOT - tune.TOP, theme.GROUND)
        for k in range(x, x + n):
            self._ys[k] = 0

    def _play(self, deg, slide):
        self._runs(self._n0, self._nn, theme.WHITE)     # the last note is old now
        self._n0, self._nn = self._x, 0
        self._last = deg
        y = tune.height(deg, -14, 0, tune.BOT - 24, tune.TOP + 24)
        self.glide_ms = self.glide.value * 40 if slide and self._y else 0
        self._y0 = self._y if self.glide_ms else y
        self._y1 = y
        self._gt = 0
        self._ring = self.boom_ms = 100 + self.decay.value * 110
        self.note(deg, 0)

    def _pick(self):
        t = tune.tones(self.root(), -7, 0)
        self._play(t[int(rand() * len(t))], rand() < self.slides.value / 10)

    def on_step(self):
        i = self.steps % BAR
        self._t = i * self.step_ms                      # keep the pen in time
        if i == 0:
            self._x = 0
        if i == 14:                                     # the bar's last note
            if rand() < self.drops.value / 10:
                self._play(max(-14, self._last - 7), True)      # it falls
            elif not settings.drummer:
                self._pick()
        elif not settings.drummer and i in _OWN:
            if i == 0:
                self._play(self.root() - 7, rand() < self.slides.value / 10)
            else:
                self._pick()

    def frame(self, ms):
        if not self.playing:
            return
        if settings.drummer and self.new_kick():        # play where the kick plays
            self._pick()
        was = self._y
        self._ring = max(0, self._ring - ms)
        self._gt += ms
        if not self._ring:
            self._y = 0
        elif self._gt < self.glide_ms:                  # ease from one note to the next
            f = self._gt * 256 // self.glide_ms
            f = f * f * (768 - 2 * f) >> 16
            self._y = self._y0 + ((self._y1 - self._y0) * f >> 8)
        else:
            self._y = self._y1
        self._t += ms
        to = min(_W - 1, self._t * _W // (BAR * self.step_ms))
        n = (to - self._x) % _W
        if n == 0 or n > _W // 2:                       # not moved, or a hair ahead of the clock
            return
        x0 = self._x
        if x0 + n + _GAP > _W:                          # wipe ahead, round the edge too
            self._wipe(0, (x0 + n + _GAP) % _W)
        self._wipe(x0, n + _GAP)
        for k in range(n):                              # each new column gets its own height
            y = self._y
            if y and was:
                y = was + (y - was) * (k + 1) // n
            self._ys[(x0 + k) % _W] = y
        self._runs(x0, n, theme.HUES[self.hue][0])
        self._nn = min(_W, self._nn + n)
        self._x = to
