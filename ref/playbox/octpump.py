"""Toy 48 (Witchbox, bass): Octave Pump.

A buzzing bass that hammers the root of the current chord. Off-beat plays
eighth notes, Straight plays sixteenths and Gallop plays a long note and
two short ones to every beat. Notes that fall between the beats may jump
up an octave. The jumps are rolled afresh for every bar.

Screen 1 is one bar as two rows of blocks: low notes on the bottom row,
high notes on the top. A block turns green as it plays. Wider blocks are
longer notes (Gate) and taller blocks are brighter ones (Bite).
"""
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_X0, _PITCH = 8, 19             # sixteen cells across the screen
_ROW = (176, 88)                # middle of the low row and of the high row
_WHEN = ((0, 2), (0, 1, 2, 3), (0, 2, 3))      # steps of each beat that sound
_UP = ((2,), (1, 3), (2, 3))                    # and those that may jump


class OctPump(Dark):
    def make_params(self):
        self.pattern = Param("Pattern", 0, 2, 0, choices=("Off-beat", "Straight", "Gallop"),
                             help="The rhythm it hammers out")
        self.octaves = Param("Octaves", 0, 10, 7, help="How often a note jumps up an octave")
        self.gate = Param("Gate", 1, 10, 5, help="From short stabs to joined notes")
        self.bite = Param("Bite", 0, 10, 5, help="How bright the tone is")
        self._cells = [0] * BAR     # this bar: 0 silent, 1 low, 2 high
        self._shown = [None] * BAR  # what is on screen: (cell, width, height, lit)
        self._lit = -1              # the cell playing now
        self._roll()
        return (self.pattern, self.octaves, self.gate, self.bite)

    def randomise(self):
        self.pattern.value = int(rand() * 3)
        self.octaves.value = int(rand() * 11)
        self.gate.value = 1 + int(rand() * 10)
        self.bite.value = int(rand() * 11)

    def _roll(self):
        """Lay out a bar: which steps sound, and which of them jump."""
        when = _WHEN[self.pattern.value]
        up = _UP[self.pattern.value]
        for i in range(BAR):
            s = i % 4
            if s not in when:
                self._cells[i] = 0
            elif s in up and rand() < self.octaves.value / 10:
                self._cells[i] = 2
            else:
                self._cells[i] = 1

    def param_changed(self):
        self._roll()
        self._sync()

    def draw_main(self):
        d = self.d1
        self._shown = [None] * BAR
        for y in _ROW:                                  # a faint shelf under each row
            d.fill_rect(_X0, y + 21, BAR * _PITCH, 2, theme.SAND)
        self._sync()

    def _sync(self):
        """Redraw only the cells that no longer match."""
        d = self.d1
        w = 5 + self.gate.value * 14 // 10              # 6 .. 19: at 19 the blocks join up
        h = 12 + self.bite.value * 2
        for i in range(BAR):
            new = (self._cells[i], w, h, i == self._lit)
            if new == self._shown[i]:
                continue
            self._shown[i] = new
            x = _X0 + i * _PITCH
            for row in (0, 1):
                d.fill_rect(x, _ROW[row] - 16, _PITCH, 32, theme.GROUND)
                if new[0] == row + 1:
                    c = theme.HUES[self.hue][0] if new[3] else theme.WHITE
                    d.fill_rect(x + (_PITCH - w) // 2, _ROW[row] - h // 2, w, h, c)

    def play_changed(self):
        if not self.playing:
            self._lit = -1
            self._sync()

    def on_step(self):
        i = self.steps % BAR
        if i == 0:
            self._roll()
        cell = self._cells[i]
        self._lit = i if cell else -1
        if cell:
            self.note(self.root() - 7 if cell == 1 else self.root(), 0)
        self._sync()
