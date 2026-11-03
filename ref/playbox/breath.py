"""Toy 77 (Witchbox, voice): Breath Stack.

Airy high voices in harmony. Up to four voices come in one at a time on
notes of the chord, Stagger apart. Once all are in they hold together for
Hold bars, following the chords, then leave one at a time, rest a moment
and start again.

Screen 1 is four ribbons stacked up the screen, the lowest voice at the
bottom. A ribbon fades in as its voice enters (grey, pale, white, then
pink at full strength), ripples while it sounds and fades out as the voice
leaves. More Air breaks the ribbons into shorter, breathier dashes. Voices
that are switched off show as a thin line.
"""
import theme
from settings import Param, BAR
from dark import Dark
from toy import rand

_APART = (4, 8, 16, 32)         # Stagger in steps
_NOTES = (7, 9, 11, 14)         # each voice's note above the chord's root
_X0, _SEG, _NSEG = 28, 33, 8    # ribbons are eight dashes
_WAVE = (0, 2, 3, 2, 0, -2, -3, -2)


def _y(v):
    return 176 - v * 38


class Breath(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.voices = Param("Voices", 1, 4, 3, help="How many voices join in")
        self.stagger = Param("Stagger", 0, 3, 1, choices=("1 beat", "2 beats", "1 bar", "2 bars"),
                             help="How far apart the voices come in")
        self.air = Param("Air", 0, 10, 3, help="From a clear tone to mostly breath")
        self.hold = Param("Hold", 1, 8, 2, help="How many bars the full stack holds")
        self._t = 0                 # steps into the in-hold-out cycle
        self._on = [False] * 4      # which voices are sounding
        self._lvl = [0] * 4         # each ribbon's strength on screen, 0..3
        self._fade = 0              # ms until the next fade step
        self._rip = 0               # ms until the next ripple step
        self._phase = 0
        return (self.voices, self.stagger, self.air, self.hold)

    def randomise(self):
        self.voices.value = 1 + int(rand() * 4)
        self.stagger.value = int(rand() * 4)
        self.air.value = int(rand() * 11)
        self.hold.value = 1 + int(rand() * 8)

    def param_changed(self):
        for v in range(self.voices.value, 4):       # voices switched off fall silent
            self._on[v] = False
        self.draw_main()

    def play_changed(self):
        self._t = 0
        if not self.playing:
            self._on = [False] * 4

    def _ribbon(self, v):
        d = self.d1
        y = _y(v)
        d.fill_rect(_X0, y - 3, _SEG * _NSEG, 18, theme.GROUND)
        if v >= self.voices.value:                  # not in use: a thin line
            d.fill_rect(_X0, y + 5, _SEG * _NSEG, 2, theme.SAND)
            return
        lvl = self._lvl[v]
        c = (theme.SAND, theme.SOFT, theme.WHITE, theme.HUES[self.hue][0])[lvl]
        w = _SEG - 1 - self.air.value * 2           # more air, shorter dashes
        for i in range(_NSEG):
            dy = _WAVE[(i + self._phase + v * 3) % 8] if lvl else 0
            d.fill_rect(_X0 + i * _SEG, y + dy, w, 12, c)

    def draw_main(self):
        for v in range(4):
            self._ribbon(v)

    def on_step(self):
        n = self.voices.value
        gap = _APART[self.stagger.value]
        full = (n - 1) * gap + self.hold.value * BAR    # when the first voice leaves
        t = self._t
        for v in range(n):
            if t == v * gap:                            # voice v comes in
                self._on[v] = True
                self.note(self.root() + _NOTES[v], v)
            elif t == full + v * gap:                   # and leaves, in the same order
                self._on[v] = False
            elif self._on[v] and self.steps % BAR == 0:
                self.note(self.root() + _NOTES[v], v)   # held voices move with the chord
        self._t = t + 1
        if self._t >= full + n * gap + 4:               # a beat's rest, then again
            self._t = 0

    def frame(self, ms):
        self._fade -= ms
        self._rip -= ms
        ripple = self._rip <= 0 and self.playing
        if ripple:
            self._rip = 200
            self._phase = (self._phase + 1) % 8
        fade = self._fade <= 0
        if fade:
            self._fade = 160
        for v in range(self.voices.value):
            want = 3 if self._on[v] else 0
            moved = fade and self._lvl[v] != want
            if moved:
                self._lvl[v] += 1 if want else -1
            if moved or (ripple and self._lvl[v]):
                self._ribbon(v)
