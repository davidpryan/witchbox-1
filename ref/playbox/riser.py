"""Toy 65 (Witchbox, texture): Riser.

Build-up and release. Every 4, 8 or 16 bars a phrase ends; before it
does, a sweep of noise or a climbing note (or both) rises into the bar
line, and an impact lands on it. With Impact at 10 a falling sweep
follows.

Screen 1 is a ramp of grey slats, low on the left and tall on the
right. It fills with white from left to right as the riser climbs, and
the last quarter fills orange. On the impact a frame bursts outwards
from the ramp and fades, and the ramp empties again.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_EVERY = (4, 8, 16)
_STEPS = (4, 8, 16, 32, 64)         # riser lengths, in steps
_N = 60                             # slats in the ramp
_X0, _BASE = 40, 188                # left end and foot of the ramp
_HOT = 45                           # slats from here on fill orange
_GROW = (6, 12, 18, 24)             # how far out the burst frame is, frame by frame


class Riser(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.every = Param("Every", 0, 2, 0, choices=("4 bars", "8 bars", "16 bars"),
                           help="How long a phrase is")
        self.span = Param("Length", 0, 4, 2,
                          choices=("1 beat", "2 beats", "1 bar", "2 bars", "4 bars"),
                          help="How long the riser lasts")
        self.kind = Param("Type", 0, 2, 2, choices=("Noise", "Pitch", "Both"),
                          help="What rises: noise, a note or both")
        self.impact = Param("Impact", 0, 10, 7, help="How hard it lands; 10 adds a falling sweep")
        self._full = 0              # slats filled on screen
        self._ms = -1               # ms since the riser began; -1 = not rising
        self._drain = 0             # slats to empty each frame after the impact
        self._burst = 0             # ms since the impact, while the frame shows
        self._edge = None           # the burst frame as drawn: (size, colour)
        self._last = None           # the last climbing note sent
        return (self.every, self.span, self.kind, self.impact)

    def randomise(self):
        self.every.value = int(rand() * 3)
        self.span.value = int(rand() * 5)
        self.kind.value = int(rand() * 3)
        self.impact.value = int(rand() * 11)

    def _slat(self, k, full):
        h = 4 + 116 * (k + 1) // _N
        c = theme.SAND
        if full:
            c = theme.HUES[self.hue][0] if k >= _HOT else theme.WHITE
        self.d1.fill_rect(_X0 + 4 * k, _BASE - h, 3, h, c)

    def _frame(self, now):
        """The burst: a square-cornered frame round the ramp, standing in
        for a ring, which would cost hundreds of writes at this size."""
        was = self._edge
        if now == was:
            return
        d = self.d1
        for e in (was, now):
            if e:
                m, c = e[0], theme.GROUND if e is was else e[1]
                x, y, w, h = _X0 - m, _BASE - 120 - m, 4 * _N + 2 * m, 120 + 2 * m
                d.fill_rect(x, y, w, 3, c)
                d.fill_rect(x, y + h - 3, w, 3, c)
                d.fill_rect(x, y, 3, h, c)
                d.fill_rect(x + w - 3, y, 3, h, c)
        self._edge = now

    def draw_main(self):
        for k in range(_N):
            self._slat(k, k < self._full)
        e, self._edge = self._edge, None
        self._frame(e)

    def _reset(self):
        self._ms = -1
        self._drain = self._burst = 0
        self._frame(None)
        while self._full:
            self._full -= 1
            self._slat(self._full, False)

    def param_changed(self):
        self._reset()

    def play_changed(self):
        self._reset()

    def on_step(self):
        phrase = _EVERY[self.every.value] * BAR
        at = self.bar_no() % _EVERY[self.every.value] * BAR + self.now() % BAR
        length = _STEPS[self.span.value]
        if at == 0 and self._ms >= 0:               # the phrase ends: impact
            self._ms = -1
            hit = self.impact.value
            if hit:
                self.note(self.root(), 2)
                self._burst = 1
            if hit == 10:
                self.note(self.root() + 14, 3)      # the falling sweep
            self._drain = 3 if hit == 10 else 12    # a sweep empties the ramp slowly
        if at == phrase - length:                   # time to start climbing
            self._ms = 0
            self._drain = 0
            self._last = None
            if self.kind.value != 1:
                self.note(0, 0, length * self.step_ms)      # the noise sweep, set going once, for the whole climb
        if self._ms >= 0 and self.kind.value:       # the note climbs two octaves
            rise = self.root(1) + 14 * (length - (phrase - at) % phrase) // length
            if rise != self._last:
                self._last = rise
                self.note(rise, 1)

    def frame(self, ms):
        if not self.playing:
            return
        if self._ms >= 0:                           # fill the ramp as far as the riser has got
            self._ms += ms
            upto = min(_N, _N * self._ms // (_STEPS[self.span.value] * self.step_ms))
            while self._full < upto:
                self._slat(self._full, True)
                self._full += 1
        elif self._drain:
            for _ in range(min(self._drain, self._full)):
                self._full -= 1
                self._slat(self._full, False)
            if not self._full:
                self._drain = 0
        if self._burst:                             # the frame grows, dims and goes
            self._burst += ms
            k = self._burst // (60 + self.impact.value * 8)
            if k >= 4:
                self._burst = 0
                self._frame(None)
            else:
                c = (theme.HUES[self.hue][0], theme.WHITE, theme.SOFT, theme.SAND)[k]
                self._frame((_GROW[k], c))
