"""Toy 54 (Witchbox, chords): Wet Piano.

Sparse five-note chords (a ninth chord: the root and every other scale
step above it) on a dull piano. Twice a bar a chord may come, a touch
late, its notes spread out like a slow hand; often it is left out.

For the sound engine: Wow is how much the pitch wavers, like a worn tape,
and Dull is how muffled the tone is.

Screen 1 is a white keyboard along the bottom, two octaves of the scale.
A beat before a chord, a blue raindrop starts to fall towards each key
that will play. A key goes dark when its drop lands and its note sounds.
Drops sway more as Wow rises, and the keys go darker as Dull rises.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_KEYS = 15                      # scale steps 0..14
_KW = 20
_KX = 10
_KY, _KH = 152, 62              # the keyboard
_FALL = 110                     # a drop falls this far in one beat
_LATE = 30                      # ms every chord drags behind the beat
_RING = 450                     # ms a key stays dark
_SWAY = (0, 1, 2, 1, 0, -1, -2, -1)


class WetPiano(Dark):
    FRAME_MS = 25

    def make_params(self):
        self.strum = Param("Strum", 0, 10, 4, help="From all at once to a slow spread")
        self.sparse = Param("Sparse", 0, 10, 4, help="How often a chord is left out")
        self.wow = Param("Wow", 0, 10, 3, help="How much the pitch wavers")
        self.dull = Param("Dull", 0, 10, 5, help="How muffled the tone is")
        self._drops = []            # falling: [key, ms until it lands, x, y on screen]
        self._ring = [0] * _KEYS    # ms each key stays dark
        return (self.strum, self.sparse, self.wow, self.dull)

    def randomise(self):
        self.strum.value = int(rand() * 11)
        self.sparse.value = int(rand() * 9)
        self.wow.value = int(rand() * 11)
        self.dull.value = int(rand() * 11)

    def _key(self, k):
        c = theme.WHITE
        if self._ring[k]:           # duller tone, darker key
            c = theme.HUES[self.hue][0 if self.dull.value < 4 else 1]
            if self.dull.value > 7:
                c = theme.SAND
        self.d1.fill_rect(_KX + k * _KW, _KY, _KW - 1, _KH, c)

    def draw_main(self):
        for k in range(_KEYS):
            self._key(k)

    def param_changed(self):
        for k in range(_KEYS):
            if self._ring[k]:
                self._key(k)

    def play_changed(self):
        if not self.playing:
            for drop in self._drops:
                self._rub(drop)
            self._drops = []
            for k in range(_KEYS):
                if self._ring[k]:
                    self._ring[k] = 0
                    self._key(k)

    def _rub(self, drop):
        if drop[3]:
            self.d1.fill_rect(drop[2] - 3, drop[3] - 3, 7, 7, theme.GROUND)

    def on_step(self):
        i = self.steps % BAR
        # One beat before each chord's place (steps 0 and 8), decide whether
        # it comes; if so its drops start to fall.
        if i not in (4, 12) or rand() < self.sparse.value / 11:
            return
        r = self.root(1 if i == 12 else 0)
        beat = 4 * self.step_ms
        self._drops = self._drops[-5:] + [
            [r + 2 * k, beat + _LATE + k * self.strum.value * 14, 0, 0] for k in range(5)]

    def frame(self, ms):
        d = self.d1
        for k in range(_KEYS):                  # keys come back up
            if self._ring[k]:
                self._ring[k] = max(0, self._ring[k] - ms)
                if not self._ring[k]:
                    self._key(k)
        if not self._drops:
            return
        beat = 4 * self.step_ms
        landed = False
        for drop in self._drops:
            drop[1] -= ms
            self._rub(drop)
            k = drop[0]
            if drop[1] <= 0:                    # it lands: the note sounds
                landed = True
                self.note(k, k % 5)
                self._ring[k] = _RING
                self._key(k)
                continue
            y = _KY - 5 - drop[1] * _FALL // beat
            if y < tune.TOP + 4:                # still above the screen
                drop[3] = 0
                continue
            x = _KX + k * _KW + _KW // 2 + self.wow.value * _SWAY[(y >> 3) & 7] // 4
            d.blit(x - 3, y - 3, 7, 7, gfx.dot_sprite(3, theme.HUES[self.hue][0]))
            drop[2], drop[3] = x, y
        if landed:
            self._drops = [drop for drop in self._drops if drop[1] > 0]
