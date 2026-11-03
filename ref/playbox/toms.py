"""Toy 64 (Witchbox, drums): Fog Toms.

Slow, booming low toms with long tails, in a sparse pattern two bars
long. There are three toms, tuned to the root, the fourth and the fifth
of the chord being played. Wander lets the pattern change a little each
time it comes round.

Screen 1 is three white circles low on the screen, small to large; the
largest is the lowest tom. A circle swells and turns purple when it is
struck, then sinks back over the length of its tail, while a faint ring
drifts up from it like fog.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_CY = 172
_X = (250, 160, 70)                 # tom 0 (root) is the big one on the right
_R = (20, 17, 14)
_UP = (0, 3, 4)                     # root, fourth, fifth
_FOG_Y = 140                        # where a fog ring starts; it rises to the top


class Toms(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.hits = Param("Hits", 1, 8, 4, help="How many hits in the two bars")
        self.tail = Param("Tail", 0, 10, 6, help="How long each tom rings on")
        self.depth = Param("Depth", 0, 10, 5, help="How low the toms are tuned")
        self.wander = Param("Wander", 0, 10, 3, help="How much the pattern changes each time")
        self._pat = [[0, 0]] + [self._deal() for _ in range(7)]     # [step, tom] each
        self._ms = [0, 0, 0]        # ms of tail left on each tom
        self._seen = [None] * 3     # each circle as drawn: (radius, purple?)
        self._fog = [None] * 3      # each ring as drawn: (y, colour)
        return (self.hits, self.tail, self.depth, self.wander)

    def _deal(self):
        """One hit: on one of the sixteen half beats, mostly on a whole beat."""
        step = int(rand() * 8) * 4 if rand() < 0.6 else int(rand() * 16) * 2
        return [step, int(rand() * 3)]

    def randomise(self):
        self.hits.value = 1 + int(rand() * 8)
        self.tail.value = int(rand() * 11)
        self.depth.value = int(rand() * 11)
        self.wander.value = int(rand() * 11)
        for k in range(1, 8):
            self._pat[k] = self._deal()

    def param_changed(self):
        for t in range(3):          # a shorter tail cuts short what is ringing
            self._ms[t] = min(self._ms[t], self._tail_ms())

    def _tail_ms(self):
        return 300 + self.tail.value * 200

    def _tom(self, t):
        """Redraw circle t if its size or colour should change. True if it drew."""
        left = self._ms[t]
        r = _R[t] + (4 * left + self._tail_ms() - 1) // self._tail_ms()
        now = (r, left > 0)
        was = self._seen[t]
        if now == was:
            return False
        d = self.d1
        if was and r < was[0]:
            gfx.disc(d, _X[t], _CY, was[0], theme.GROUND)
        gfx.disc(d, _X[t], _CY, r, theme.HUES[self.hue][0] if now[1] else theme.WHITE)
        self._seen[t] = now
        return True

    def _ring(self, t):
        """The fog over tom t: a flat hollow shape that rises and dims."""
        left = self._ms[t]
        now = None
        if left:
            y = tune.TOP + 6 + (_FOG_Y - tune.TOP - 6) * left // self._tail_ms()
            now = (y // 2 * 2, theme.SOFT if left * 2 > self._tail_ms() else theme.SAND)
        was = self._fog[t]
        if now == was:
            return
        d = self.d1
        x, w = _X[t] - _R[t], 2 * _R[t]
        if was:
            d.fill_rect(x, was[0], w, 8, theme.GROUND)
        if now:
            d.fill_rect(x + 3, now[0], w - 6, 2, now[1])
            d.fill_rect(x + 3, now[0] + 6, w - 6, 2, now[1])
            d.fill_rect(x, now[0] + 2, 3, 4, now[1])
            d.fill_rect(x + w - 3, now[0] + 2, 3, 4, now[1])
        self._fog[t] = now

    def draw_main(self):
        self._seen = [None] * 3
        self._fog = [None] * 3
        for t in range(3):
            self._tom(t)
            self._ring(t)

    def on_step(self):
        i = self.steps % (2 * BAR)
        if i == 0 and self.steps:           # round again: some hits wander off
            for k in range(1, 8):
                if rand() < self.wander.value / 20:
                    self._pat[k] = self._deal()
        low = self.root() - 7 * (self.depth.value // 4)     # Depth drops whole octaves
        struck = []                         # two hits may share a step and a tom
        for k in range(self.hits.value):
            step, t = self._pat[k]
            if step == i and t not in struck:
                struck.append(t)
                self.note(low + _UP[t], 3 + t)
                self._ms[t] = self._tail_ms()

    def frame(self, ms):
        drew = False
        for t in range(3):
            if self._ms[t]:
                self._ms[t] = max(0, self._ms[t] - ms)
            self._ring(t)
            if not drew:            # one circle a frame: they are the costly part
                drew = self._tom(t)
