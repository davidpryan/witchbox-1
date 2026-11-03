"""Toy 59 (Witchbox, lead): Lost Melody.

A sparse five-note tune heard as if from far away. A short phrase of long
notes, then a long silence. Sometimes the phrase is answered: played again
a fifth lower. Then silence again, and a new phrase.

Screen 1 is a dark skyline with small windows. A window lights white for
each note, higher up for higher notes, and fades slowly; the further away
the tune, the longer the fade. Answers light windows lower down in purple.
With Droop turned up the fading light slips to the window below.
"""
import theme, tune
from settings import Param
from dark import Dark
from toy import rand

_PENTA = (0, 2, 3, 4, 6, 7, 9, 10, 11, 13)      # the tune's notes, as scale steps
_TOWERS = ((6, 38, 96), (48, 30, 138), (82, 44, 70), (130, 34, 152),
           (168, 40, 110), (212, 30, 166), (246, 38, 84), (288, 28, 124))   # x, width, height
_BLACK = theme.HUES["black"][0]


def _floors(h):
    return (h - 12) // 14


class Lost(Dark):
    FRAME_MS = 50

    def make_params(self):
        self.far = Param("Distance", 0, 10, 6, help="How far away it sounds")
        self.gaps = Param("Gaps", 1, 10, 5, help="How long the silences are")
        self.droop = Param("Droop", 0, 10, 4, help="How much each note sags")
        self.answer = Param("Answer", 0, 10, 6, help="How often a phrase is answered")
        self._tune = []                 # the phrase: [steps to wait, note index]
        self._i = 0                     # the next note of it
        self._wait = 0                  # steps until that note
        self._reply = False             # this pass is the answer
        self._lights = []               # [ms left, ms in all, x, y, purple, stage]
        return (self.far, self.gaps, self.droop, self.answer)

    def randomise(self):
        self.far.value = int(rand() * 11)
        self.gaps.value = 1 + int(rand() * 6)
        self.droop.value = int(rand() * 11)
        self.answer.value = int(rand() * 11)

    def _window(self, x, y, c):
        self.d1.fill_rect(x, y, 6, 8, c)

    def draw_main(self):
        d = self.d1
        for x, w, h in _TOWERS:
            d.fill_rect(x, tune.BOT - h, w, h, _BLACK)
            cols = (w - 4) // 10
            for f in range(_floors(h)):
                for c in range(cols):
                    self._window(x + (w - cols * 10 + 4) // 2 + c * 10,
                                 tune.BOT - 18 - f * 14, theme.SAND)
        for q in self._lights:
            self._shine(q)

    def _shine(self, q):
        """Draw a light at its stage: bright, then dim."""
        if q[5] == 0:
            c = theme.HUES[self.hue][0] if q[4] else theme.WHITE
        else:
            c = theme.HUES[self.hue][1] if q[4] else theme.SOFT
        self._window(q[2], q[3], c)

    def _light(self, floor, purple):
        """Light a window on this floor, in any tower tall enough."""
        while True:
            x, w, h = _TOWERS[int(rand() * len(_TOWERS))]
            if _floors(h) > floor:
                break
        cols = (w - 4) // 10
        x += (w - cols * 10 + 4) // 2 + int(rand() * cols) * 10
        y = tune.BOT - 18 - floor * 14
        ms = 700 + self.far.value * 260
        for q in self._lights:          # already lit: start it again
            if q[2] == x and q[3] == y:
                self._lights.remove(q)
                break
        if len(self._lights) >= 8:
            self._window(self._lights[0][2], self._lights[0][3], theme.SAND)
            self._lights.pop(0)
        q = [ms, ms, x, y, purple, 0]
        self._lights.append(q)
        self._shine(q)

    def _phrase(self):
        """Three to five notes that wander by a step or two."""
        i = int(rand() * 4) + 3
        for _ in range(4):              # try to start on a note of the chord
            if (_PENTA[i] - self.root()) % 7 in (0, 2, 4):
                break
            i = int(rand() * 4) + 3
        self._tune = []
        for k in range(3 + int(rand() * 3)):
            self._tune.append([2 + int(rand() * 3) * 2, i])
            i = max(2, min(9, i + int(rand() * 5) - 2))

    def on_step(self):
        if self._wait > 0:
            self._wait -= 1
            return
        if self._i >= len(self._tune):  # the phrase is over
            self._i = 0
            if not self._reply and self._tune and rand() < self.answer.value / 10:
                self._reply = True
            else:
                self._reply = False
                self._phrase()
        gap, i = self._tune[self._i]
        self._i += 1
        if self._reply:
            self.note(7 + _PENTA[i] - 4)            # a fifth lower
            self._light(i - 2, True)
        else:
            self.note(7 + _PENTA[i])
            self._light(i, False)
        self._wait = gap - 1
        if self._i >= len(self._tune):              # then the long silence
            self._wait += self.gaps.value * (3 + int(rand() * 3))

    def frame(self, ms):
        out = False
        for q in self._lights:
            q[0] -= ms
            if q[5] == 0 and q[0] * 2 < q[1]:       # half way: go dim
                q[5] = 1
                low = q[3] + 14
                if self.droop.value > 4 and low < tune.BOT - 10:    # and sag a floor
                    self._window(q[2], q[3], theme.SAND)
                    q[3] = low
                self._shine(q)
            elif q[0] <= 0:
                self._window(q[2], q[3], theme.SAND)
                out = True
        if out:
            self._lights = [q for q in self._lights if q[0] > 0]

    def play_changed(self):
        if self.playing:
            self._i = len(self._tune)
            self._wait = 0
            self._reply = True          # so the first thing heard is a new phrase
