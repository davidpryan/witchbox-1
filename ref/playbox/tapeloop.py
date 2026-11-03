"""Toy 68 (Witchbox, chords): Tape Loop.

A short loop of chords on worn tape. The loop is 3, 5, 6 or 7 beats long,
so it never lines up with the bar and drifts against the beat. Every pass
wears it a little: duller and more wobbly. After a set number of passes a
fresh loop is recorded from the chords of that moment.

Screen 1 is two white reels with a line of tape hanging between them. The
blocks on the tape are the loop's chords, and a cyan marker travels along
above it; a block lights as the marker reaches it. The tape line turns
fainter and wavier as the loop wears, and comes back white when renewed.
"""
import theme, gfx, fonts, tune
from settings import Param
from dark import Dark
from toy import rand

_BEATS = (3, 5, 6, 7)
_X0, _X1, _Y = 60, 260, 168             # the tape line
_RX, _RY, _RR = (59, 270), 84, 26       # the reels
_SPOKE = ((0, -14), (10, -10), (14, 0), (10, 10), (0, 14), (-10, 10), (-14, 0), (-10, -10))
_INK = (theme.WHITE, theme.SOFT, theme.SAND)


class TapeLoop(Dark):
    def make_params(self):
        self.loop = Param("Loop", 0, 3, 1, choices=("3 beats", "5 beats", "6 beats", "7 beats"),
                          help="How long the loop is")
        self.wear = Param("Wear", 1, 10, 5, help="How fast the loop wears out")
        self.wobble = Param("Wobble", 0, 10, 5, help="How much the pitch wavers")
        self.renew = Param("Renew", 4, 32, 8, help="Passes before a fresh loop")
        self._hits = [0, None, 0, None, None, None, None]   # each beat's chord, or None
        self._age = 0                   # passes since the loop was fresh
        self._look = None               # the tape as drawn: beats, level, wave
        self._mx = -1                   # marker x on screen
        self._lit = -1                  # the block that is lit
        self._turn = 0                  # where the reels' holes are
        self._said = None               # the pass count as written
        return (self.loop, self.wear, self.wobble, self.renew)

    def randomise(self):
        self.loop.value = int(rand() * 4)
        self.wear.value = 1 + int(rand() * 10)
        self.wobble.value = int(rand() * 11)
        self.renew.value = 4 + int(rand() * 13)

    def _state(self):
        level = min(2, self._age * self.wear.value // 10)
        return (self.loop.value, level, level * self.wobble.value * 3 // 10)

    def param_changed(self):
        if self._look and self.loop.value != self._look[0]:
            self._record()              # a different length: a new loop
        if self._state() != self._look:
            self._tape()
        self._count()

    def _record(self):
        """A fresh loop: the chord now, then the next one, on some of the beats."""
        n = _BEATS[self.loop.value]
        for b in range(7):
            r = self.root() if b * 2 < n else self.root(1)
            self._hits[b] = r if b < n and (b == 0 or rand() < 0.55) else None
        self._age = 0

    def _block(self, b, lit):
        if self._hits[b] is not None and b < _BEATS[self.loop.value]:
            x = _X0 + b * (_X1 - _X0) // _BEATS[self.loop.value]
            c = theme.HUES[self.hue][0] if lit else _INK[self._look[1]]
            self.d1.fill_rect(x, _Y - 8, 6, 18, c)

    def _tape(self):
        """Redraw the tape line: fainter and more crooked the more it is worn."""
        d = self.d1
        self._look = self._state()
        _, level, wave = self._look
        d.fill_rect(_X0, _Y - 9, _X1 - _X0 + 10, 20, theme.GROUND)
        for i in range(21):             # short pieces, up and down in turn
            d.fill_rect(_X0 + i * 10, _Y + (wave if i % 2 else -wave), 10, 2, _INK[level])
        for b in range(7):
            self._block(b, b == self._lit)

    def _count(self):
        d = self.d1
        s = "Pass %d of %d" % (self._age % self.renew.value + 1, self.renew.value)
        if s != self._said:
            self._said = s
            d.fill_rect(110, tune.BOT - 22, 100, 14, theme.GROUND)
            gfx.text_centre(d, 160, tune.BOT - 15, s, fonts.LABEL, theme.SOFT, theme.GROUND)

    def _holes(self, c):
        dx, dy = _SPOKE[self._turn % 8]
        for x in _RX:
            self.d1.fill_rect(x + dx - 3, _RY + dy - 3, 6, 6, c)
            self.d1.fill_rect(x - dx - 3, _RY - dy - 3, 6, 6, c)

    def draw_main(self):
        d = self.d1
        for x in _RX:
            gfx.disc(d, x, _RY, _RR, theme.WHITE)
            gfx.disc(d, x, _RY, 5, theme.GROUND)
            d.fill_rect(x - 1 if x < 160 else x, _RY + _RR, 2, _Y + 2 - _RY - _RR, theme.SOFT)
        self._holes(theme.GROUND)
        self._tape()
        self._said = None
        self._count()
        self._mark(self._mx)

    def _mark(self, x):
        d = self.d1
        if self._mx >= 0:
            d.fill_rect(self._mx, _Y - 22, 6, 10, theme.GROUND)
        self._mx = x
        if x >= 0:
            d.fill_rect(x, _Y - 22, 6, 10, theme.HUES[self.hue][0])

    def on_step(self):
        n = _BEATS[self.loop.value] * 4
        at = self.steps % n
        if at == 0:                     # a new pass: older, or renewed
            self._age += 1
            if self.steps == 0 or self._age >= self.renew.value:
                self._record()
                self._look = None
            if self._state() != self._look:     # only when the wear shows
                self._tape()
            self._count()
        if at % 2 == 0:                 # the reels turn
            self._holes(theme.WHITE)
            self._turn += 1
            self._holes(theme.GROUND)
        self._mark(_X0 + at * (_X1 - _X0) // n)
        if self._lit >= 0:
            b, self._lit = self._lit, -1
            self._block(b, False)
        if at % 4 == 0 and self._hits[at // 4] is not None:
            self._lit = at // 4
            self._block(self._lit, True)
            for v in range(3):
                self.note(self._hits[self._lit] + 2 * v, v)

    def play_changed(self):
        if not self.playing:
            self._mark(-1)
            if self._lit >= 0:
                b, self._lit = self._lit, -1
                self._block(b, False)
