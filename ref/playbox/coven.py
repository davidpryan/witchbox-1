"""Toy 52 (Witchbox, chords): Coven Saws.

Thick, buzzing witch house chords made of three voices: the chord's root,
its fifth and the root again an octave up. Each chord is held for whole
bars while its tone slowly opens from dull to bright.

Now and then the chord slips up a semitone. A semitone is not always a
step of the scale, so the toy cannot say it as a scale step: it plays the
next step up (root + 1) and sets self.slipped = True, and the sound engine
should pitch that chord one semitone above the real root instead.
self.opened (0..100) is how far the tone has opened.

Screen 1 is three stacked triangles, one per voice. They sit further apart
as Detune rises and turn from white to purple as the tone opens. On a
slip the whole stack jolts sideways.
"""
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_CX, _CY = 160, 123
_H, _ROWS = 36, 9               # a triangle: 9 slices of 4 px
_HOLD = (1, 2, 4)
_JOLT = 26                      # how far a slip throws the stack
_SHADES = 6


def _mix(a, b, n, of):
    """Colour n/of of the way from a to b (both RGB565)."""
    m = of - n
    return ((((a >> 11) * m + (b >> 11) * n) // of) << 11
            | (((a >> 5 & 63) * m + (b >> 5 & 63) * n) // of) << 5
            | ((a & 31) * m + (b & 31) * n) // of)


class Coven(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.detune = Param("Detune", 0, 10, 4, help="How far apart the voices are tuned")
        self.open = Param("Open", 0, 10, 7, help="How far the tone opens over each chord")
        self.slip = Param("Slip", 0, 10, 2, help="How often a chord slips up a semitone")
        self.hold = Param("Hold", 0, 2, 0, choices=("1 bar", "2 bars", "4 bars"),
                          help="How long each chord is held")
        self.slipped = False
        self.opened = 0
        self._age = 0               # ms since the chord began
        self._jolt = 0              # ms the stack stays thrown sideways
        self._look = None           # on screen: (x offset, gap, shade)
        return (self.detune, self.open, self.slip, self.hold)

    def randomise(self):
        self.detune.value = int(rand() * 11)
        self.open.value = int(rand() * 11)
        self.slip.value = int(rand() * 6)
        self.hold.value = int(rand() * 3)

    def param_changed(self):
        self._stack()

    def draw_main(self):
        self._look = None
        self._stack()

    def play_changed(self):
        self._age = self._jolt = self.opened = 0
        self.slipped = False
        self._stack()

    def on_step(self):
        if self.steps % BAR or self.bar_no() % _HOLD[self.hold.value]:
            return
        r = self.root()
        self.slipped = rand() < self.slip.value / 20
        if self.slipped:
            r += 1                              # stands for a semitone up; see the top
            self._jolt = 320
        for v, deg in enumerate((r - 7, r - 3, r)):     # root, fifth, root
            self.note(deg, v)
        self._age = 0

    def frame(self, ms):
        if not self.playing:
            return
        self._age += ms
        self._jolt = max(0, self._jolt - ms)
        whole = _HOLD[self.hold.value] * BAR * self.step_ms
        self.opened = self.open.value * 10 * min(self._age, whole) // whole
        self._stack()

    def _stack(self):
        """Redraw the three triangles only if they moved or changed shade."""
        d = self.d1
        gap = 38 + self.detune.value * 2
        new = (_JOLT if self._jolt else 0, gap, self.opened * _SHADES // 100)
        was = self._look
        if new == was:
            return
        self._look = new
        if was and was[:2] != new[:2]:          # moved: rub out the old stack
            d.fill_rect(_CX + was[0] - 40, _CY - was[1] - _H // 2, 80, 2 * was[1] + _H,
                        theme.GROUND)
        c = _mix(theme.WHITE, theme.HUES[self.hue][0], new[2], _SHADES)
        for k in (-1, 0, 1):
            top = _CY + k * gap - _H // 2
            for i in range(_ROWS):              # a triangle, widening slice by slice
                half = 4 + i * 4
                d.fill_rect(_CX + new[0] - half, top + i * 4, 2 * half, 4, c)
