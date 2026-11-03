"""Toy 51 (Witchbox, chords): Pump Pad.

A held chord from the shared chords. Every kick knocks its volume down and
it swells back up. With a drum toy in the band it ducks on that toy's
kicks; without one it pumps by itself on every beat.

For the sound engine: self.level is the volume now, 0..100.

Screen 1 is a soft white blob. Each kick squashes it flat and it puffs
back up, glowing pink while it swells. Its width shows how widely the
chord's notes are spread.
"""
import theme, gfx, tune
import settings
from settings import Param, BAR
from dark import Dark
from toy import rand

_CX, _CY = 160, 123
_TALL = 124                     # the blob at full volume
_FLAT = 24                      # and squashed right down
_R = 12                         # how round its corners are


class PumpPad(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.depth = Param("Depth", 0, 10, 8, help="How far the volume ducks on a kick")
        self.recovery = Param("Recovery", 1, 10, 5, help="How fast it swells back")
        self.colour = Param("Colour", 0, 2, 1, choices=("Triad", "Add 7", "Add 9"),
                            help="How many notes are in the chord")
        self.spread = Param("Spread", 0, 10, 4, help="From close notes to a wide chord")
        self.level = 100
        self._lvl = 1000            # the level in thousandths, for smooth swelling
        self._box = None            # the blob on screen: (width, height, colour)
        return (self.depth, self.recovery, self.colour, self.spread)

    def randomise(self):
        self.depth.value = 3 + int(rand() * 8)
        self.recovery.value = 1 + int(rand() * 10)
        self.colour.value = int(rand() * 3)
        self.spread.value = int(rand() * 11)

    def param_changed(self):
        self._blob()

    def draw_main(self):
        self._box = None
        self._blob()

    def play_changed(self):
        self._lvl = 1000
        self._blob()

    def _duck(self):
        self._lvl = min(self._lvl, 1000 - self.depth.value * 100)

    def on_step(self):
        i = self.steps % BAR
        if i == 0:                                  # a new bar: sound the chord
            r = self.root()
            sp = self.spread.value
            for k in range(3 + self.colour.value):
                deg = r + 2 * k
                if (k == 1 and sp >= 3) or (k == 3 and sp >= 9):
                    deg += 7                        # wider: lift the third, then the seventh
                if k == 0 and sp >= 6:
                    deg -= 7                        # and drop the root
                self.note(deg, k)
        if not settings.drummer and i % 4 == 0:     # nobody is drumming: pump on the beat
            self._duck()

    def frame(self, ms):
        if not self.playing:
            return
        if settings.drummer and self.new_kick():
            self._duck()
        if self._lvl < 1000:                        # back to full in 1.2 s down to 0.15 s
            self._lvl = min(1000, self._lvl + ms * 1000 // (1320 - self.recovery.value * 117))
        self.level = self._lvl // 10
        self._blob()

    def _blob(self):
        """Draw the blob again only if its size or colour has changed."""
        d = self.d1
        w = 96 + self.spread.value * 20
        h = (_FLAT + (_TALL - _FLAT) * self._lvl // 1000) & ~3     # in 4 px steps
        c = theme.HUES[self.hue][0] if self._lvl < 1000 else theme.WHITE
        new = (w, h, c)
        was = self._box
        if new == was:
            return
        self._box = new
        if was:
            ow, oh = was[0], was[1]
            if ow != w:                             # a new width: start afresh
                d.fill_rect(_CX - ow // 2, _CY - oh // 2, ow, oh, theme.GROUND)
            elif oh > h:                            # shorter: trim top and bottom
                cut = (oh - h) // 2 + _R            # (the old round corners go too)
                d.fill_rect(_CX - w // 2, _CY - oh // 2, w, cut, theme.GROUND)
                d.fill_rect(_CX - w // 2, _CY + oh // 2 - cut, w, cut, theme.GROUND)
        gfx.rrect(d, _CX - w // 2, _CY - h // 2, w, h, _R, c)
