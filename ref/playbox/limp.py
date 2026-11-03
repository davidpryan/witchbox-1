"""Toy 45 (Witchbox, drums): Limp.

A drunk kick and snare. The kick comes on beats 1 and 3 but rushes in
early; the snare comes on beats 2 and 4 but drags in late. A second,
lighter kick follows each snare at a spot picked by Swing grid: half
way, two thirds or three fifths of the way to the next kick. Now and then a hit is
dropped altogether (Stumble).

Screen 1 is a white metronome arm. It swings left for the kick and
right for the snare, hurrying to the one and waiting for the other, so
you can see the beat limp. The arm flashes pink on each hit.
"""
from math import sqrt
import theme, gfx, fonts, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_PX, _PY = 160, 200                 # the pivot
_LEN = 138                          # arm length
_REACH = 48                         # how far the tip swings to each side
_SWING = (500, 667, 600)            # where the extra kick falls in its beat, per mille
_FLASH = 110


class Limp(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.drag = Param("Drag", 0, 10, 6, help="How late the snare is")
        self.rush = Param("Rush", 0, 10, 4, help="How early the kick is")
        self.grid = Param("Swing grid", 0, 2, 1, choices=("Straight", "Triplet", "Quint"),
                          help="Where the in-between kick falls")
        self.stumble = Param("Stumble", 0, 10, 2, help="How often a hit is dropped")
        self._due = []              # hits on their way: [ms to go, voice]
        self._at = 0                # where the arm is, -1000 (left) .. 1000 (right)
        self._to = 0                # where it is heading
        self._hold = 0              # ms it waits before setting off
        self._left = 0              # ms it has to get there
        self._hot = 0               # ms of pink left
        self._tip = None            # the arm as drawn: (x, y, colour)
        return (self.drag, self.rush, self.grid, self.stumble)

    def randomise(self):
        self.drag.value = int(rand() * 11)
        self.rush.value = int(rand() * 11)
        self.grid.value = int(rand() * 3)
        self.stumble.value = int(rand() * 11)

    def _arm(self):
        """Redraw the arm if its tip or colour changed: rub out the old
        line only, then draw the new one and the pivot over its foot."""
        d = self.d1
        x = _PX + self._at * _REACH // 1000
        y = _PY - int(sqrt(_LEN * _LEN - (x - _PX) * (x - _PX)))
        now = (x, y, theme.HUES[self.hue][0] if self._hot else theme.WHITE)
        was = self._tip
        if now == was:
            return
        if was:
            gfx.line(d, _PX, _PY, was[0], was[1], theme.GROUND, 3)
            d.fill_rect((_PX + was[0]) // 2 - 6, (_PY + was[1]) // 2 - 4, 15, 9, theme.GROUND)
        gfx.line(d, _PX, _PY, x, y, now[2], 3)
        d.fill_rect((_PX + x) // 2 - 6, (_PY + y) // 2 - 4, 15, 9, now[2])     # the weight
        d.blit(_PX - 4, _PY - 4, 11, 11, gfx.dot_sprite(5, theme.WHITE))
        self._tip = now

    def draw_main(self):
        d = self.d1
        gfx.text_centre(d, _PX - _REACH - 34, 50, "Kick", fonts.LABEL, theme.SOFT, theme.GROUND)
        gfx.text_centre(d, _PX + _REACH + 36, 50, "Snare", fonts.LABEL, theme.SOFT, theme.GROUND)
        d.fill_rect(_PX - 60, _PY + 8, 121, 3, theme.SOFT)      # the base it stands on
        self._tip = None
        self._arm()

    def play_changed(self):
        self._due = []
        self._to = self._at = self._hold = self._left = self._hot = 0
        self._arm()

    def _send(self, ms, voice):
        if rand() >= self.stumble.value / 25:       # else it stumbles: no hit
            self._due.append([ms, voice])

    def on_step(self):
        if self.steps % 4:
            return
        # Once a beat, send off the hits that fall before the next beat
        # comes round: the next beat's own hit (early if it is a kick, late
        # if a snare) and, after a snare, the in-between kick.
        beat = self.steps // 4 % 4
        beat_ms = 4 * self.step_ms
        early = self.rush.value * beat_ms // 40
        late = self.drag.value * beat_ms // 40
        if self.steps == 0:
            self._send(0, 0)                        # nothing came before the first kick
        if beat % 2:                                # a snare beat; a kick comes next
            gap = beat_ms - early - late            # from the late snare to the early kick
            self._send(late + _SWING[self.grid.value] * gap // 1000, 0)
            self._send(beat_ms - early, 0)
            self._to, self._hold, self._left = -1000, late, beat_ms - early - late
        else:                                       # a kick beat; a snare comes next
            self._send(beat_ms + late, 1)
            self._to, self._hold, self._left = 1000, 0, beat_ms
        self._due.sort()

    def frame(self, ms):
        if self._hot:
            self._hot = max(0, self._hot - ms)
        if self.playing:
            for hit in self._due:
                hit[0] -= ms
            while self._due and self._due[0][0] <= 0:
                if self._due.pop(0)[1]:
                    self.note(0, 1)
                else:
                    self.kick()
                self._hot = _FLASH
            if self._hold > 0:                      # waiting for the late snare
                self._hold -= ms
            elif self._left > 0:
                go = min(ms, self._left)
                self._at += (self._to - self._at) * go // self._left
                self._left -= go
        self._arm()
