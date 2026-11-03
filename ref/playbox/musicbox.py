"""Toy 71 (Witchbox, lead): Music Box.

A tinkling, slightly sour little tune, the same one over and over. It pays
no attention to the chords. It keeps its own time: after running for a
while its spring runs down and every note comes later than the last, until
it has slowed as far as Slow-down allows. Then it is wound up again.

Screen 1 is the box's works. A white cylinder with dark pins turns under a
comb of eight teeth, low notes on the left. Pins creep up the cylinder and
the tooth above flashes pink when its pin reaches the top. Sour makes the
teeth uneven. The key on the right spins while the box is rewound.
"""
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_TUNE = (0, 2, 4, 2, 7, 4, 2, 0, 1, 4, 5, 4, 2, 0, 3, 1)    # as teeth: note = 7 + tooth
_BENT = (3, -2, 5, -4, 1, -5, 4, -1)    # which way Sour bends each tooth
_ROWS, _PITCH = 5, 16                   # pins in view, and their spacing
_X0, _Y0 = 52, 108                      # the cylinder's corner
_REWIND_MS = 900


def _tx(k):
    return 72 + k * 25


class MusicBox(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.size = Param("Tune", 4, 16, 8, help="How many notes are in the tune")
        self.wind = Param("Wind", 2, 16, 4, help="Bars it runs before it slows")
        self.sour = Param("Sour", 0, 10, 3, help="How out of tune the teeth are")
        self.slow = Param("Slow-down", 1, 10, 5, help="How far it slows before the rewind")
        self._i = 0                     # the next note of the tune
        self._gap = 250                 # ms between notes now
        self._t = 0                     # ms since the last note
        self._run = 0                   # ms since it was wound
        self._rewind = 0                # ms of rewinding left
        self._off = 0                   # how far the pins have crept, in pixels
        self._hot = -1                  # the tooth that is pink
        self._hot_ms = 0
        self._key = 0                   # which way round the key is drawn
        self._look = None               # sour and tune length as drawn
        return (self.size, self.wind, self.sour, self.slow)

    def randomise(self):
        self.size.value = 4 + int(rand() * 13)
        self.wind.value = 2 + int(rand() * 6)
        self.sour.value = int(rand() * 11)
        self.slow.value = 1 + int(rand() * 10)

    def param_changed(self):
        if (self.sour.value, self.size.value) != self._look:
            self.draw_main()

    def _tooth(self, k, c):
        h = 44 - k * 2 + _BENT[k] * self.sour.value // 5
        self.d1.fill_rect(_tx(k) - 8, 46, 16, 58, theme.GROUND)
        self.d1.fill_rect(_tx(k) - 8, 46, 16, h, c)

    def _pins(self, off, c):
        """The five pins in view, crept up by off pixels."""
        for r in range(_ROWS):
            k = _TUNE[(self._i + r) % self.size.value]
            self.d1.fill_rect(_tx(k) - 5, _Y0 + (r + 1) * _PITCH - off, 10, 5, c)

    def _wkey(self):
        """The wind-up key, seen flat or edge on."""
        d = self.d1
        d.fill_rect(280, 136, 28, 30, theme.GROUND)
        if self._key % 2:
            d.fill_rect(291, 136, 6, 30, theme.WHITE)
        else:
            d.fill_rect(280, 146, 28, 10, theme.WHITE)

    def draw_main(self):
        d = self.d1
        self._look = (self.sour.value, self.size.value)
        d.fill_rect(56, 34, 208, 12, theme.WHITE)               # the comb's back
        for k in range(8):
            self._tooth(k, theme.HUES[self.hue][0] if k == self._hot else theme.WHITE)
        d.fill_rect(_X0, _Y0, 216, 88, theme.WHITE)             # the cylinder
        d.fill_rect(_X0, _Y0, 6, 88, theme.SOFT)
        d.fill_rect(_X0 + 210, _Y0, 6, 88, theme.SOFT)
        d.fill_rect(_X0 + 216, 148, 12, 6, theme.SOFT)          # the key's shaft
        self._pins(self._off, theme.INK)
        self._wkey()

    def _pluck(self):
        k = _TUNE[self._i]
        self.note(7 + k)
        if self._hot >= 0 and self._hot != k:
            self._tooth(self._hot, theme.WHITE)
        self._hot = k
        self._hot_ms = 140
        self._tooth(k, theme.HUES[self.hue][0])
        self._i = (self._i + 1) % self.size.value
        base = 2 * self.step_ms                                 # two notes to the beat
        if self._run < self.wind.value * BAR * self.step_ms:
            self._gap = base
        else:                                                   # running down
            self._gap = self._gap * 6 // 5
            if self._gap > base * (10 + self.slow.value * 4) // 10:
                self._rewind = _REWIND_MS

    def frame(self, ms):
        if self._hot >= 0:
            self._hot_ms -= ms
            if self._hot_ms <= 0:
                self._tooth(self._hot, theme.WHITE)
                self._hot = -1
        if not self.playing:
            return
        if self._rewind:                # being wound: the key spins, nothing plays
            self._rewind = max(0, self._rewind - ms)
            if self._key != self._rewind // 90:
                self._key = self._rewind // 90
                self._wkey()
            if not self._rewind:
                self._run = self._t = 0
                self._gap = 2 * self.step_ms
            return
        self._run += ms
        self._t += ms
        off = self._off
        if self._t >= self._gap:        # a pin has reached the comb
            self._t -= self._gap
            self._pins(off, theme.WHITE)
            self._pluck()
            off = -1
        new = min(_PITCH - 1, self._t * _PITCH // self._gap)
        if new != off:                  # move the pins only when they shift a pixel
            if off >= 0:
                self._pins(off, theme.WHITE)
            self._pins(new, theme.INK)
            self._off = new

    def play_changed(self):
        if self.playing:
            self._run = 0
