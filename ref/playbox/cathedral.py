"""Toy 55 (Witchbox, chords): Cathedral.

Slow choir and organ chords. Each bar the chord's notes fade in one after
another, hold for most of the bar, then ring on long after they are let
go, so one chord is still dying away while the next one rises.

For the sound engine: self.fade_ms and self.ring_ms are the fade-in and
ring-out times, and Blend runs from all organ (0) to all choir (10).

Screen 1 is a row of tall white arches, one for each note of the chord
(Height adds one or two more, an octave up). An arch fills with purple
from the floor as its note fades in and drains slowly as it rings out.
The purple is deeper for organ and paler for choir.
"""
from math import sqrt
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_TOP, _FLOOR = 42, 208          # an arch's tip and its foot
_W, _R = 44, 22                 # outer width, and the round top's radius
_T = 4                          # thickness of the stone
_IN = _FLOOR - _TOP - _T        # how tall the inside is
_NOTES = (0, 2, 4, 7, 9)        # root, third, fifth, then root and third again higher
_LET_GO = 10                    # the step of the bar where the chord is released


def _inset(r, i):
    """How far row i of a round top of radius r stands in from the side."""
    return r - int(sqrt(r * r - (r - i - 0.5) ** 2) + 0.5)


class Cathedral(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.blend = Param("Blend", 0, 10, 5, help="From all organ to all choir")
        self.fade = Param("Fade", 1, 10, 4, help="How slowly chords fade in")
        self.ring = Param("Ring", 1, 10, 6, help="How long they ring on")
        self.height = Param("Height", 0, 2, 1, choices=("None", "One", "Two"),
                            help="Adds notes an octave above")
        self._lv = [0] * 5          # each note's loudness, 0..1000
        self._wait = [0] * 5        # ms before each note starts to fade in
        self._px = [0] * 5          # how high each arch is filled on screen
        self._held = False
        self._tint = self._n = None
        self.fade_ms = self.ring_ms = 0
        return (self.blend, self.fade, self.ring, self.height)

    def randomise(self):
        self.blend.value = int(rand() * 11)
        self.fade.value = 1 + int(rand() * 10)
        self.ring.value = 1 + int(rand() * 10)
        self.height.value = int(rand() * 3)

    def _colour(self):
        return theme.HUES[self.hue][1 if self.blend.value < 4 else 0] \
            if self.blend.value < 8 else 0xC57E     # pale purple

    def param_changed(self):
        if self._n != 3 + self.height.value or self._tint != self._colour():
            self.draw_main()

    def _slab(self, x, top, w, r, ya, yb, c):
        """Rows ya..yb of an arch shape (round top at `top`), in colour c."""
        d = self.d1
        for y in range(ya, min(yb, top + r)):       # the curved part, row by row
            n = _inset(r, y - top)
            d.fill_rect(x + n, y, w - 2 * n, 1, c)
        if yb > top + r:                            # the straight part in one go
            a = max(ya, top + r)
            d.fill_rect(x, a, w, yb - a, c)

    def _x(self, k):
        return theme.W // 2 - self._n * 30 + k * 60 + 8

    def draw_main(self):
        self._n = n = 3 + self.height.value
        self._tint = self._colour()
        self.d1.fill_rect(0, _TOP, theme.W, _FLOOR - _TOP, theme.GROUND)
        for k in range(n):
            x = self._x(k)
            self._slab(x, _TOP, _W, _R, _TOP, _FLOOR, theme.WHITE)
            self._slab(x + _T, _TOP + _T, _W - 2 * _T, _R - _T, _TOP + _T, _FLOOR, theme.GROUND)
            self._px[k] = 0
        self._fill()

    def _fill(self):
        """Raise or lower each arch's purple by just the rows that changed."""
        for k in range(self._n):
            px = (self._lv[k] * _IN // 1000) & ~1
            was = self._px[k]
            if px != was:
                x = self._x(k) + _T
                c = self._tint if px > was else theme.GROUND
                self._slab(x, _TOP + _T, _W - 2 * _T, _R - _T,
                           _FLOOR - max(px, was), _FLOOR - min(px, was), c)
                self._px[k] = px

    def play_changed(self):
        self._held = False                          # on pause the chord rings out

    def on_step(self):
        i = self.steps % BAR
        if i == 0:
            self.fade_ms = self.fade.value * 150
            self.ring_ms = self.ring.value * 350
            r = self.root()
            for k in range(self._n):                # the notes enter one after another
                self.note(r + _NOTES[k], k)
                self._wait[k] = k * self.fade_ms // 4
            self._held = True
        elif i == _LET_GO:
            self._held = False

    def frame(self, ms):
        for k in range(self._n):
            if self._held and self._wait[k] <= 0:
                self._lv[k] = min(1000, self._lv[k] + ms * 1000 // (self.fade.value * 150))
            else:
                self._wait[k] -= ms
                self._lv[k] = max(0, self._lv[k] - ms * 1000 // (self.ring.value * 350))
        self._fill()
