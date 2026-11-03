"""Toy 80 (Witchbox, conductor): Arc.

Makes no notes of its own: it leads the band through a song of six
sections, intro, build, drop, breakdown, drop and outro. Shape picks how
loud each section is. At every bar it reads the loudness off the line and
sets how busy each seat of the band may be, so parts come in, thin out
and return by themselves. Drums follow the line closely, bass a little
less, chords stay fuller, a lead only plays in the louder half, voices
and textures are fullest when it is quiet. Other conductors are left
alone. With Loop on Once the song ends with everyone at full.

Screen 1 is a white line graph of the song with the section names under
it and a green dot riding along it. Four small bars on the right show how
busy each seat is now. Played alone it works on four pretend seats.
"""
import theme, gfx, fonts, settings
from settings import Param, BAR
from dark import Dark
from toy import rand

_NAMES = ("intro", "build", "drop", "break", "drop", "outro")
_SHAPES = ((15, 35, 70, 50, 100, 40),       # slow burn
           (30, 60, 100, 25, 100, 20),      # two drops
           (70, 30, 90, 35, 100, 40))       # waves
_BARS = (4, 8, 16)
_ALONE = ("drums", "bass", "chords", "lead")
_X0, _SW = 10, 40               # graph's left edge, width of a section
_GW = 6 * _SW
_YLO, _YHI = 176, 48            # y of level 0 and of level 100
_BX = 264                       # the seat bars' left edge


class Arc(Dark):

    def make_params(self):
        self.shape = Param("Shape", 0, 2, 1, choices=("Slow burn", "Two drops", "Waves"),
                           help="How the song rises and falls")
        self.size = Param("Section", 0, 2, 0, choices=("4 bars", "8 bars", "16 bars"),
                          help="How long each section lasts")
        self.contrast = Param("Contrast", 0, 10, 7, help="How far quiet and loud are apart")
        self.loop = Param("Loop", 0, 1, 1, choices=("Once", "Again"),
                          help="Stop after one song, or go round again")
        self._ys = bytearray(_GW + 1)   # the line's y at every x of the graph
        self._made = None                # the (shape, contrast) the line was worked out for
        self._dot = None                # the marker's place on screen
        self._seen = [None] * 4         # each seat bar's level on screen
        self._line()
        return (self.shape, self.size, self.contrast, self.loop)

    def randomise(self):
        self.shape.value = int(rand() * 3)
        self.size.value = int(rand() * 3)
        self.contrast.value = 3 + int(rand() * 8)
        self.loop.value = int(rand() * 2)

    def _line(self):
        """Work out the line: flat to the middle of the first section, then
        straight from the middle of each section to the next."""
        k = self.contrast.value
        lv = [100 - (100 - v) * k // 10 for v in _SHAPES[self.shape.value]]
        for x in range(_GW + 1):
            i = max(0, min(4, (x - _SW // 2) // _SW))
            f = max(0, min(_SW, x - _SW // 2 - i * _SW))
            v = lv[i] + (lv[i + 1] - lv[i]) * f // _SW
            self._ys[x] = _YLO - (_YLO - _YHI) * v // 100
        self._made = (self.shape.value, k)

    def _stroke(self, a, b):
        """Draw the line from graph x = a to b, level stretches as one write."""
        d = self.d1
        ys = self._ys
        x = a
        while x <= b:
            y = ys[x]
            n = 1
            while x + n <= b and ys[x + n] == y:
                n += 1
            y2 = ys[min(_GW, x + n)]                # then join up to the next column
            if n > 1 or y2 == y:
                d.fill_rect(_X0 + x, y, n, 2, theme.WHITE)
            if y2 != y:
                d.fill_rect(_X0 + x + n - 1, min(y, y2), 2, abs(y2 - y) + 2, theme.WHITE)
            x += n

    def _where(self):
        """How far through the song we are, as an x on the graph."""
        total = 6 * _BARS[self.size.value] * BAR
        t = self.now() if self.playing else 0
        if self.loop.value == 0 and t >= total:
            return _GW                              # played once: stay at the end
        return (t % total) * _GW // total

    def _marker(self):
        x = self._where()
        at = (_X0 + x - 4, self._ys[x] - 3)
        if at == self._dot:
            return
        d = self.d1
        if self._dot:                               # rub out, then mend the line
            d.fill_rect(self._dot[0], self._dot[1], 9, 9, theme.GROUND)
            a = self._dot[0] - _X0
            self._stroke(max(0, a - 2), min(_GW - 1, a + 9))
        d.blit(at[0], at[1], 9, 9, gfx.dot_sprite(4, theme.HUES[self.hue][0]))
        self._dot = at

    def _seats(self):
        for i in range(4):
            v = settings.levels[i]
            if v != self._seen[i]:
                h = v * (_YLO - _YHI) // 100
                x = _BX + i * 13
                self.d1.fill_rect(x, _YHI, 9, _YLO - _YHI - h, theme.SAND)
                self.d1.fill_rect(x, _YLO - h, 9, h, theme.WHITE)
                self._seen[i] = v

    def draw_main(self):
        d = self.d1
        d.fill_rect(0, _YHI - 6, _BX - 4, _YLO - _YHI + 14, theme.GROUND)
        self._stroke(0, _GW - 1)
        for i in range(6):
            gfx.text_centre(d, _X0 + i * _SW + _SW // 2, 194, _NAMES[i], fonts.LABEL,
                            theme.SOFT, theme.GROUND)
        for i in range(4):
            gfx.text_centre(d, _BX + i * 13 + 4, 194, "%d" % (i + 1), fonts.LABEL,
                            theme.SOFT, theme.GROUND)
        self._dot = None
        self._seen = [None] * 4
        self._marker()
        self._seats()

    def _set(self):
        """Give every seat its level for where the song is now."""
        x = self._where()
        v = (_YLO - self._ys[x]) * 100 // (_YLO - _YHI)
        if self.loop.value == 0 and x == _GW:
            v = 100                                 # the song is over: all in
        roles = settings.roles if settings.band else _ALONE
        for i in range(settings.SEATS):
            job = roles[i]
            if job == "conductor":
                continue
            if job == "bass":
                n = (v * 3 + 100) // 4
            elif job == "chords":
                n = (v + 100) // 2
            elif job == "lead":
                n = v if v >= 50 else 0
            elif job in ("voice", "texture"):
                n = 100 - v // 4
            else:                                   # drums, and anything unknown
                n = v
            settings.levels[i] = 100 if v >= 100 else n

    def close(self):
        for i in range(settings.SEATS):
            settings.levels[i] = 100

    def param_changed(self):
        if (self.shape.value, self.contrast.value) != self._made:
            self._line()
            self.draw_main()
        self.play_changed()

    def play_changed(self):
        if self.playing:
            self._set()
        else:
            self.close()                            # paused: everyone back to full
        self._marker()
        self._seats()

    def on_step(self):
        if self.now() % BAR == 0:
            self._set()
            self._seats()
        self._marker()
