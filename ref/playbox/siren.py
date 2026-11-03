"""Toy 72 (Witchbox, lead): Siren.

One long note that glides between notes of the current chord, with a slow
wobble in its pitch. It rests on a note, then slides smoothly to another
chord note within reach. It never jumps: every move is a glide.

Screen 1 draws the pitch as one line, left to right like a heart monitor,
starting again at the left edge when it reaches the right. Higher on the
screen is higher in pitch, and the vibrato shows as a fine ripple. The
newest part of the line is purple, the rest white, and a small gap runs
ahead of the pen to wipe the old line away.
"""
from math import sin
import theme, tune
from settings import Param
from dark import Dark
from toy import rand

_N = 160                        # columns, 2 pixels each
_NEW = 14                       # how many of the newest are purple
_GAP = 5                        # columns wiped ahead of the pen
_LO, _HI = 3, 17                # the notes it keeps between


class Siren(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.glide = Param("Glide", 1, 10, 5, help="How slow each glide is")
        self.vib = Param("Vibrato", 0, 10, 4, help="From none to deep")
        self.reach = Param("Reach", 1, 7, 4, help="How far each move goes, in notes")
        self.hold = Param("Hold", 1, 10, 4, help="How long it rests on each note")
        self._ys = bytearray(_N)    # the line as drawn: each column's top
        self._hs = bytearray(_N)    # and height (0 = wiped)
        self._x = 0                 # the pen's column
        self._a = self._b = 10      # gliding from note a to note b
        self._t = 0                 # ms into the rest or the glide
        self._moving = False
        self._wave = 0.0            # where the vibrato is in its cycle
        self._last = 0              # the last y drawn
        for _ in range(_N - _GAP):  # a line to show before anything has played
            self._pen(30, False)
        return (self.glide, self.vib, self.reach, self.hold)

    def randomise(self):
        self.glide.value = 1 + int(rand() * 10)
        self.vib.value = int(rand() * 11)
        self.reach.value = 1 + int(rand() * 7)
        self.hold.value = 1 + int(rand() * 10)

    def _aim(self, live):
        """Pick the next note: a chord note within reach, else the nearest."""
        r, here, far = self.root(), self._b, self.reach.value
        near = [n for n in tune.tones(r, max(_LO, here - far), min(_HI, here + far)) if n != here]
        if near:
            to = near[int(rand() * len(near))]
        else:
            to = max(_LO, min(_HI, tune.nearest(here, r)))
        self._t = 0
        if to != here:
            self._a, self._b, self._moving = here, to, True
            if live:
                self.note(to)

    def _pen(self, ms, live=True):
        """Move the pitch on by ms and draw one more column of the line."""
        self._t += ms
        if self._moving:
            f = min(1.0, self._t / (self.glide.value * 2 * self.step_ms))
            p = self._a + (self._b - self._a) * f * f * (3 - 2 * f)     # ease out and in
            if f >= 1:
                self._moving = False
                self._t = 0
        else:
            p = self._b
            if self._t >= self.hold.value * 2 * self.step_ms:
                self._aim(live)
        self._wave += ms / 45.0
        y = int(196 - (p - _LO) * 11 + sin(self._wave) * self.vib.value * 0.6)
        x = self._x
        top = min(y, self._last) if x else y        # join up with the column before
        h = abs(y - self._last) + 2 if x else 2
        self._last = y
        self._ys[x] = top
        self._hs[x] = min(h, 250)
        self._x = (x + 1) % _N
        self._hs[(x + _GAP) % _N] = 0
        if live:
            self._col(x, theme.HUES[self.hue][0])
            self._col((x - _NEW) % _N, theme.WHITE)
            self.d1.fill_rect(((x + _GAP) % _N) * 2, tune.TOP + 4, 2, 186, theme.GROUND)

    def _col(self, i, c):
        if self._hs[i]:
            self.d1.fill_rect(i * 2, self._ys[i], 2, self._hs[i], c)

    def draw_main(self):
        self.d1.fill_rect(0, tune.TOP + 4, 320, 186, theme.GROUND)
        for i in range(_N):
            new = (self._x - 1 - i) % _N < _NEW
            self._col(i, theme.HUES[self.hue][0] if new else theme.WHITE)

    def frame(self, ms):
        if self.playing:
            self._pen(ms)

    def play_changed(self):
        if self.playing and not self._moving:
            self.note(self._b)          # the note it is resting on
