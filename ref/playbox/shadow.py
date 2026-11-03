"""Toy 73 (Witchbox, lead): Shadow.

A second line that follows whichever lead is playing in the band. In
Harmony it plays along with each lead note, a third, a sixth or an octave
below it. In Echo it plays each lead note again half a bar, one bar or two
bars later. If no lead has played for two bars (always so when it plays
alone) it follows the top note of the chord on every beat instead.

Screen 1 is two lanes, two bars wide, written left to right like a scope:
the notes it hears on top in white (grey when it is the chord's top note
standing in for a lead), its own notes below in blue. A gap runs just
ahead of the newest notes, wiping the old ones away.
"""
import theme, gfx, fonts, tune, settings
from settings import Param, BAR
from dark import Dark
from toy import rand

_BELOW = (2, 5, 7)              # a third, a sixth, an octave, in scale steps
_WAIT = (8, 16, 32)             # echo delay in steps
_COLS = 32                      # the lanes hold two bars
_X0, _CW = 60, 8                # first column, column width
_LANE = (30, 124)               # top of each lane
_LH = 86                        # lane height
_LO, _HI = -10, 17              # notes a lane can show


class Shadow(Dark):

    def make_params(self):
        self.way = Param("Mode", 0, 1, 0, choices=("Harmony", "Echo"),
                         help="Play along with the lead, or after it")
        self.gap = Param("Interval", 0, 2, 0, choices=("Third", "Sixth", "Octave"),
                         help="How far below the lead it sings")
        self.delay = Param("Delay", 0, 2, 1, choices=("1/2 bar", "1 bar", "2 bars"),
                           help="In Echo, how long after the lead")
        self.thin = Param("Thin", 0, 10, 2, help="The chance that a note is left out")
        self._seen = settings.leads     # the lead count we have dealt with
        self._last = -2 * BAR           # step of the last real lead note
        self._queue = []                # echoes to come: (due step, note)
        self._heard = [None] * _COLS    # what each lane shows: (note, colour)
        self._mine = [None] * _COLS
        return (self.way, self.gap, self.delay, self.thin)

    def randomise(self):
        self.way.value = int(rand() * 2)
        self.gap.value = int(rand() * 3)
        self.delay.value = int(rand() * 3)
        self.thin.value = int(rand() * 6)

    def param_changed(self):
        if self.way.value == 0:
            self._queue = []                # no echoes left over in Harmony

    def play_changed(self):
        self._queue = []
        self._seen = settings.leads
        self._last = -2 * BAR

    def _bar(self, lane, col, cell):
        """One note in a lane, or nothing: the whole column is cleared first."""
        d = self.d1
        x = _X0 + col * _CW
        d.fill_rect(x, _LANE[lane], _CW, _LH, theme.GROUND)
        if cell:
            n = max(_LO, min(_HI, cell[0]))
            d.fill_rect(x, _LANE[lane] + _LH - 5 - (n - _LO) * 3, _CW - 2, 4, cell[1])

    def draw_main(self):
        d = self.d1
        for lane, name in enumerate(("Lead", "Shadow")):
            gfx.text_mid(d, 8, _LANE[lane] + _LH // 2, name, fonts.LABEL, theme.SOFT, theme.GROUND)
            d.fill_rect(_X0, _LANE[lane] + _LH + 1, _COLS * _CW, 1, theme.SAND)
            cells = self._mine if lane else self._heard
            for col in range(_COLS):
                if cells[col]:
                    self._bar(lane, col, cells[col])

    def _sing(self, n, col):
        """Play one of our own notes. We are a lead too, so the note lands on
        the lead bus: note the new count, so we never follow ourselves."""
        if rand() * 12 < self.thin.value:
            return
        self.note(n, 0)
        self._seen = settings.leads
        self._mine[col] = (n, theme.HUES[self.hue][0])
        self._bar(1, col, self._mine[col])

    def on_step(self):
        s = self.steps
        col = s % _COLS
        ahead = (col + 2) % _COLS               # the wipe runs two columns ahead
        for lane, cells in enumerate((self._heard, self._mine)):
            if cells[ahead]:
                cells[ahead] = None
                self._bar(lane, ahead, None)
            if cells[col]:                      # this column's old note goes too
                cells[col] = None
                self._bar(lane, col, None)
        heard = None
        if settings.leads != self._seen:        # a lead toy has played
            self._seen = settings.leads
            self._last = s
            heard = (settings.lead, theme.WHITE)
        elif s - self._last >= 2 * BAR and s % 4 == 0:
            heard = (self.root() + 4, theme.SOFT)   # no lead: the chord's top note
        if heard and heard[0] is not None:
            self._heard[col] = heard
            self._bar(0, col, heard)
            if self.way.value == 0:
                self._sing(heard[0] - _BELOW[self.gap.value], col)
            elif len(self._queue) < 16:
                self._queue.append((s + _WAIT[self.delay.value], heard[0]))
        while self._queue and self._queue[0][0] <= s:
            self._sing(self._queue.pop(0)[1], col)
