"""Toy 62 (Witchbox, drums): Snare Rush.

Snare rolls that speed up into the end of a bar. Every few bars a roll
starts, a quarter of a bar to a whole bar before the bar line, and gets
faster as it goes (Speed-up). A crash lands on the downbeat that ends
it. Between rolls it plays nothing, or a plain snare on beats 2 and 4
(Backbeat).

Screen 1 is the roll as a row of white upright bars, one per hit, read
left to right: they crowd together towards the right edge as the roll
speeds up. Each bar flashes orange as it plays, and a thin orange strip
flashes right across the screen on the downbeat that ends the roll. The
two small squares underneath are the backbeat.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_EVERY = (2, 4, 8)
_X0, _W = 14, 290                   # the roll's bars are spread across this
_Y, _H = 76, 96
_STRIP_Y = tune.TOP + 12
_FLASH = 130                        # ms the downbeat strip stays lit


class Rush(Dark):
    FRAME_MS = 20

    def make_params(self):
        self.every = Param("Every", 0, 2, 0, choices=("2 bars", "4 bars", "8 bars"),
                           help="How often a roll comes")
        self.span = Param("Length", 1, 4, 2, choices=("1/4 bar", "1/2 bar", "3/4 bar", "1 bar"),
                          help="How long the roll lasts")
        self.accel = Param("Speed-up", 0, 10, 6, help="0 is an even roll; 10 ends 4 times as fast")
        self.back = Param("Backbeat", 0, 1, 1, choices=("Off", "On"),
                          help="A snare on beats 2 and 4 between rolls")
        self._hits = []             # when each hit of the roll falls, in 1000ths of a step
        self._k = -1                # the next hit to play; -1 = no roll going
        self._t = 0                 # ms since the roll began
        self._hot = -1              # the bar that is orange
        self._strip = 0             # ms of downbeat flash left
        self._beat = -1             # the backbeat square that is lit
        self._plan()
        return (self.every, self.span, self.accel, self.back)

    def _plan(self):
        """Lay out the roll: it starts in sixteenths and the gaps shrink
        smoothly until, at the far end, it is `ratio` times as fast."""
        total = self.span.value * 4000
        ratio = 4 ** (self.accel.value / 10)
        self._hits = []
        t = 0
        while t < total - 100:
            self._hits.append(int(t))
            t += 1000 / ratio ** (t / total)

    def randomise(self):
        self.every.value = int(rand() * 3)
        self.span.value = 1 + int(rand() * 4)
        self.accel.value = int(rand() * 11)
        self.back.value = int(rand() * 2)

    def param_changed(self):
        self._plan()
        self._k = self._hot = -1
        self.d1.fill_rect(0, _Y, 320, _H, theme.GROUND)
        self.draw_main()

    def _bar(self, k, on=False):
        x = _X0 + self._hits[k] * _W // (self.span.value * 4000)
        self.d1.fill_rect(x, _Y, 2, _H, theme.HUES[self.hue][0] if on else theme.WHITE)

    def _square(self, n, on=False):
        c = theme.GROUND
        if self.back.value:
            c = theme.HUES[self.hue][0] if on else theme.SAND
        self.d1.fill_rect(_X0 + 66 + n * 145, _Y + _H + 14, 12, 12, c)

    def _flash(self, on):
        c = theme.HUES[self.hue][0] if on else theme.GROUND
        self.d1.fill_rect(0, _STRIP_Y, 320, 6, c)
        self.d1.fill_rect(0, tune.BOT - 12, 320, 6, c)

    def draw_main(self):
        for k in range(len(self._hits)):
            self._bar(k, k == self._hot)
        self._square(0, self._beat == 0)
        self._square(1, self._beat == 1)
        self._flash(self._strip > 0)

    def play_changed(self):
        self._k = -1
        self._calm()
        self._strip = 0
        self._flash(False)

    def _calm(self):
        if self._hot >= 0:
            self._bar(self._hot)
            self._hot = -1
        if self._beat >= 0:
            self._square(self._beat)
            self._beat = -1

    def on_step(self):
        i = self.steps % BAR
        every = _EVERY[self.every.value]
        last = self.bar_no() % every == every - 1       # the bar a roll runs into
        begin = BAR - 4 * self.span.value
        if i == 0:
            self._calm()
            if self._k >= 0:
                self._k = -1                            # a roll has just run into this bar line
                self.note(0, 3)                         # the crash
                self._strip = _FLASH
                self._flash(True)
        if last and i == begin:
            self._k = 0                                 # frame() plays the roll from here
            self._t = 0
        elif self._k < 0 and self.back.value and i % 8 == 4:
            self._calm()
            self.note(0, 1)
            self._beat = i // 8
            self._square(self._beat, True)
        elif self._beat >= 0 and i % 4 == 2:
            self._calm()

    def frame(self, ms):
        if self._strip:
            self._strip = max(0, self._strip - ms)
            if not self._strip:
                self._flash(False)
        if not self.playing or self._k < 0:
            return
        while self._k < len(self._hits) and self._hits[self._k] * self.step_ms // 1000 <= self._t:
            if self._hot >= 0:
                self._bar(self._hot)
            self._hot = self._k
            self._bar(self._k, True)
            self.note(0, 1)
            self._k += 1
        self._t += ms
