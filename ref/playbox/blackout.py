"""Toy 78 (Witchbox, conductor): Blackout.

Makes no notes of its own: it cuts other parts of the band dead for a
moment. Every 4, 8 or 16 bars the last stretch before the bar line goes
silent for the seats it picks, and everything slams back on the bar line.
Depth is how many seats drop: drums go first, then bass, chords and lead.
Voices, textures and other conductors are never cut. A noise swell rises
from two beats before the cut until the return.

Screen 1 is four upright bars, one for each seat of the band, with the
seat's job under it. A bar is white, grey if the seat is empty, and goes
dark while the seat is cut. A thin purple line climbs the screen during
the swell; Riser sets how high it gets. Played alone it shows four
pretend seats.
"""
import theme, gfx, fonts, tune, settings
from settings import Param, BAR
from dark import Dark
from toy import rand

_EVERY = (4, 8, 16)
_LONG = (4, 8, 16, 32)          # the cut's length in steps
_ORDER = ("drums", "bass", "chords", "lead", None)      # who is cut first
_ALONE = ("drums", "bass", "chords", "lead")
_Y0, _Y1 = 44, 196              # top and bottom of the bars
_BW = 44
_GAPS = ((4, 20), (75, 22), (149, 22), (223, 22), (297, 20))    # x, width beside the bars


class Blackout(Dark):

    def make_params(self):
        self.every = Param("Every", 0, 2, 0, choices=("4 bars", "8 bars", "16 bars"),
                           help="How often the band is cut")
        self.long = Param("Length", 0, 3, 1, choices=("1 beat", "2 beats", "1 bar", "2 bars"),
                          help="How long each cut lasts")
        self.depth = Param("Depth", 1, 4, 1, choices=("1", "2", "3", "All"),
                           help="How many parts drop out")
        self.riser = Param("Riser", 0, 10, 6, help="The level of the noise swell")
        self._cutting = False
        self._line = 0              # y of the purple line on screen, 0 = none
        self._shown = [None] * 4    # each bar's colour on screen
        self._names = None          # the jobs written under the bars
        return (self.every, self.long, self.depth, self.riser)

    def randomise(self):
        self.every.value = int(rand() * 3)
        self.long.value = int(rand() * 4)
        self.depth.value = 1 + int(rand() * 4)
        self.riser.value = int(rand() * 11)

    def _roles(self):
        return tuple(settings.roles) if settings.band else _ALONE

    def _let_go(self):
        for i in range(settings.SEATS):
            settings.cuts[i] = False
        self._cutting = False

    def close(self):
        self._let_go()

    def param_changed(self):
        if self._cutting:               # cut the newly chosen seats instead
            self._let_go()
        self._bars()

    def play_changed(self):
        if not self.playing:
            self._let_go()
            self._swell(0)
        self._bars()

    def _cut(self):
        """Silence the first Depth seats that may be cut, drums first."""
        roles = self._roles()
        left = self.depth.value
        for job in _ORDER:
            for i in range(settings.SEATS):
                if roles[i] == job and left:
                    settings.cuts[i] = True
                    left -= 1
        self._cutting = True

    def _bars(self):
        """Redraw any bar whose colour changed, and the jobs if they changed."""
        d = self.d1
        roles = self._roles()
        for i in range(4):
            x = tune.col(i, 4)
            c = theme.PANEL if settings.cuts[i] else theme.WHITE if roles[i] else theme.SAND
            if c != self._shown[i]:
                d.fill_rect(x - _BW // 2, _Y0, _BW, _Y1 - _Y0, c)
                self._shown[i] = c
            if self._names is None or self._names[i] != roles[i]:
                d.fill_rect(x - 36, _Y1 + 6, 72, 14, theme.GROUND)
                gfx.text_centre(d, x, _Y1 + 13, roles[i] or "empty", fonts.LABEL,
                                theme.SOFT, theme.GROUND)
        self._names = roles

    def draw_main(self):
        self._shown = [None] * 4
        self._names = None
        self._bars()
        y, self._line = self._line, 0
        self._swell(y)

    def _swell(self, y):
        """Move the purple line to height y (0 = take it away). It is drawn
        only in the gaps beside the bars, so the bars are never touched."""
        if y == self._line:
            return
        d = self.d1
        for x, w in _GAPS:
            if self._line:
                d.fill_rect(x, self._line, w, 2, theme.GROUND)
            if y:
                d.fill_rect(x, y, w, 2, theme.HUES[self.hue][0])
        self._line = y

    def on_step(self):
        period = _EVERY[self.every.value] * BAR
        cut = _LONG[self.long.value]
        p = self.now() % period
        start = period - cut                    # the cut runs up to the bar line
        rise = start - 8                        # the swell starts two beats earlier
        if (p >= start) != self._cutting:
            if self._cutting:
                self._let_go()                  # the bar line: everyone back in
            else:
                self._cut()
        if p == rise and self.riser.value:
            self.note(0, 0)                     # the noise swell begins
        if p >= rise and self.riser.value:
            top = _Y1 - (_Y1 - _Y0) * self.riser.value // 10
            self._swell(_Y1 - 2 - (_Y1 - 2 - top) * (p - rise + 1) // (period - rise))
        else:
            self._swell(0)
        self._bars()
