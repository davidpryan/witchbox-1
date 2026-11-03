"""Toy 57 (Witchbox, lead): Bitcrush Arp.

A square-wave arpeggio running up and down the current chord, fast enough
to blur into one crunchy chord. Now and then it glitches: the last three
notes repeat in place a few times before it carries on.

Screen 1 is a staircase of sixteen big white pixels, one per note, that
climb and fall with the notes. The playing pixel is green. Pulse width
makes the pixels thin or square, Crush makes them bigger and rougher, and
on a glitch the green pixel jumps back over the same three.
"""
import theme, tune
from settings import Param
from dark import Dark
from toy import rand

_N = 16
_STAIR = (0, 1, 2, 3, 4, 3, 2, 1)       # up and down five chord notes
_DIV = (1, 2, 4)                        # notes to a step at each Rate
_SLICE = 3                              # how many pixels a glitch repeats


class BitArp(Dark):
    FRAME_MS = 20

    def make_params(self):
        self.rate = Param("Rate", 0, 2, 1, choices=("16ths", "32nds", "Faster"),
                          help="How fast the notes run")
        self.pulse = Param("Pulse", 1, 10, 6, help="The wave, from thin to square")
        self.crush = Param("Crush", 0, 10, 4, help="From clean to very rough")
        self.glitch = Param("Glitch", 0, 10, 3, help="How often a slice repeats")
        self._root = 0
        self._at = 0                    # the next pixel to play
        self._lit = -1                  # the pixel that is green now
        self._due = 0                   # notes still to play in this step
        self._wait = 0                  # ms until the next of them
        self._again = 0                 # glitch notes still to play
        self._from = 0                  # where the glitch slice starts
        self._look = None               # pulse and crush as drawn
        return (self.rate, self.pulse, self.crush, self.glitch)

    def randomise(self):
        self.rate.value = int(rand() * 3)
        self.pulse.value = 1 + int(rand() * 10)
        self.crush.value = int(rand() * 11)
        self.glitch.value = int(rand() * 8)

    def param_changed(self):
        if (self.pulse.value, self.crush.value) != self._look:
            self.draw_main()

    def _note(self, i):
        return tune.tones(self._root, 0, 20)[_STAIR[i % 8]]

    def _pixel(self, i, c):
        w = 4 + (self.pulse.value * 4) // 3         # 5 to 17 wide
        h = 6 + self.crush.value                    # 6 to 16 tall
        y = 194 - self._note(i) * 12
        self.d1.fill_rect(25 + i * 18 - w // 2, y - h // 2, w, h, c)

    def draw_main(self):
        self._look = (self.pulse.value, self.crush.value)
        for i in range(_N):
            self.d1.fill_rect(16 + i * 18, tune.TOP + 4, 18, 180, theme.GROUND)
            self._pixel(i, theme.HUES[self.hue][0] if i == self._lit else theme.WHITE)

    def on_step(self):
        r = self.root()
        if r != self._root:             # a new chord: the whole staircase moves
            self._root = r
            self.draw_main()
        if not self._again and rand() < self.glitch.value / 40:
            self._from = (self._at - _SLICE) % _N
            self._again = _SLICE * (2 + int(rand() * 3))
        self._due = _DIV[self.rate.value]           # this step's notes, timed in frame()
        self._wait = 0

    def _play(self):
        if self._again:                 # glitching: go round the slice again
            self._again -= 1
            i = (self._from + (_SLICE - 1 - self._again) % _SLICE) % _N
        else:
            i = self._at
            self._at = (i + 1) % _N
        if self._lit >= 0:
            self._pixel(self._lit, theme.WHITE)
        self._lit = i
        self._pixel(i, theme.HUES[self.hue][0])
        self.note(7 + self._note(i))

    def frame(self, ms):
        if not self.playing or not self._due:
            return
        self._wait -= ms
        while self._due and self._wait <= 0:
            self._due -= 1
            self._play()
            self._wait += self.step_ms // _DIV[self.rate.value]

    def play_changed(self):
        self._due = self._again = 0
        if self.playing:
            self._at = 0
        elif self._lit >= 0:
            self._pixel(self._lit, theme.WHITE)
            self._lit = -1
