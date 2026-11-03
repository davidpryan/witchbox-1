"""Toy 43 (Witchbox, drums): Slow Stomp.

A half-speed witch house beat. A long kick lands on beat 1, a huge clap
with a long tail on beat 3, and now and then the kick doubles. Every few
bars the end of the bar fills with extra kicks.

Screen 1 is a white disc that swells on the kick. On the clap a purple
square frame opens round it and closes in over the length of the tail.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_CX, _CY = 160, 123
_REST = 26                      # the disc at rest
_RING = (56, 64, 72, 80)        # frame size as the tail runs out, last to first


class Stomp(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.weight = Param("Weight", 1, 10, 6, help="How long the kick booms")
        self.tail = Param("Clap tail", 0, 10, 6, help="How long the clap rings on")
        self.doubles = Param("Doubles", 0, 10, 3, help="How often the kick hits twice")
        self.fill = Param("Fill every", 0, 1, 0, choices=("4 bars", "8 bars"),
                          help="How often the bar ends with extra kicks")
        self._boom = 0              # ms of kick swell left
        self._ring = 0              # ms of clap tail left
        self._r = None              # disc radius on screen
        self._ringr = 0             # ring radius on screen (0 = none)
        return (self.weight, self.tail, self.doubles, self.fill)

    def randomise(self):
        self.weight.value = 1 + int(rand() * 10)
        self.tail.value = int(rand() * 11)
        self.doubles.value = int(rand() * 11)
        self.fill.value = int(rand() * 2)

    def draw_main(self):
        self._r = None
        self._ringr = 0
        self._disc()

    def _disc(self):
        """Draw the ring, then the disc, only if either changed size."""
        d = self.d1
        r = _REST + (16 * self._boom) // self._boom_ms() if self._boom else _REST
        if self._ring:
            ring = _RING[min(3, self._ring * 4 // self._ring_ms())]
        else:
            ring = 0
        if ring != self._ringr:
            if self._ringr:
                self._frame(self._ringr, theme.GROUND)
            if ring:
                self._frame(ring, theme.HUES[self.hue][0])
            self._ringr = ring
        if r != self._r:
            if self._r and r < self._r:
                gfx.disc(d, _CX, _CY, self._r, theme.GROUND)
            gfx.disc(d, _CX, _CY, r, theme.WHITE)
            self._r = r

    def _frame(self, r, c):
        """A square frame r from the centre: four writes, where a round
        ring this size would take a couple of hundred."""
        d = self.d1
        d.fill_rect(_CX - r, _CY - r, 2 * r, 4, c)
        d.fill_rect(_CX - r, _CY + r - 4, 2 * r, 4, c)
        d.fill_rect(_CX - r, _CY - r, 4, 2 * r, c)
        d.fill_rect(_CX + r - 4, _CY - r, 4, 2 * r, c)

    def _boom_ms(self):
        return 120 + self.weight.value * 70

    def _ring_ms(self):
        return 150 + self.tail.value * 160

    def on_step(self):
        i = self.steps % BAR
        every = 4 if self.fill.value == 0 else 8
        filling = self.bar_no() % every == every - 1 and i >= 12
        if i == 0 or (i == 6 and rand() < self.doubles.value / 12) or (filling and i % 2 == 0):
            self.kick()
            self._boom = self._boom_ms()
        if i == 8:
            self.note(0, 1)                     # the clap
            self._ring = self._ring_ms()

    def frame(self, ms):
        if self._boom:
            self._boom = max(0, self._boom - ms)
        if self._ring:
            self._ring = max(0, self._ring - ms)
        self._disc()
