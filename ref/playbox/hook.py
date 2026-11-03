"""Toy 58 (Witchbox, lead): Hook Machine.

It writes a two-bar riff from chord notes and passing notes, plays it
three times, then changes some of its notes on the fourth pass. After a
set number of these rounds it writes a new riff. The riff is kept as
steps above the chord's root, so it follows the chords.

Screen 1 is a piano roll of the riff: white bars, higher notes higher,
with a red line that sweeps across as it plays. On the fourth pass the
changed notes are red. Four small squares underneath count the passes.
"""
import theme, tune
from settings import Param
from dark import Dark
from toy import rand

_LEN = 32                               # two bars
_X0, _CELL = 16, 9
_LOW = 186                              # y of the lowest note
_TOP, _H = 38, 160                      # the roll, top and height
_REACH = (4, 7, 9, 14)


class Hook(Dark):
    def make_params(self):
        self.busy = Param("Busy", 1, 10, 5, help="How many notes are in the riff")
        self.reach = Param("Range", 0, 3, 1, choices=("Fifth", "Octave", "Tenth", "2 octaves"),
                           help="How far the riff roams")
        self.vary = Param("Vary", 0, 10, 5, help="How much the fourth pass changes")
        self.rounds = Param("Repeats", 1, 8, 2, help="Rounds before it writes a new riff")
        self._riff = [None] * _LEN      # the note that starts on each step
        self._alt = [None] * _LEN       # the fourth pass's changed notes
        self._hold = [0] * _LEN         # how many steps each note lasts
        self._own = [-1] * _LEN         # which note's bar covers each step
        self._line = -1                 # the step the sweep line is on
        self._pass = 0
        self._made = None               # busy and range the riff was written for
        self._write()
        return (self.busy, self.reach, self.vary, self.rounds)

    def randomise(self):
        self.busy.value = 1 + int(rand() * 10)
        self.reach.value = int(rand() * 4)
        self.vary.value = int(rand() * 11)
        self.rounds.value = 1 + int(rand() * 4)
        self._made = None

    def param_changed(self):
        if (self.busy.value, self.reach.value) != self._made:
            self._write()
            self.draw_main()

    def _write(self):
        """A new riff: pick the steps, then walk a tune through them."""
        self._made = (self.busy.value, self.reach.value)
        top = _REACH[self.reach.value]
        want = 2 + (self.busy.value * 7) // 5
        on = [False] * _LEN
        on[0] = True
        while want > 1:                 # beats are likeliest, then half beats
            s = int(rand() * _LEN)
            if not on[s] and rand() < (1.0 if s % 4 == 0 else 0.6 if s % 2 == 0 else 0.25):
                on[s] = True
                want -= 1
        n = (0, 2, 4)[int(rand() * 3)]
        last = 0
        for s in range(_LEN):
            self._riff[s] = self._alt[s] = None
            self._hold[s] = 0
            if on[s]:
                n += (int(rand() * 3) + 1) * (1 if rand() < 0.5 else -1)
                if n < 0 or n > top:    # bounce off the ends of its range
                    n = -n if n < 0 else 2 * top - n
                if s % 4 == 0:          # on the beat it lands on a chord note
                    n = max(0, min(top, tune.nearest(n, 0)))
                self._riff[s] = n
                last = s
            if self._hold[last] < 4:    # a note lasts until the next, a beat at most
                self._hold[last] += 1
                self._own[s] = last
            else:
                self._own[s] = -1

    def _change(self):
        """The fourth pass: move some of the notes."""
        top = _REACH[self.reach.value]
        for s in range(_LEN):
            if self._riff[s] is not None and rand() < self.vary.value / 12:
                n = self._riff[s] + (int(rand() * 3) + 1) * (1 if rand() < 0.5 else -1)
                n = max(0, min(top, n))
                if n != self._riff[s]:
                    self._bar(s, False, theme.GROUND)
                    self._alt[s] = n
                    self._bar(s, True)

    def _restore(self):
        for s in range(_LEN):
            if self._alt[s] is not None:
                self._bar(s, True, theme.GROUND)
                self._alt[s] = None
                self._bar(s, False)

    def _bar(self, s, alt, c=None):
        """One note's bar: the riff's in white, or its changed one in red."""
        n = self._alt[s] if alt else self._riff[s]
        if c is None:
            c = theme.HUES[self.hue][0] if alt else theme.WHITE
        y = _LOW - n * 140 // _REACH[self.reach.value]     # the range fills the roll
        self.d1.fill_rect(_X0 + s * _CELL, y, self._hold[s] * _CELL - 2, 6, c)

    def _pips(self):
        for i in range(4):
            c = theme.HUES[self.hue][0] if (i == self._pass and self.playing) else theme.SAND
            self.d1.fill_rect(134 + i * 14, tune.BOT - 14, 10, 6, c)

    def draw_main(self):
        d = self.d1
        d.fill_rect(_X0, _TOP, _LEN * _CELL + 2, _H, theme.GROUND)
        for s in range(_LEN):
            if self._riff[s] is not None:
                self._bar(s, self._alt[s] is not None)
        if self._line >= 0:
            d.fill_rect(_X0 + self._line * _CELL, _TOP, 2, _H, theme.HUES[self.hue][0])
        self._pips()

    def _sweep(self, s):
        """Move the line: rub out its thin column, mend the bar it crossed."""
        d = self.d1
        was = self._line
        if was >= 0:
            d.fill_rect(_X0 + was * _CELL, _TOP, 2, _H, theme.GROUND)
            o = self._own[was]
            if o >= 0:
                self._bar(o, self._alt[o] is not None)
        self._line = s
        if s >= 0:
            d.fill_rect(_X0 + s * _CELL, _TOP, 2, _H, theme.HUES[self.hue][0])

    def on_step(self):
        s = self.steps % _LEN
        if s == 0:
            p = (self.steps // _LEN) % 4
            if p == 0:
                self._restore()
                if self.steps and (self.steps // (4 * _LEN)) % self.rounds.value == 0:
                    self._line = -1
                    self._write()
                    self.draw_main()
            elif p == 3:
                self._change()
            self._pass = p
            self._pips()
        self._sweep(s)
        n = self._alt[s] if self._alt[s] is not None else self._riff[s]
        if n is not None:
            self.note(7 + self.root() + n)

    def play_changed(self):
        if not self.playing:
            self._sweep(-1)
            self._restore()
        self._pass = 0
        self._pips()
