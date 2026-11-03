"""Toy 79 (Witchbox, conductor): Screw.

Makes no notes of its own: it slows the clock that every toy runs on, and
tells the sound how far to drop in pitch. Drag is a steady slow-down, down
to three quarters speed (about a fourth lower). Stops pulls a tape stop
just before every 16th, 8th or 4th bar: the speed sinks to nothing over
Stop time, hangs for a moment and snaps back. Waver is a slow, unsteady
drift of a few percent.

While the tape is stopped no clock steps arrive at all, so the stop is
timed in real milliseconds in frame(), never by counting steps.

Screen 1 is a tape reel: a ring of dots with three spokes that turn at the
current speed. The spokes turn purple as the speed sinks and stand still
on a tape stop. The speed is written underneath.
"""
from math import sin, cos, pi
import theme, gfx, fonts, settings
from settings import Param, BAR
from dark import Dark
from toy import rand

_STOPS = (0, 16, 8, 4)          # a tape stop every so many bars (0 = never)
_TIME = (4, 8, 16)              # Stop time in steps
_HOLD_MS = 300                  # how long the tape stands still
_CX, _CY = 160, 106
_RIM, _ARM, _DOT = 64, 38, 7    # rim radius, spoke radius, spoke dot size
_PALE = 0xC57D                  # half way from white to purple


class Screw(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.drag = Param("Drag", 0, 10, 3, help="A steady slow-down and drop in pitch")
        self.stops = Param("Stops", 0, 3, 3, choices=("Never", "16 bars", "8 bars", "4 bars"),
                           help="How often the tape stops dead")
        self.time = Param("Stop time", 0, 2, 0, choices=("1 beat", "2 beats", "1 bar"),
                          help="How long the slow-down takes")
        self.waver = Param("Waver", 0, 10, 2, help="A slow unsteady drift in speed")
        self._state = 0             # 0 running, 1 slowing to a stop, 2 stopped
        self._t = 0                 # ms into the slow-down or the stop
        self._len = 1               # ms the slow-down takes
        self._armed = True          # no stop pulled yet for the coming bar line
        self._drift = 0             # ms along the waver
        self._turn = 0.0            # the reel's angle
        self._at = [None] * 3       # where each spoke is drawn: (x, y, colour)
        self._said = None           # the number on screen
        return (self.drag, self.stops, self.time, self.waver)

    def randomise(self):
        self.drag.value = int(rand() * 11)
        self.stops.value = int(rand() * 4)
        self.time.value = int(rand() * 3)
        self.waver.value = int(rand() * 11)

    def _base(self):
        return 100 - self.drag.value * 5 // 2

    def _free(self):
        """Out of any tape stop, back to the steady speed (full when paused)."""
        self._state = 0
        settings.speed = self._base() if self.playing else 100
        settings.drop = self.drag.value if self.playing else 0

    def close(self):
        settings.speed = 100
        settings.drop = 0

    def param_changed(self):
        if self._state == 0 or not self.playing:
            self._free()

    def play_changed(self):
        self._armed = True
        self._free()

    def handle(self, ev):
        r = Dark.handle(self, ev)
        if self._help:              # frame() rests while help is open: never
            self._free()            # leave the tape stopped behind it
        return r

    def on_step(self):
        bars = _STOPS[self.stops.value]
        if not bars:
            return
        period = bars * BAR
        # The clock covers half its usual ground while it slows to nothing,
        # so starting half the Stop time early lands the stop on the bar line.
        early = _TIME[self.time.value] // 2
        late = self.now() % period >= period - early
        if late and self._armed and self._state == 0:
            self._state = 1
            self._t = 0
            self._len = _TIME[self.time.value] * 1500000 // (settings.TEMPO.value * self._base())
        self._armed = not late

    def frame(self, ms):
        if self.playing:
            base = self._base()
            if self._state == 1:                    # slowing down
                self._t += ms
                speed = base * (self._len - self._t) // self._len
                if speed <= 0:
                    speed = 0
                    self._state = 2
                    self._t = 0
            elif self._state == 2:                  # standing still
                self._t += ms
                speed = 0
                if self._t >= _HOLD_MS:
                    self._state = 0
                    speed = base                    # snap back
            else:
                self._drift = (self._drift + ms) % 6000
                speed = base + int(sin(self._drift * pi / 3000) * self.waver.value * 0.4)
            settings.speed = max(0, int(speed))
            self._turn = (self._turn + ms * settings.speed * 0.00003) % (2 * pi)
        self._spokes()
        self._number()

    def _spokes(self):
        d = self.d1
        v = settings.speed
        c = theme.WHITE if v >= 95 else _PALE if v >= 70 else theme.HUES[self.hue][0]
        for i in range(3):
            a = self._turn + i * 2 * pi / 3
            now = (_CX + int(_ARM * sin(a) + 100.5) - 100, _CY - int(_ARM * cos(a) + 100.5) + 100, c)
            was = self._at[i]
            if now != was:
                if was:
                    d.fill_rect(was[0] - _DOT, was[1] - _DOT, 2 * _DOT + 1, 2 * _DOT + 1, theme.GROUND)
                d.blit(now[0] - _DOT, now[1] - _DOT, 2 * _DOT + 1, 2 * _DOT + 1, gfx.dot_sprite(_DOT, c))
                self._at[i] = now

    def _number(self):
        s = "%d%%" % settings.speed
        if s != self._said:
            self.d1.fill_rect(_CX - 40, 186, 80, 24, theme.GROUND)
            gfx.text_centre(self.d1, _CX, 198, s, fonts.HERO, theme.TEXT, theme.GROUND)
            self._said = s

    def draw_main(self):
        d = self.d1
        for i in range(12):                         # the rim, drawn once
            a = i * pi / 6
            gfx.disc(d, _CX + int(_RIM * sin(a)), _CY - int(_RIM * cos(a)), 3, theme.WHITE)
        gfx.disc(d, _CX, _CY, 9, theme.WHITE)       # the hub
        self._at = [None] * 3
        self._said = None
        self._spokes()
        self._number()
