"""Toy 60 (Witchbox, lead): Screamer.

A dirty, noisy lead in short bursts. Each burst is a few very fast notes
from the scale, two to a step, and the last one is a squeal that bends
sharply up or down.

Screen 1 is a jagged line that shoots across for each burst, one piece per
note, and ends in a spike that points the way the squeal bends. Dirt makes
the line more ragged. The line is red while it sounds, turns grey when the
burst is over, and is then rubbed out. While paused the last line is white.
"""
import theme, gfx, tune
from settings import Param
from dark import Dark
from toy import rand

_DX = 30                        # how far each note carries the line
_TOP, _BOT = tune.TOP + 6, tune.BOT - 22
_TRAIL_MS = 320                 # how long the grey trail stays


class Screamer(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.bursts = Param("Bursts", 1, 10, 5, help="How often a burst happens")
        self.size = Param("Length", 2, 8, 5, help="How many notes are in a burst")
        self.squeal = Param("Squeal", 0, 10, 6, help="How far the last note bends")
        self.dirt = Param("Dirt", 0, 10, 4, help="From buzzy to pure noise")
        self._pts = []              # the line's corners on screen: x, y, x, y ...
        self._left = 0              # notes still to come in this burst
        self._wait = 0              # ms until the next of them
        self._n = 10                # the last note played
        self._ink = theme.WHITE     # the colour the line is in
        self._paint = None          # repainting: [colour, next piece]
        self._hold = 0              # ms the finished line has left
        self._start(4)
        while self._left:           # a line to show before anything has played
            self._next(False)
        return (self.bursts, self.size, self.squeal, self.dirt)

    def randomise(self):
        self.bursts.value = 1 + int(rand() * 10)
        self.size.value = 2 + int(rand() * 7)
        self.squeal.value = int(rand() * 11)
        self.dirt.value = int(rand() * 11)

    def _start(self, many):
        self._left = many
        self._wait = 0
        self._n = tune.nearest(8 + int(rand() * 7), self.root())
        x = 16 + int(rand() * (288 - many * _DX))
        self._pts = [x, tune.height(self._n, 4, 20, _BOT - 10, _TOP + 10)]

    def _next(self, live=True):
        """The burst's next note: sound it and carry the line on."""
        p = self._pts
        self._left -= 1
        x, y = p[-2], p[-1]
        if self._left:                              # a fast note: a few steps away
            self._n = max(5, min(19, self._n + (int(rand() * 4) + 1) * (1 if rand() < 0.5 else -1)))
            ny = tune.height(self._n, 4, 20, _BOT - 10, _TOP + 10)
            # each piece zigzags, one up, the next down; dirt makes it wilder
            j = int((4 + self.dirt.value * 2) * (0.5 + rand() / 2))
            j = j if self._left % 2 else -j
            p.extend((x + _DX // 2, max(_TOP, min(_BOT, (y + ny) // 2 + j)),
                      x + _DX, ny))
        else:                                       # the squeal: a spike up or down
            far = 12 + self.squeal.value * 8
            ny = y - far if (y - far >= _TOP and (rand() < 0.5 or y + far > _BOT)) else y + far
            self._n += -self.squeal.value if ny > y else self.squeal.value
            ny = max(_TOP, min(_BOT, ny))
            p.extend((x + 8, y + (ny - y) // 4, x + 14, ny))    # it curls, then shoots off
            self._hold = 2 * self.step_ms           # the squeal rings for two steps
        if live:
            self.note(self._n)
            self._ink = theme.HUES[self.hue][0]
            self._pieces(len(p) // 2 - 3, len(p) // 2 - 1, self._ink)

    def _pieces(self, a, b, c):
        p = self._pts
        for k in range(a, min(b, len(p) // 2 - 1)):
            gfx.line(self.d1, p[2 * k], p[2 * k + 1], p[2 * k + 2], p[2 * k + 3], c)

    def draw_main(self):
        d = self.d1
        for i in range(17):                         # a ruler of beats along the bottom
            d.fill_rect(15 + i * 18, tune.BOT - 10, 2, 6 if i % 4 == 0 else 3, theme.SAND)
        self._pieces(0, 99, self._ink)

    def _repaint(self, c):
        self._paint = [c, 0]

    def on_step(self):
        if self._pts or self._left or self.steps % 2:
            return
        b = self.bursts.value
        if rand() < (b * b + 4) / 110:
            self._start(self.size.value)

    def frame(self, ms):
        if self._paint:                             # a few pieces a frame, in order
            c, k = self._paint
            self._pieces(k, k + 3, c)
            self._paint[1] = k + 3
            if k + 3 >= len(self._pts) // 2 - 1:
                self._paint = None
                self._ink = c
                if c == theme.GROUND:
                    self._pts = []
                    if not self.playing:
                        self._rest()
            return
        if not self.playing:
            return
        if self._left:
            self._wait -= ms
            if self._wait <= 0:
                self._wait += self.step_ms // 2
                self._next()
        elif self._hold:
            self._hold = max(0, self._hold - ms)
            if not self._hold:                      # over: grey first, rubbed out later
                self._repaint(theme.GROUND if self._ink == theme.SAND else theme.SAND)
                if self._ink != theme.SAND:
                    self._hold = _TRAIL_MS

    def _rest(self):
        """Paused: leave a whole line on show, in white."""
        if not self._pts:
            self._start(self.size.value)
            while self._left:
                self._next(False)
        self._repaint(theme.WHITE)

    def play_changed(self):
        self._left = 0
        self._hold = _TRAIL_MS if self.playing and self._pts else 0
        if not self.playing and not (self._paint and self._paint[0] == theme.GROUND):
            self._rest()            # (a line being rubbed out is finished first)
