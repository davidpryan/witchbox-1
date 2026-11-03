"""Toy 41 (Witchbox, drums): Two-Step.

A limping garage beat. The kick lands on beat 1 and once more somewhere
odd later in the bar (Skip). A rim snare lands on beats 2 and 4, hats
tick between them and quiet ghost hits fill the gaps. Every hit sits a
little early or late, by an amount that stays the same from bar to bar,
so the beat limps the same way each time round.

Screen 1 is sixteen thin white lines, one per step of the bar. Each hit
is a dot beside its line: left of the line is early, right is late.
Kicks are blue (bottom row), snares white (middle), hats small grey dots
(top), ghosts tiny. A dot swells for a moment when it sounds.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_KICK, _SNARE, _HAT, _GHOST = 1, 2, 3, 4
_ROW = (0, 172, 124, 76, 100)       # y of each kind's row
_SIZE = (0, 5, 4, 2, 1)             # dot radius of each kind
_PITCH = 18                         # pixels from one line to the next
_LEAD = 6                           # every hit waits this many pixels' worth of
                                    # time, so that an early one can come sooner
_FLASH = 110                        # ms a dot stays swollen


def _x(i):
    return 24 + i * _PITCH


class TwoStep(Dark):
    FRAME_MS = 20

    def make_params(self):
        self.skip = Param("Skip", 7, 12, 11, help="The step the second kick lands on")
        self.shuffle = Param("Shuffle", 0, 10, 5, help="How late the off-beat hats swing")
        self.drift = Param("Drift", 0, 10, 5, help="How far hits sit off the grid")
        self.ghosts = Param("Ghosts", 0, 10, 4, help="How many quiet extra hits")
        self._lean = [0] * BAR      # each step's fixed lean, -100 early .. 100 late
        self._rank = [0] * BAR      # low rank = a ghost turns up here first
        self._kind = [0] * BAR      # what sounds on each step
        self._hot = [0] * BAR       # ms each dot stays swollen
        self._due = []              # hits on their way: [ms to go, step]
        self._deal()
        return (self.skip, self.shuffle, self.drift, self.ghosts)

    def _deal(self):
        for i in range(BAR):
            self._lean[i] = int(rand() * 201) - 100
            self._rank[i] = int(rand() * 12)
        self._lean[0] = 0           # beat 1 stays put: the rest limp round it
        self._lay()

    def _lay(self):
        """Work out what sounds on each step from the knobs."""
        k = self._kind
        for i in range(BAR):
            if i % 2 == 0:
                k[i] = _HAT
            else:
                k[i] = _GHOST if self._rank[i] < self.ghosts.value else 0
        k[4] = k[12] = _SNARE
        k[0] = k[self.skip.value - 1] = _KICK

    def randomise(self):
        self.skip.value = 7 + int(rand() * 6)
        self.shuffle.value = int(rand() * 11)
        self.drift.value = int(rand() * 11)
        self.ghosts.value = int(rand() * 11)
        self._deal()

    def param_changed(self):
        self._lay()
        tune.clear(self.d1)
        self.draw_main()

    def _shift(self, i):
        """How many pixels step i's hit sits right (late) of its line."""
        px = self._lean[i] * self.drift.value * 5 // 1000
        if self._kind[i] == _HAT and i % 4 == 2:
            px += self.shuffle.value // 2
        return px

    def _line(self, i, y, h):
        self.d1.fill_rect(_x(i), y, 1, h, theme.WHITE if i % 4 == 0 else theme.SOFT)

    def _dot(self, i):
        """Redraw step i's dot: clear a box round it, put back the bits of
        line the box took out, then draw the dot at its size for now."""
        d = self.d1
        kind = self._kind[i]
        if not kind:
            return
        x = _x(i) + self._shift(i)
        y = _ROW[kind]
        big = _SIZE[kind] + 2
        d.fill_rect(x - big, y - big, 2 * big + 1, 2 * big + 1, theme.GROUND)
        for j in (i - 1, i, i + 1):
            if 0 <= j < BAR and x - big <= _x(j) <= x + big:
                self._line(j, y - big, 2 * big + 1)
        c = (0, theme.HUES[self.hue][0], theme.WHITE, theme.SOFT, theme.SOFT)[kind]
        gfx.disc(d, x, y, big if self._hot[i] else big - 2, c)

    def draw_main(self):
        for i in range(BAR):
            self._line(i, tune.TOP + 22, 152)
        for i in range(BAR):
            self._dot(i)

    def play_changed(self):
        self._due = []
        for i in range(BAR):
            if self._hot[i]:
                self._hot[i] = 0
                self._dot(i)

    def on_step(self):
        i = self.steps % BAR
        if self._kind[i]:           # on the picture one line's gap is one step long
            self._due.append([(_LEAD + self._shift(i)) * self.step_ms // _PITCH, i])

    def frame(self, ms):
        for i in range(BAR):
            if self._hot[i]:
                self._hot[i] = max(0, self._hot[i] - ms)
                if not self._hot[i]:
                    self._dot(i)
        if not self.playing:
            return
        for hit in self._due:
            hit[0] -= ms
        while self._due and self._due[0][0] <= 0:
            i = self._due.pop(0)[1]
            kind = self._kind[i]
            if kind == _KICK:
                self.kick()
            elif kind:
                self.note(0, kind - 1)      # 1 rim snare, 2 hat, 3 ghost
            self._hot[i] = _FLASH
            self._dot(i)
