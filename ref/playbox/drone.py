"""Toy 50 (Witchbox, bass): Drone.

One note held under everything. It stays on the key note, the fifth above
it, or both, and takes no notice of the chords. It is struck again every
four bars so it never dies away, and its tone slowly brightens and dulls.

For the sound engine: self.tone runs 0..100 and back as the tone shifts.

Screen 1 is one thick white bar across the middle. It grows fatter and
thinner as the tone breathes, and thin purple bands appear above it, one
for each overtone that has come in.
"""
from math import sin, pi
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_CY = 150                       # middle of the bar
_X, _W = 20, 280
_BANDS = 10                     # most overtone bands


class Drone(Dark):
    FRAME_MS = 50

    def make_params(self):
        self.which = Param("Note", 0, 2, 0, choices=("Root", "Fifth", "Both"),
                           help="Which note it holds")
        self.over = Param("Overtones", 0, 10, 5, help="From a pure tone to a rich buzz")
        self.breathe = Param("Breathe", 1, 10, 5, help="How fast the tone shifts")
        self.octave = Param("Octave", 0, 2, 0, choices=("Low", "Middle", "Both"),
                            help="How deep it sits")
        self.tone = 0
        self._t = 0                 # where we are in the breath, 0..65535
        self._again = True          # strike the notes on the next step
        self._half = 0              # half the bar's thickness on screen
        self._shown = 0             # overtone bands on screen
        return (self.which, self.over, self.breathe, self.octave)

    def randomise(self):
        self.which.value = int(rand() * 3)
        self.over.value = int(rand() * 11)
        self.breathe.value = 1 + int(rand() * 10)
        self.octave.value = int(rand() * 3)

    def param_changed(self):
        self._again = True
        self._look()

    def play_changed(self):
        self._again = True
        self._look()

    def on_step(self):
        if self._again or self.steps % (4 * BAR) == 0:
            self._again = False
            v = 0
            for octave in ((-14,), (-7,), (-14, -7))[self.octave.value]:
                for deg in ((0,), (4,), (0, 4))[self.which.value]:
                    self.note(octave + deg, v)      # the key's own notes, whatever the chord
                    v += 1

    def frame(self, ms):
        if self.playing:            # one breath takes 16 seconds down to 3
            self._t = (self._t + ms * 65536 // (17500 - self.breathe.value * 1450)) & 65535
            self.tone = int(50 - 50 * sin(2 * pi * self._t / 65536 + pi / 2))
            self._look()

    def draw_main(self):
        self._half = self._shown = 0
        self._look()

    def _look(self):
        """Bring the bar and the bands up to date, drawing only the change."""
        d = self.d1
        tone = self.tone if self.playing else 0
        # A fatter bar for more notes; it swells as the tone opens.
        notes = (1, 1, 2)[self.which.value] * (1, 1, 2)[self.octave.value]
        half = 6 + notes * 3 + tone * 8 // 100
        was = self._half
        if half > was:
            d.fill_rect(_X, _CY - half, _W, half - was, theme.WHITE)
            d.fill_rect(_X, _CY + was, _W, half - was, theme.WHITE)
        elif half < was:
            d.fill_rect(_X, _CY - was, _W, was - half, theme.GROUND)
            d.fill_rect(_X, _CY + half, _W, was - half, theme.GROUND)
        self._half = half
        # Overtones come in one by one as the tone opens.
        n = (self.over.value * (30 + tone * 70 // 100) + 50) // 100 if self.playing else 0
        while self._shown < n:
            self._band(self._shown, theme.HUES[self.hue][0])
            self._shown += 1
        while self._shown > n:
            self._shown -= 1
            self._band(self._shown, theme.GROUND)

    def _band(self, k, c):
        # Higher overtones are shorter and sit higher.
        self.d1.fill_rect(_X + k * 10, _CY - 36 - k * 9, _W - k * 20, 3, c)
