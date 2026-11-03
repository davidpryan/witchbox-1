"""Toy 63 (Witchbox, drums): Broken Clock.

Glitch edits. Underneath is a simple beat: a click twice a beat, a snap
on beats 2 and 4 and a soft kick on beat 1. Now and then a small slice
of it gets stuck and repeats in a fast stutter (Stutter, Slice,
Repeats), and now and then a step is cut out to leave a tiny silence
(Holes).

Screen 1 is a clock: sixteen dots round the rim, one per step of the
bar, and one white hand that goes round once a bar. When a slice
repeats, the hand jumps back and judders in place, drawn in cyan. A
silence shows as a dot missing from the rim until the hand comes round
again.
"""
from math import sin, cos, pi
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_CX, _CY = 160, 123
_SLICE = (2000, 1000, 500)          # slice lengths in 1000ths of a step
# where the hand points for each half step, and where each rim dot sits
_HAND = tuple((_CX + int(62 * sin(k * pi / 16)), _CY - int(62 * cos(k * pi / 16)))
              for k in range(32))
_RIM = tuple((_CX + int(84 * sin(k * pi / 8)), _CY - int(84 * cos(k * pi / 8)))
             for k in range(16))


class Clock(Dark):
    FRAME_MS = 20

    def make_params(self):
        self.stutter = Param("Stutter", 0, 10, 4, help="How often a slice gets stuck")
        self.slice = Param("Slice", 0, 2, 1, choices=("8th", "16th", "32nd"),
                           help="How long the stuck slice is")
        self.reps = Param("Repeats", 2, 8, 4, help="How many times the slice plays")
        self.holes = Param("Holes", 0, 10, 3, help="How often a step is cut to silence")
        self._gone = [False] * BAR  # rim dots that are missing
        self._hand = None           # the hand as drawn: (half step, colour)
        self._togo = 0              # repeats still to play
        self._wait = 0              # ms to the next repeat
        self._voice = 2             # the sound that is stuck
        self._from = 0              # the step the slice was cut from
        return (self.stutter, self.slice, self.reps, self.holes)

    def randomise(self):
        self.stutter.value = int(rand() * 11)
        self.slice.value = int(rand() * 3)
        self.reps.value = 2 + int(rand() * 7)
        self.holes.value = int(rand() * 11)

    def _dot(self, i):
        x, y = _RIM[i]
        r = 4 if i % 4 == 0 else 2
        c = theme.GROUND if self._gone[i] else theme.WHITE
        self.d1.blit(x - r, y - r, 2 * r + 1, 2 * r + 1, gfx.dot_sprite(r, c))

    def _point(self, half, c=theme.WHITE):
        """Move the hand: rub out the old line, draw the new, cap the middle."""
        if self._hand == (half, c):
            return
        d = self.d1
        if self._hand:
            x, y = _HAND[self._hand[0]]
            gfx.line(d, _CX, _CY, x, y, theme.GROUND, 3)
        x, y = _HAND[half]
        gfx.line(d, _CX, _CY, x, y, c, 3)
        d.blit(_CX - 4, _CY - 4, 11, 11, gfx.dot_sprite(5, theme.WHITE))
        self._hand = (half, c)

    def draw_main(self):
        for i in range(BAR):
            self._dot(i)
        half = self._hand[0] if self._hand else 0
        self._hand = None
        self._point(half)

    def play_changed(self):
        self._togo = 0
        for i in range(BAR):
            if self._gone[i]:
                self._gone[i] = False
                self._dot(i)
        self._point(self._hand[0] if self.playing else 0)

    def _sound(self, voice):
        if voice:
            self.note(0, voice)
        else:
            self.kick()

    def on_step(self):
        i = self.steps % BAR
        if self._togo:              # stuck: the beat underneath is not heard
            return
        self._point(2 * i)
        if self._gone[i]:           # the hand is back: mend last bar's hole
            self._gone[i] = False
            self._dot(i)
        if i and rand() < self.holes.value / 40:
            self._gone[i] = True    # a hole: this step is silent
            self._dot(i)
            return
        if i % 2:
            return
        self._voice = 0 if i == 0 else 1 if i % 8 == 4 else 2      # kick, snap, click
        self._sound(self._voice)
        if rand() < self.stutter.value / 30:
            self._from = i          # this slice sticks; frame() plays the repeats
            self._togo = self.reps.value - 1
            self._wait = _SLICE[self.slice.value] * self.step_ms // 1000

    def frame(self, ms):
        if not self.playing or not self._togo:
            return
        self._wait -= ms
        if self._wait <= 0:
            self._togo -= 1
            self._wait += _SLICE[self.slice.value] * self.step_ms // 1000
            self._sound(self._voice or 2)       # a stuck kick repeats as a click
            # judder: the hand flicks between the slice's start and just past it
            self._point(2 * self._from + self._togo % 2, theme.HUES[self.hue][0])
