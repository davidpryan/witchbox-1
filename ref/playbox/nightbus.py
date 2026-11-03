"""Toy 70 (Witchbox, texture): Night Bus.

The city at night, heard from far off. A low hum sits under everything and
follows the chords. Cars pass now and then. Rare events break the quiet: a
siren far away, its two notes taken from the chord so it is in tune; a
train going by; a door.

Screen 1 is a street seen from the side: a row of white street lamps on a
kerb, the road below, a railway line above. Each passing car is a pair of
blue headlights crossing the road; with more Distance they are smaller,
slower and further up the road. A light flashes in the sky during a siren,
lit windows run along the railway for a train, and a doorway lights up for
a door. A thicker kerb means more Hum.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_KERB = 168
_RAIL = 96
_SKY = (262, 54)                # where the far-off siren light flashes
_GAP = 16                       # between a car's two headlights


class NightBus(Dark):
    FRAME_MS = 40

    def make_params(self):
        self.traffic = Param("Traffic", 0, 10, 5, help="How often a car passes")
        self.events = Param("Events", 0, 10, 4, help="How often a siren, train or door is heard")
        self.far = Param("Distance", 0, 10, 3, help="How far away everything sounds")
        self.hum = Param("Hum", 0, 10, 5, help="The level of the low hum")
        self._cars = []             # each [x, y, speed, radius]; speed < 0 goes left
        self._siren = 0             # siren notes still to come
        self._flash = False         # is the sky light on
        self._train = None          # x of the train's first window, or None
        self._door = 0              # ms the doorway stays lit
        self._was = 3               # Distance when the cars were set going
        return (self.traffic, self.events, self.far, self.hum)

    def randomise(self):
        self.traffic.value = int(rand() * 11)
        self.events.value = int(rand() * 11)
        self.far.value = int(rand() * 11)
        self.hum.value = int(rand() * 11)

    def param_changed(self):
        self._curb()
        if self._cars and self.far.value != self._was:  # the road moves: start it clear
            self.d1.fill_rect(0, _KERB + 6, 320, 46, theme.GROUND)
            self._cars = []
        self._was = self.far.value

    def _curb(self):
        d = self.d1
        d.fill_rect(0, _KERB, 320, 5, theme.GROUND)
        d.fill_rect(0, _KERB, 320, 1 + self.hum.value * 2 // 5, theme.SAND)

    def draw_main(self):
        d = self.d1
        self._curb()
        d.fill_rect(0, _RAIL, 320, 1, theme.SAND)
        for i in range(5):                          # a pole, an arm and a lamp each
            x = tune.col(i, 5)
            d.fill_rect(x, _KERB - 54, 2, 54, theme.WHITE)
            d.fill_rect(x, _KERB - 56, 14, 2, theme.WHITE)
            d.fill_rect(x + 8, _KERB - 54, 8, 4, theme.WHITE)

    def _wipe(self):
        """Take every moving thing off the screen."""
        d = self.d1
        d.fill_rect(0, _KERB + 6, 320, 46, theme.GROUND)            # the road
        d.fill_rect(0, _RAIL - 8, 320, 6, theme.GROUND)             # the railway
        d.fill_rect(_SKY[0] - 4, _SKY[1] - 4, 9, 9, theme.GROUND)
        d.fill_rect(28, _KERB - 22, 10, 22, theme.GROUND)           # the doorway
        self._cars = []
        self._train = None
        self._siren = self._door = 0
        self._flash = False

    def play_changed(self):
        if not self.playing:
            self._wipe()

    def on_step(self):
        s = self.steps
        if s % BAR == 0 and self.hum.value:
            self.note(self.root() - 14, 0)              # the hum, two octaves down
        if s % 4 == 0 and len(self._cars) < 2 and rand() * 20 < self.traffic.value:
            far = self.far.value
            v = 7 - far // 2                            # far cars cross slowly
            left = rand() < 0.5
            y = _KERB + 40 - far * 2
            if self._cars and self._cars[0][1] == y:    # that lane is taken
                y -= 12
            self._cars.append([298 if left else 6, y, -v if left else v, 5 - (far + 1) // 4])
            self.note(0, 2)                             # a car goes by
        if self._siren and s % 2 == 0:                  # two notes, back and forth
            self._siren -= 1
            self._flash = self._siren % 2 == 1
            self.note(self.root() + (4 if self._flash else 2), 1)
            self.d1.blit(_SKY[0] - 3, _SKY[1] - 3, 7, 7, gfx.dot_sprite(
                3, theme.HUES[self.hue][0] if self._flash else theme.GROUND))
        if self._train is not None and s % 2 == 0:
            self.note(0, 3)                             # wheels on the rails
        busy = self._siren or self._train is not None or self._door
        if s % BAR == 0 and not busy and rand() * 24 < self.events.value:
            r = rand()
            if r < 0.5:
                self._siren = 16
            elif r < 0.8:
                self._train = -70
            else:
                self.note(0, 4)                         # a door
                self._door = 400
                self.d1.fill_rect(28, _KERB - 22, 10, 22, theme.WHITE)

    def frame(self, ms):
        if not self.playing:
            return
        d = self.d1
        for car in self._cars:
            x, y, v, r = car
            n = 2 * r + 1
            for dx in (0, _GAP):                        # rub out, move on, draw
                d.fill_rect(x + dx - r, y - r, n, n, theme.GROUND)
            x += v
            car[0] = x
            if 6 <= x <= 298:
                buf = gfx.dot_sprite(r, theme.HUES[self.hue][0])
                d.blit(x - r, y - r, n, n, buf)
                d.blit(x + _GAP - r, y - r, n, n, buf)
        self._cars = [c for c in self._cars if 6 <= c[0] <= 298]
        if self._train is not None:                     # five lit windows in a row
            x = self._train
            for i in range(5):
                wx = x + i * 14
                if 6 <= wx <= 320:                      # rub out the back
                    d.fill_rect(wx - 6, _RAIL - 7, 6, 4, theme.GROUND)
                if 0 <= wx <= 312:                      # draw the front
                    d.fill_rect(wx, _RAIL - 7, 8, 4, theme.SOFT)
            self._train = x + 6
            if x > 330:
                self._train = None
                d.fill_rect(0, _RAIL - 8, 320, 6, theme.GROUND)
        if self._door:
            self._door = max(0, self._door - ms)
            if not self._door:
                d.fill_rect(28, _KERB - 22, 10, 22, theme.GROUND)
