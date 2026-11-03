"""Toy 61 (Witchbox, drums): Four Floor.

A straight dance beat. The kick lands on every beat, an open hat on
every off-beat and a clap on beats 2 and 4. Closed hats can fill in the
sixteenths. Now and then the kick sits out the last beat of a four-bar
phrase (Drop-outs).

Screen 1 is four large white squares in a row, one per beat. A square
turns red on its kick; the harder the Drive, the more dark bands crush
it. The thin bar above a square lights on the off-beat hat. Squares 2
and 4 gain a white outline on the clap. The row of small ticks below
shows the closed hats. A kick that sits out leaves its square grey.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_Y, _S = 92, 60                     # top and size of the squares
# The Hats setting at which each step of the bar gains a closed hat
# (0 = never: the open hat lives there).
_NEED = (9, 2, 0, 5, 10, 3, 0, 7, 9, 1, 0, 6, 10, 4, 0, 8)


def _x(b):
    return 19 + b * 74


class FourFloor(Dark):
    def make_params(self):
        self.drive = Param("Drive", 0, 10, 4, help="How crushed the kick is")
        self.hats = Param("Hats", 0, 10, 5, help="How busy the closed hats are")
        self.clap = Param("Clap", 0, 10, 7, help="How loud the clap is; 0 is off")
        self.dropouts = Param("Drop-outs", 0, 10, 3, help="How often the kick skips a phrase end")
        self._sq = [0] * 4          # each square: 0 white, 1 kicked, 2 sat out
        self._ring = -1             # the square wearing the clap outline
        self._bar_on = -1           # the lit off-beat bar
        self._tick = -1             # the lit closed-hat tick
        return (self.drive, self.hats, self.clap, self.dropouts)

    def randomise(self):
        self.drive.value = int(rand() * 11)
        self.hats.value = int(rand() * 11)
        self.clap.value = int(rand() * 11)
        self.dropouts.value = int(rand() * 11)

    def param_changed(self):
        for i in range(BAR):        # the ticks show which closed hats are in
            self._hat(i)

    def _square(self, b):
        d = self.d1
        s = self._sq[b]
        d.fill_rect(_x(b), _Y, _S, _S, (theme.WHITE, theme.HUES[self.hue][0], theme.SAND)[s])
        if s == 1:                  # drive: dark bands across the red
            n = self.drive.value // 2
            for k in range(n):
                d.fill_rect(_x(b), _Y + (2 * k + 1) * _S // (2 * n) - 2, _S, 3,
                            theme.HUES[self.hue][1])

    def _outline(self, b, on):
        gfx.ring(self.d1, _x(b), _Y, _S, _S, theme.WHITE if on else theme.GROUND)

    def _top(self, b, on):
        self.d1.fill_rect(_x(b), _Y - 16, _S, 5, theme.HUES[self.hue][0] if on else theme.SAND)

    def _hat(self, i):
        d = self.d1
        x = _x(i // 4) + (i % 4) * 16 + 3
        d.fill_rect(x, _Y + _S + 14, 6, 12, theme.GROUND)
        if i % 4 == 2:
            return
        if i == self._tick:
            d.fill_rect(x, _Y + _S + 14, 6, 12, theme.WHITE)
        else:
            on = self.hats.value >= _NEED[i]
            d.fill_rect(x, _Y + _S + 14, 6, 12 if on else 4, theme.SOFT if on else theme.SAND)

    def draw_main(self):
        for b in range(4):
            self._square(b)
            self._top(b, b == self._bar_on)
            if b == self._ring:
                self._outline(b, True)
        for i in range(BAR):
            self._hat(i)

    def _settle(self):
        """Put back whatever was lit."""
        for b in range(4):
            if self._sq[b]:
                self._sq[b] = 0
                self._square(b)
        if self._ring >= 0:
            self._outline(self._ring, False)
            self._ring = -1
        if self._bar_on >= 0:
            self._top(self._bar_on, False)
            self._bar_on = -1
        was, self._tick = self._tick, -1
        if was >= 0:
            self._hat(was)

    def play_changed(self):
        self._settle()

    def on_step(self):
        i = self.steps % BAR
        b = i // 4
        if i % 2 == 0:
            self._settle()
        else:
            was, self._tick = self._tick, -1
            if was >= 0:
                self._hat(was)
        if i % 4 == 0:
            out = b == 3 and self.bar_no() % 4 == 3 and rand() < self.dropouts.value / 10
            if not out:
                self.kick()
            self._sq[b] = 2 if out else 1
            self._square(b)
            if b % 2 and self.clap.value:
                self.note(0, 1)                     # the clap
                self._ring = b
                self._outline(b, True)
        elif i % 4 == 2:
            self.note(0, 3)                         # the open hat
            self._bar_on = b
            self._top(b, True)
        if _NEED[i] and self.hats.value >= _NEED[i]:
            self.note(0, 2)                         # a closed hat
            self._tick = i
            self._hat(i)
