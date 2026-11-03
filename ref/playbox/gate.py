"""Toy 53 (Witchbox, chords): Stutter Gate.

The current chord chopped into a rhythm. A loop of 1 to 32 steps has its
hits spread as evenly as they can be (the same sum as Euclidean Rhythm,
toy 1). On every hit the chord sounds for a moment; Smear sets how long.

Screen 1 is a row of white shutters, one per step. A small cyan tab under
a shutter marks a hit. On a hit the shutter snaps open and shows cyan for
as long as the chord sounds through it. A yellow mark walks along below.
"""
import theme, tune
from settings import Param
from dark import Dark
from toy import rand

_TOP, _H = 58, 116              # the shutters
_TAB = _TOP + _H + 6            # the hit tabs under them


def pattern(hits, steps):
    """True for each step that is a hit; step 0 is a hit whenever hits > 0.
    (Copied from euclid.py.)"""
    return [(i * hits) % steps < hits for i in range(steps)]


class Gate(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.hits = Param("Hits", 0, 16, 7, help="How many steps let the chord through")
        self.count = Param("Steps", 1, 32, 16, help="How long the loop is, 1 to 32")
        self.rotate = Param("Rotate", 0, 15, 0, help="Shifts the rhythm round the loop")
        self.smear = Param("Smear", 1, 10, 4, help="How long each chop rings on")
        self._pat = pattern(7, 16)
        self._open = -1             # the shutter open now
        self._ms = 0                # and how long it stays open
        self._head = -1             # where the yellow mark is drawn
        return (self.hits, self.count, self.rotate, self.smear)

    def length(self):
        return self.count.value

    def randomise(self):
        self.count.value = 2 + int(rand() * 31)
        self.hits.value = 1 + int(rand() * self.count.value)
        self.rotate.value = int(rand() * self.count.value)
        self.smear.value = 1 + int(rand() * 10)

    def param_changed(self):
        n = self.count.value
        self.hits.hi = n                        # no more hits than steps
        self.hits.value = min(self.hits.value, n)
        self.rotate.hi = n - 1
        self.rotate.value = min(self.rotate.value, n - 1)
        new = pattern(self.hits.value, n)
        r = self.rotate.value
        new = new[n - r:] + new[:n - r]         # turned round by Rotate
        if new != self._pat:
            self._pat = new
            self.draw_main()

    def _x(self, i):
        """Left edge and width of shutter i."""
        n = len(self._pat)
        pitch = min(40, 304 // n)
        return theme.W // 2 - n * pitch // 2 + i * pitch + 1, pitch - 2

    def _shutter(self, i, opened):
        x, w = self._x(i)
        if opened:                              # the slats part and the chord shows
            self.d1.fill_rect(x, _TOP + 8, w, _H - 16, theme.HUES[self.hue][0])
        else:
            self.d1.fill_rect(x, _TOP, w, _H, theme.WHITE)

    def draw_main(self):
        d = self.d1
        d.fill_rect(0, _TOP, theme.W, tune.BOT - _TOP, theme.GROUND)
        self._open = self._head = -1
        for i, hit in enumerate(self._pat):
            self._shutter(i, False)
            if hit:
                x, w = self._x(i)
                d.fill_rect(x, _TAB, w, 5, theme.HUES[self.hue][0])

    def _mark(self, i):
        """Move the yellow mark to step i (-1 = none)."""
        for k, on in ((self._head, False), (i, True)):
            if k >= 0:
                x, w = self._x(k)
                tune.marker(self.d1, x + w // 2, w, on)
        self._head = i

    def _shut(self):
        if self._open >= 0:
            self._shutter(self._open, False)
            self._open = -1

    def play_changed(self):
        if not self.playing:
            self._shut()
            self._mark(-1)

    def on_step(self):
        i = self.pos % len(self._pat)
        self._mark(i)
        if self._pat[i]:
            r = self.root()
            for v in range(3):                  # the chord, all at once
                self.note(r + 2 * v, v)
            self._shut()
            self._open = i
            self._ms = self.step_ms * (1 + self.smear.value) // 4
            self._shutter(i, True)

    def frame(self, ms):
        if self._open >= 0:
            self._ms -= ms
            if self._ms <= 0:
                self._shut()
