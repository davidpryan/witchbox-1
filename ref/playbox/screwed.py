"""Toy 76 (Witchbox, voice): Screwed Voice.

A slowed-down voice, pitched far down. It moves at half the speed of
everything else: it only ever acts on every other step. Every Drawl bars
it drags out one long low note on the root of the chord, sliding up into
it from a step below. Between notes it may let out a short low mumble.

Screen 1 is a tall pale face outline. The lower the note, the longer the
face sags and the more purple it turns. Its mouth opens slowly on each
note (slower with more Slur) and shuts before the next; a mumble makes
the eyes blink.
"""
import theme, gfx
from settings import Param, BAR
from dark import Dark
from toy import rand

_DOWN = (-3, -4, -7, -14)       # Depth: a fourth, a fifth, one and two octaves
_CX, _TOP, _W = 160, 32, 96
_R = 22                         # the face's corner


def _blend(a, b, n):
    """Colour a moved n sixteenths of the way to colour b (RGB565)."""
    m = 16 - n
    return ((((a >> 11) * m + (b >> 11) * n) >> 4) << 11
            | ((((a >> 5) & 63) * m + ((b >> 5) & 63) * n) >> 4) << 5
            | ((a & 31) * m + (b & 31) * n) >> 4)


class Screwed(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.depth = Param("Depth", 0, 3, 2, choices=("4th", "5th", "1 oct", "2 oct"),
                           help="How far down the voice is pitched")
        self.drawl = Param("Drawl", 0, 3, 0, choices=("1 bar", "2 bars", "3 bars", "4 bars"),
                           help="How long each note lasts")
        self.slur = Param("Slur", 0, 10, 5, help="How slowly it slides into each note")
        self.mumble = Param("Mumble", 0, 10, 3, help="The chance of a low murmur in between")
        self._n = -7                # the note being sung
        self._slide = 0             # ms until the slide reaches the note
        self._sing = False          # mouth opening (True) or closing
        self._blink = 0             # ms the eyes stay shut after a mumble
        self._gape = 0              # mouth height wanted now
        self._face = None           # (sag, colour) of the face on screen
        self._lips = 0              # mouth height on screen
        self._eyes = None
        return (self.depth, self.drawl, self.slur, self.mumble)

    def randomise(self):
        self.depth.value = int(rand() * 4)
        self.drawl.value = int(rand() * 4)
        self.slur.value = int(rand() * 11)
        self.mumble.value = int(rand() * 11)

    def param_changed(self):
        if not self.playing:                    # show the new depth straight away
            self._n = _DOWN[self.depth.value]
        self._draw_face()

    def play_changed(self):
        self._sing = False
        self._slide = 0

    def draw_main(self):
        self._face = None
        self._draw_face()

    def _draw_face(self):
        """The outline: redrawn only when a note makes it sag or change colour."""
        low = max(0, min(16, 2 - self._n))      # 0 = highest note, 16 = lowest
        sag = low * 3
        c = _blend(theme.WHITE, theme.HUES[self.hue][0], low)
        if (sag, c) == self._face:
            return
        d = self.d1
        x = _CX - _W // 2
        d.fill_rect(x, _TOP, _W, 186, theme.GROUND)
        gfx.rrect(d, x, _TOP, _W, 136 + sag, _R, c)
        gfx.rrect(d, x + 5, _TOP + 5, _W - 10, 126 + sag, _R - 5, theme.GROUND)
        self._face = (sag, c)
        self._lips = 0
        self._eyes = None
        self._features()

    def _features(self):
        d = self.d1
        sag, c = self._face
        shut = self._blink > 0
        if shut != self._eyes:                  # eyes: open squares, or shut lines
            for x in (_CX - 26, _CX + 14):
                d.fill_rect(x, _TOP + 40, 12, 10, theme.GROUND)
                d.fill_rect(x, _TOP + (46 if shut else 40), 12, 3 if shut else 10, c)
            self._eyes = shut
        h = 4 + self._gape
        if h != self._lips:                     # the mouth grows downwards
            y = _TOP + 78 + sag // 2
            if h < self._lips:
                d.fill_rect(_CX - 16, y, 32, self._lips, theme.GROUND)
            gfx.rrect(d, _CX - 16, y, 32, h, min(8, h // 2), c)
            self._lips = h

    def on_step(self):
        s = self.steps
        if s % 2:                               # half speed: every other step only
            return
        every = (self.drawl.value + 1) * BAR
        if s % every == 0:
            self._n = self.root() + _DOWN[self.depth.value]
            self._slide = self.slur.value * self.step_ms
            self.note(self._n - 1 if self._slide else self._n, 0)   # start a step below
            self._sing = True
            self._draw_face()
        elif s % every == every - 6:
            self._sing = False                  # shut the mouth before the next note
        elif s % 8 == 4 and not self._slide and rand() * 14 < self.mumble.value:
            self.note(self._n - 2 - int(rand() * 3), 1)             # a mumble
            self._blink = 240

    def frame(self, ms):
        if self._face is None:
            return
        if self._slide and self.playing:
            self._slide = max(0, self._slide - ms)
            if not self._slide:
                self.note(self._n, 0)           # the slide arrives on the note
        if self._blink:
            self._blink = max(0, self._blink - ms)
        full = 30 + self._face[0] // 3
        step = 1 + (10 - self.slur.value) // 3  # more Slur, slower mouth
        if self._sing:
            self._gape = min(full, self._gape + step)
        else:
            self._gape = max(0, self._gape - 2)
        self._features()
