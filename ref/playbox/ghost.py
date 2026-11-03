"""Toy 74 (Witchbox, voice): Ghost Voice.

Short sung phrases of two to four notes that come and go at odd moments.
Each phrase starts on a note of the chord, wanders a step or two and ends
on a chord note again. Pitch moves the whole voice up or down by a fifth
or an octave. Glide is how slowly it slides from note to note.

Screen 1 is a mouth in the middle of the screen. It is shut and white
while the voice rests. With each phrase it opens, taller for a longer
phrase, turns pink, and drifts up and down with the notes; Glide sets how
slowly it drifts.
"""
import theme, gfx, tune
from settings import Param
from dark import Dark
from toy import rand

_SHIFT = (-7, -4, 0, 4, 7)      # Pitch, in scale steps
_CX, _CY, _W = 160, 123, 84
_SHUT = 10                      # the mouth's height at rest


class Ghost(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.pitch = Param("Pitch", 0, 4, 2, choices=("-Oct", "-5th", "0", "+5th", "+Oct"),
                           help="Moves the voice down or up")
        self.phrase = Param("Phrase", 2, 4, 3, help="How many notes in each phrase")
        self.glide = Param("Glide", 0, 10, 5, help="How slowly it slides between notes")
        self.haunt = Param("Haunt", 1, 10, 6, help="How often a phrase appears")
        self._todo = []             # notes of the phrase still to sing
        self._wait = 0              # steps until the next note, or of rest
        self._open = 0              # how far the mouth should open, in pixels
        self._to_y = _CY            # where the mouth is drifting to
        self._box = None            # the mouth on screen: (y, height, colour)
        self._y, self._h = _CY, _SHUT
        return (self.pitch, self.phrase, self.glide, self.haunt)

    def randomise(self):
        self.pitch.value = int(rand() * 5)
        self.phrase.value = 2 + int(rand() * 3)
        self.glide.value = int(rand() * 11)
        self.haunt.value = 1 + int(rand() * 10)

    def play_changed(self):
        if not self.playing:
            self._todo = []
            self._open = 0
            self._to_y = _CY

    def draw_main(self):
        self._box = None
        self._mouth()

    def _mouth(self):
        """Draw the mouth only if it moved, changed size or changed colour."""
        d = self.d1
        c = theme.HUES[self.hue][0] if self._open else theme.WHITE
        h = self._h
        y = self._y - h // 2
        if (y, h, c) == self._box:
            return
        if self._box:
            oy, oh = self._box[0], self._box[1]
            if oy < y:                                  # what the old one leaves behind
                d.fill_rect(_CX - _W // 2, oy, _W, y - oy, theme.GROUND)
            if oy + oh > y + h:
                d.fill_rect(_CX - _W // 2, y + h, _W, oy + oh - y - h, theme.GROUND)
        r = min(10, h // 2)
        d.fill_rect(_CX - _W // 2, y, _W, r, theme.GROUND)      # the old mouth may
        d.fill_rect(_CX - _W // 2, y + h - r, _W, r, theme.GROUND)  # lie under the corners
        gfx.rrect(d, _CX - _W // 2, y, _W, h, r, c)
        if h >= 24:                                     # open: the dark inside shows
            gfx.rrect(d, _CX - _W // 2 + 12, y + 8, _W - 24, h - 16, min(6, (h - 16) // 2),
                      theme.GROUND)
        self._box = (y, h, c)

    def on_step(self):
        if self._wait:
            self._wait -= 1
            return
        if self._todo:
            n = self._todo.pop(0)
            self.note(n, 0)
            self._to_y = _CY - max(-12, min(12, n - _SHIFT[self.pitch.value] - 4)) * 3
            self._wait = 1 + int(rand() * 3)            # notes 2 to 4 steps apart
            if not self._todo:
                self._wait += 2                         # the last note hangs on
        elif self._open:                                # the phrase is over
            self._open = 0
            self._to_y = _CY
            self._wait = 3
        elif self.steps % 2 == 0 and rand() * 30 < self.haunt.value:
            root = self.root()
            n = _SHIFT[self.pitch.value] + root + 2 * int(rand() * 4)   # a chord note
            k = self.phrase.value
            self._todo = [n]
            for i in range(1, k):
                n += int(rand() * 5) - 2                # wander a step or two
                self._todo.append(tune.nearest(n, root) if i == k - 1 else n)
            self._open = 14 + 14 * k

    def frame(self, ms):
        want = self._open if self._open else _SHUT
        if self._h != want:                             # open quickly, close a bit slower
            step = 12 if want > self._h else 6
            self._h += max(-step, min(step, want - self._h))
        if self._y != self._to_y:
            step = 1 + (10 - self.glide.value) * 2 // 3     # more Glide, slower drift
            self._y += max(-step, min(step, self._to_y - self._y))
        self._mouth()
