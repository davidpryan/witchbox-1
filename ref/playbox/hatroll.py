"""Toy 42 (Witchbox, drums): Hat Roll.

Steady hi-hats, two to the beat, that break into a fast roll on the last
beat of some bars. Roll rate picks how fast the roll is. Ramp makes the
roll fall or rise in pitch and loudness as it goes. Open hat lets some
off-beat hats ring open.

Screen 1 is a string of white beads, one per hat, read left to right
across the bar. The bead that sounds turns purple. An open hat is a
hollow bead. A roll is a tight cluster of smaller beads at the right
end, which turn purple one by one as they play and climb or sink with
the ramp.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_Y = 123
_COUNT = (4, 6, 8)                  # hats in a one-beat roll
_BASE = (3, 3, 2)                   # middle bead size of each kind of roll
_SWING = (2, 2, 1)                  # how much the ramp grows or shrinks them
_TAIL_X = 232                       # where the last beat starts on screen


def _x(slot):
    return 34 + slot * 36


class HatRoll(Dark):
    FRAME_MS = 20

    def make_params(self):
        self.chance = Param("Roll chance", 0, 10, 5, help="How often a bar ends in a roll")
        self.rate = Param("Roll rate", 0, 2, 1, choices=("16ths", "Triplets", "32nds"),
                          help="How fast the roll is")
        self.ramp = Param("Ramp", -5, 5, 3, help="Roll falls (below 0) or rises (above 0)")
        self.open = Param("Open hat", 0, 10, 3, help="How often an off-beat hat rings open")
        self._rolling = True        # does this bar end in a roll?
        self._opens = [False] * 8   # which of the eight hats ring open
        self._shown = [None] * 8    # each bead as drawn: (open, lit)
        self._lit = -1              # the bead that is purple now
        self._k = self._n = 0       # roll hats played, and how many in the roll
        self._wait = 0              # ms to the next roll hat
        return (self.chance, self.rate, self.ramp, self.open)

    def randomise(self):
        self.chance.value = int(rand() * 11)
        self.rate.value = int(rand() * 3)
        self.ramp.value = int(rand() * 11) - 5
        self.open.value = int(rand() * 11)

    def param_changed(self):
        self._k = self._n = 0       # a roll in flight would no longer match the picture
        self._tail()

    def _bead(self, slot, lit=False):
        """One of the eight steady hats; drawn only if it has changed."""
        now = (self._opens[slot], lit)
        if self._shown[slot] == now:
            return
        self._shown[slot] = now
        d = self.d1
        gfx.disc(d, _x(slot), _Y, 6, theme.HUES[self.hue][0] if lit else theme.WHITE)
        if now[0]:
            gfx.disc(d, _x(slot), _Y, 3, theme.GROUND)

    def _roll_bead(self, k, lit):
        """Hat k of the roll. The ramp tilts the cluster and sizes the beads."""
        n = _COUNT[self.rate.value]
        lean = self.ramp.value * (2 * k - (n - 1))      # -5(n-1) .. 5(n-1)
        r = _BASE[self.rate.value] + lean * _SWING[self.rate.value] // (5 * (n - 1))
        x = _TAIL_X + 6 + k * 72 // n
        y = _Y - lean * 4 // (n - 1)
        gfx.disc(self.d1, x, y, r, theme.HUES[self.hue][0] if lit else theme.WHITE)

    def _tail(self):
        """Redraw the last beat: two steady beads, or the roll's cluster."""
        d = self.d1
        d.fill_rect(_TAIL_X - 6, _Y - 28, 88, 57, theme.GROUND)
        d.fill_rect(_TAIL_X - 6, _Y, 88, 1, theme.SAND)
        self._shown[6] = self._shown[7] = None
        if self._rolling:
            for k in range(_COUNT[self.rate.value]):
                self._roll_bead(k, False)
        else:
            self._bead(6)
            self._bead(7)

    def draw_main(self):
        self.d1.fill_rect(12, _Y, 296, 1, theme.SAND)       # the string
        self._shown = [None] * 8
        for slot in range(6):
            self._bead(slot, slot == self._lit)
        self._tail()

    def play_changed(self):
        if self._n:                 # tidy a roll that was cut short
            self._tail()
        self._k = self._n = 0
        if self._lit >= 0 and not (self._rolling and self._lit > 5):
            self._bead(self._lit)
        self._lit = -1

    def on_step(self):
        i = self.steps % BAR
        if i == 0:                  # plan the bar: a roll or not, and the open hats
            had = self._rolling
            self._rolling = rand() < self.chance.value / 10
            for slot in (1, 3, 5, 7):
                self._opens[slot] = rand() < self.open.value / 12
            if had or self._rolling:
                self._tail()
                self._lit = -1
            else:
                self._bead(7)
        elif i == 1:                # a step later, to spread the drawing out
            for slot in (1, 3, 5):
                self._bead(slot)
        if i == 12 and self._rolling:
            if self._lit >= 0:
                self._bead(self._lit)
                self._lit = -1
            self._k = 0             # the roll itself is played in frame()
            self._n = _COUNT[self.rate.value]
            self._wait = 0
        elif i % 2 == 0 and not (self._rolling and i > 12):
            slot = i // 2
            if self._lit >= 0:
                self._bead(self._lit)
            self._lit = slot
            self._bead(slot, True)
            self.note(0, 3 if self._opens[slot] else 2)     # 3 open hat, 2 closed

    def frame(self, ms):
        if not self.playing or self._k >= self._n:
            return
        self._wait -= ms
        while self._wait <= 0 and self._k < self._n:
            k = self._k
            self._roll_bead(k, True)                # it stays purple to the bar's end
            self.note(self.ramp.value * k // (self._n - 1), 2)     # pitch follows the ramp
            self._k += 1
            self._wait += 4 * self.step_ms // self._n
