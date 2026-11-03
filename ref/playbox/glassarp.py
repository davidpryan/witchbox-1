"""Toy 56 (Witchbox, lead): Glass Arp.

A bright bell runs through the notes of the current chord, high up, one
note every step. Some notes are dropped, some jump an octave, and each
note that sounds can come back as one, two or three quieter echoes.

Screen 1 is a row of eight white crystals, in the order they are played;
a taller crystal is a higher note. A crystal lights cyan as it sounds and
a darker cyan for each echo. A small gem floats over a crystal while its
note is leaping an octave.
"""
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_N = 8
_UP = (0, 2, 4, 7, 9, 11, 14, 16)       # chord notes above the root, low to high
_INOUT = (0, 2, 4, 7, 9, 7, 4, 2)
_BASE = tune.BOT - 12                   # where the crystals stand
_LIT_MS = 110
_GAP = 3                                # steps between a note and each echo


class GlassArp(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.shape = Param("Shape", 0, 3, 0, choices=("Up", "Down", "In-out", "Random"),
                           help="The order the notes are played in")
        self.skips = Param("Skips", 0, 10, 2, help="How often a note is dropped")
        self.leap = Param("Leap", 0, 10, 2, help="How often a note jumps an octave")
        self.echo = Param("Echo", 0, 10, 5, help="How many echoes follow each note")
        self._order = list(_UP)         # the eight notes, as steps above the root
        self._was = None                # the shape the order was built for
        self._root = 0
        self._tall = [0] * _N           # each crystal's height on screen
        self._glow = [0] * _N           # 0 white, 1 sounding, 2 echoing
        self._ms = [0] * _N             # how long each stays lit
        self._gem = [False] * _N        # the leap gem is showing
        self._echoes = []               # [ms to go, crystal, note, echoes left]
        return (self.shape, self.skips, self.leap, self.echo)

    def randomise(self):
        self.shape.value = int(rand() * 4)
        self.skips.value = int(rand() * 6)
        self.leap.value = int(rand() * 7)
        self.echo.value = int(rand() * 11)
        self._was = None                # a Random shape is dealt again

    def param_changed(self):
        s = self.shape.value
        if s == self._was:
            return
        self._was = s
        if s == 3:                      # eight notes drawn out of the hat
            self._order = [_UP[int(rand() * _N)] for _ in range(_N)]
        else:
            self._order = list((_UP, tuple(reversed(_UP)), _INOUT)[s])
        self._stand()

    def _height(self, i):
        return 36 + (self._root + self._order[i]) * 6

    def _crystal(self, i):
        """A bar with a stepped point, and the gem above it if leaping."""
        d = self.d1
        x = tune.col(i, _N, 24)
        h = self._tall[i]
        c = (theme.WHITE, theme.HUES[self.hue][0], theme.HUES[self.hue][1])[self._glow[i]]
        top = _BASE - h
        for k in range(3):              # the point: 4, 8, then 12 wide
            d.fill_rect(x - 2 - 2 * k, top + 4 * k, 4 + 4 * k, 4, c)
        d.fill_rect(x - 8, top + 12, 16, h - 12, c)
        if self._glow[i] != 1:
            self._gem[i] = False
        d.fill_rect(x - 3, top - 9, 6, 6, c if self._gem[i] else theme.GROUND)

    def _stand(self):
        """Redraw the crystals whose height has changed."""
        for i in range(_N):
            h = self._height(i)
            if h != self._tall[i]:
                self.d1.fill_rect(tune.col(i, _N, 24) - 8, tune.TOP + 2, 16,
                                  _BASE - tune.TOP - 2, theme.GROUND)
                self._tall[i] = h
                self._crystal(i)

    def draw_main(self):
        self._tall = [0] * _N
        self._stand()

    def _light(self, i, glow):
        self._glow[i] = glow
        self._ms[i] = _LIT_MS
        self._crystal(i)

    def on_step(self):
        r = self.root()
        if r != self._root:             # a new chord: the crystals regrow
            self._root = r
            self._stand()
        if rand() < self.skips.value / 14:
            return
        i = self.steps % _N
        n = 7 + r + self._order[i]
        self._gem[i] = rand() < self.leap.value / 14
        if self._gem[i]:
            n += 7
        self.note(n)
        self._light(i, 1)
        many = (self.echo.value + 2) // 4           # 0 to 3 echoes
        if many and len(self._echoes) < 8:
            self._echoes.append([_GAP * self.step_ms, i, n, many])

    def frame(self, ms):
        for i in range(_N):
            if self._ms[i]:
                self._ms[i] = max(0, self._ms[i] - ms)
                if not self._ms[i]:
                    self._glow[i] = 0
                    self._crystal(i)
        if not self.playing:
            return
        for e in self._echoes:
            e[0] -= ms
            if e[0] <= 0:
                self.note(e[2], 1)                  # voice 1: the quieter echo
                if self._glow[e[1]] != 1:           # a fresh note outshines an echo
                    self._light(e[1], 2)
                e[3] -= 1
                e[0] += _GAP * self.step_ms
        self._echoes = [e for e in self._echoes if e[3] > 0]

    def play_changed(self):
        self._echoes = []
