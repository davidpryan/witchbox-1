"""Toy 66 (Witchbox, chords): Rave Stab.

Short chord stabs in a jumpy pattern, like an old rave sample played on a
keyboard. The chord keeps one fixed shape and is moved whole: to the root
of the current chord, and a little up or down for some of the off-beat
stabs, the way a sampled chord is played as a riff.

Screen 1 is the bar as a row of sixteen steps. Each stab is a white block
of stacked bars, one per note, placed higher or lower by its pitch and
wider the longer it is held. A block flashes red as it plays.
"""
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_SHAPES = ((0, 2, 4), (0, 2, 4, 6), (0, 3, 4))      # minor, minor 7, suspended
_ON = (0, 8, 4, 12, 2, 10, 6, 14)       # where stabs go on the beat, first to last
_OFF = (3, 11, 7, 15, 9, 1, 13, 5)      # and off it
_LIFT = (0, 0, 0, 2, 0, 0, 2, 4, 0, 0, 0, 2, 0, 4, 2, -1)     # the riff: each step's move
_X0, _CELL = 16, 18


class Rave(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.hits = Param("Hits", 1, 8, 5, help="How many stabs are in a bar")
        self.push = Param("Push", 0, 8, 2, help="How many stabs land off the beat")
        self.shape = Param("Shape", 0, 2, 0, choices=("Minor", "Minor 7", "Sus"),
                           help="The chord's shape, which never changes")
        self.hold = Param("Length", 1, 10, 4, help="From a clipped stab to a held one")
        self._root = 0
        self._stab = [False] * BAR      # the pattern
        self._drawn = [False] * BAR     # which steps have a block on screen
        self._hot = -1                  # the block that is red
        self._ms = 0                    # how long it stays red
        self._tick = -1                 # the step marker on screen
        self._lay()
        return (self.hits, self.push, self.shape, self.hold)

    def randomise(self):
        self.hits.value = 2 + int(rand() * 7)
        self.push.value = int(rand() * 5)
        self.shape.value = int(rand() * 3)
        self.hold.value = 1 + int(rand() * 10)

    def _lay(self):
        """The pattern: so many off the beat, the rest on it."""
        off = min(self.push.value, self.hits.value)
        for s in range(BAR):
            self._stab[s] = False
        for s in _ON[:self.hits.value - off] + _OFF[:off]:
            self._stab[s] = True

    def param_changed(self):
        self._lay()
        self.draw_main()

    def _block(self, s, c):
        """Step s's stab: one bar per note of the chord."""
        y = 172 - (self._root + _LIFT[s]) * 9
        w = 6 + self.hold.value
        for o in _SHAPES[self.shape.value]:
            self.d1.fill_rect(_X0 + s * _CELL, y - o * 8, w, 5, c)

    def _mark(self, s, on):
        c = theme.HUES[self.hue][0] if on else theme.SOFT if s % 4 == 0 else theme.SAND
        self.d1.fill_rect(_X0 + s * _CELL, tune.BOT - 14, 16, 4, c)

    def draw_main(self):
        d = self.d1
        for s in range(BAR):
            if self._drawn[s] or self._stab[s]:
                d.fill_rect(_X0 + s * _CELL, tune.TOP + 4, 16, 168, theme.GROUND)
            self._drawn[s] = self._stab[s]
            if self._stab[s]:
                self._block(s, theme.HUES[self.hue][0] if s == self._hot else theme.WHITE)
            self._mark(s, s == self._tick)

    def _cool(self):
        if self._hot >= 0:
            s = self._hot
            self._hot = -1
            if self._stab[s]:
                self._block(s, theme.WHITE)

    def on_step(self):
        s = self.steps % BAR
        r = self.root()
        if r != self._root:             # a new chord: every block moves with it
            self._root = r
            self.draw_main()
        if self._tick >= 0:
            self._mark(self._tick, False)
        self._tick = s
        self._mark(s, True)
        if self._stab[s]:
            self._cool()
            self._hot = s
            self._ms = self.step_ms * (1 + self.hold.value) // 3
            self._block(s, theme.HUES[self.hue][0])
            for v, o in enumerate(_SHAPES[self.shape.value]):
                self.note(r + _LIFT[s] + o, v)

    def frame(self, ms):
        if self._hot >= 0:
            self._ms -= ms
            if self._ms <= 0:
                self._cool()

    def play_changed(self):
        if not self.playing:
            self._cool()
            if self._tick >= 0:
                self._mark(self._tick, False)
                self._tick = -1
