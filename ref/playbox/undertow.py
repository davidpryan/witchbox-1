"""Toy 46 (Witchbox, bass): Undertow.

A deep sub bass. A long note on the root of the chord comes in one step
after the start of the bar (just behind the kick) and beats slowly against
itself. It often rests, and only now and then moves up to the fifth.

Screen 1 is a slow wave across the screen. How high it sits is the note,
and its ripple is the wobble. The wave is blue while a note sounds and
settles to a flat white line in the rests.
"""
from math import sin, pi
import theme, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_N = 32                         # the wave is this many short pieces
_W = theme.W // _N
_SIN = tuple(int(64 * sin(2 * pi * i / 64)) for i in range(64))
_HOLD = (4, 8, 16, 32)          # note lengths in steps


class Undertow(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.hold = Param("Hold", 0, 3, 2, choices=("1 beat", "2 beats", "1 bar", "2 bars"),
                          help="How long each note lasts")
        self.wobble = Param("Wobble", 0, 10, 4, help="How fast the note beats, still to fast")
        self.moves = Param("Movement", 0, 10, 2, help="How often it leaves the root")
        self.gaps = Param("Gaps", 0, 10, 4, help="How often it rests")
        self._left = 0              # steps the note still has to sound
        self._want = 150            # where the wave should sit (the note)
        self._lvl = 150             # where it sits now; it eases over
        self._amp = 0               # ripple height now
        self._ph = 0                # how far the ripple has travelled
        self._ys = [0] * _N         # each piece's y on screen (0 = not drawn)
        self._c = theme.WHITE       # the colour on screen
        return (self.hold, self.wobble, self.moves, self.gaps)

    def randomise(self):
        self.hold.value = int(rand() * 4)
        self.wobble.value = int(rand() * 11)
        self.moves.value = int(rand() * 6)
        self.gaps.value = int(rand() * 8)

    def draw_main(self):
        self._ys = [0] * _N
        self._wave()

    def play_changed(self):
        if not self.playing:
            self._left = 0

    def on_step(self):
        i = self.steps % BAR
        if self._left > 0:
            self._left -= 1
        # A note may start one step after beat 1, and after beat 3 for short notes.
        if (i == 1 or (i == 9 and self.hold.value < 2)) and self._left <= 0:
            if rand() < self.gaps.value / 11:
                return                              # a rest
            deg = self.root() - 7
            if rand() < self.moves.value / 20:      # rarely: up to the fifth
                deg += 4
            self.note(deg, 0)
            self._left = _HOLD[self.hold.value]
            self._want = tune.height(deg, -7, 3, 180, 80)

    def frame(self, ms):
        on = self.playing and self._left > 0
        amp = 6 + self.wobble.value * 2 if on else 0
        if self.playing:
            self._ph += ms * self.wobble.value      # the ripple runs with the wobble
        if self._amp != amp:                        # ease the ripple in and out
            self._amp += max(-2, min(2, amp - self._amp))
        if self._lvl != self._want:                 # and slide to the new note
            self._lvl += max(-6, min(6, self._want - self._lvl))
        self._wave(theme.HUES[self.hue][0] if on else theme.WHITE)

    def _wave(self, c=None):
        """Move only the pieces of the wave that are not where they should be."""
        d = self.d1
        if c is None:
            c = self._c
        fresh = c != self._c
        self._c = c
        ph = self._ph // 60
        ys = self._ys
        for i in range(_N):
            y = self._lvl + self._amp * _SIN[(i * 4 + ph) & 63] // 64
            was = ys[i]
            if y != was or fresh:
                if was and was != y:
                    d.fill_rect(i * _W, was - 2, _W, 4, theme.GROUND)
                d.fill_rect(i * _W, y - 2, _W, 4, c)
                ys[i] = y
