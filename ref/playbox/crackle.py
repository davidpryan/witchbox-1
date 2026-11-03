"""Toy 69 (Witchbox, texture): Crackle.

A bed of record noise with no notes in it: vinyl pops (Crackle), rain
hiss (Rain), a low rumble (Rumble), and now and then the record skips
(Wear).

Screen 1 is a white record with a purple label. A small mark near the
rim, and another on the label, go round to show it turning. Specks of
dust flash purple on the record at each pop. On a skip the record jerks
back a little. Rain falls as short streaks at the sides, and the bar
underneath trembles with the rumble.
"""
from math import sin, cos
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_CX, _CY = 160, 118
_R = 60                             # the record
_TURN = 1800                        # ms for one turn
_POP_MS = 120                       # how long a speck shows


class Crackle(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.pops = Param("Crackle", 0, 10, 5, help="How many pops")
        self.rain = Param("Rain", 0, 10, 4, help="How loud the hiss is")
        self.rumble = Param("Rumble", 0, 10, 4, help="How loud the low noise is")
        self.wear = Param("Wear", 0, 10, 3, help="How often the record skips")
        self._ang = 0               # how far round the record is, in ms of turning
        self._marks = None          # the two marks as drawn: (x, y, x, y)
        self._specks = []           # dust showing now: [x, y, ms left]
        self._drops = [None] * 10   # rain streaks on screen: (x, y)
        self._drop = 0              # the streak to renew next
        self._low = 0               # the rumble bar's width on screen
        return (self.pops, self.rain, self.rumble, self.wear)

    def randomise(self):
        self.pops.value = int(rand() * 11)
        self.rain.value = int(rand() * 11)
        self.rumble.value = int(rand() * 11)
        self.wear.value = int(rand() * 11)

    def _turn(self):
        """Move the two marks round; the record itself is never redrawn."""
        d = self.d1
        a = self._ang * 6.2832 / _TURN
        s, c = sin(a), cos(a)
        now = (_CX + int(50 * s), _CY - int(50 * c), _CX + int(11 * s), _CY - int(11 * c))
        was = self._marks
        if now == was:
            return
        if was:
            d.fill_rect(was[0] - 2, was[1] - 2, 5, 5, theme.WHITE)
            d.fill_rect(was[2] - 1, was[3] - 1, 3, 3, theme.HUES[self.hue][0])
        d.fill_rect(now[0] - 2, now[1] - 2, 5, 5, theme.GROUND)
        d.fill_rect(now[2] - 1, now[3] - 1, 3, 3, theme.WHITE)
        self._marks = now

    def _bar(self):
        """The rumble: a bar under the record whose length trembles."""
        w = 0
        if self.rumble.value:
            w = 20 + self.rumble.value * 14
            if self.playing:
                w += int(rand() * (4 + 2 * self.rumble.value))
        d = self.d1
        if w < self._low:
            d.fill_rect(_CX - self._low // 2, tune.BOT - 14, self._low, 4, theme.GROUND)
        if w:
            d.fill_rect(_CX - w // 2, tune.BOT - 14, w, 4, theme.SOFT)
        self._low = w

    def draw_main(self):
        d = self.d1
        gfx.disc(d, _CX, _CY, _R, theme.WHITE)                  # drawn once, here only
        gfx.disc(d, _CX, _CY, 40, theme.HUES["white"][1])       # one groove
        gfx.disc(d, _CX, _CY, 38, theme.WHITE)
        gfx.disc(d, _CX, _CY, 18, theme.HUES[self.hue][0])      # the label
        gfx.disc(d, _CX, _CY, 2, theme.GROUND)
        self._marks = None
        self._turn()
        self._low = 0
        self._bar()
        for p in self._drops:
            if p:
                d.fill_rect(p[0], p[1], 2, 7, theme.SOFT)
        for p in self._specks:
            d.fill_rect(p[0], p[1], 3, 3, theme.HUES[self.hue][0])

    def param_changed(self):
        self._bar()

    def play_changed(self):
        self._bar()

    def on_step(self):
        i = self.steps % BAR
        if i == 0:                  # the hiss and rumble are beds: set going each bar
            if self.rain.value:
                self.note(0, 1)
            if self.rumble.value:
                self.note(0, 2)
        if i % 4 == 0 and rand() < self.wear.value / 30:
            self.note(0, 3)         # a skip: the needle jumps back
            self._ang -= _TURN // 8

    def frame(self, ms):
        d = self.d1
        for p in self._specks:
            p[2] -= ms
        while self._specks and self._specks[0][2] <= 0:
            p = self._specks.pop(0)
            d.fill_rect(p[0], p[1], 3, 3, theme.WHITE)
        j = self._drop = (self._drop + 1) % 10      # one rain streak a frame
        p = self._drops[j]
        if p:
            d.fill_rect(p[0], p[1], 2, 7, theme.GROUND)
            self._drops[j] = None
        if not self.playing:
            return
        self._ang = (self._ang + ms) % _TURN
        self._turn()
        if j < self.rain.value:
            x = 16 + int(rand() * 66) + (206 if rand() < 0.5 else 0)
            p = self._drops[j] = (x, tune.TOP + 8 + int(rand() * 150))
            d.fill_rect(p[0], p[1], 2, 7, theme.SOFT)
        if j % 3 == 0:
            self._bar()
        if len(self._specks) < 6 and rand() * 400 < self.pops.value * ms:
            self.note(0, 0)         # a pop
            a = rand() * 6.2832
            r = 43 + int(rand() * 14)           # between the groove and the rim
            p = [_CX + int(r * sin(a)) - 1, _CY - int(r * cos(a)) - 1, _POP_MS]
            d.fill_rect(p[0], p[1], 3, 3, theme.HUES[self.hue][0])
            self._specks.append(p)
