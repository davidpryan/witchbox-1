"""Toy 44 (Witchbox, drums): Pocket Change.

Tiny found sounds, like keys, a lighter, scissors and coins, scattered
as ghost percussion. Hits are most likely just before and just after
beats 2 and 4. Lateness lets them fall behind the beat; Kit picks what
is being dropped.

Screen 1: small white things drop from the top onto a line near the
bottom and bounce once. Each one turns orange as it lands, which is the
moment it sounds, then lies there white until newer ones push it out.
Scatter spreads them out to the left and right.
"""
import theme, gfx, tune
from settings import Param, BAR
from dark import Dark
from toy import rand

_FLOOR = 190                        # where a thing's middle is when it lands
_TOP = tune.TOP + 8
_BOUNCE = 260                       # ms the bounce lasts
_KEEP = 8                           # how many things are on screen at most
# how likely each step of the bar is, out of 10: most just round beats 2 and 4
_ODDS = (2, 1, 3, 9, 2, 9, 3, 1, 2, 1, 3, 9, 2, 9, 3, 1)


class Pocket(Dark):
    FRAME_MS = 30

    def make_params(self):
        self.clutter = Param("Clutter", 1, 10, 5, help="How many hits, sparse to busy")
        self.kit = Param("Kit", 0, 3, 0, choices=("Metal", "Wood", "Paper", "Glass"),
                         help="What the clicks are made of")
        self.late = Param("Lateness", 0, 10, 3, help="How far hits fall behind the beat")
        self.scatter = Param("Scatter", 0, 10, 6, help="How widely hits spread left and right")
        self._things = []           # each: [x, ms so far, ms of fall, y drawn or None]
        self._rest()
        return (self.clutter, self.kit, self.late, self.scatter)

    def _rest(self):
        """Paused: three things lie on the line, so the screen says what it is."""
        self._things = [[x, 9999, 1, None] for x in (120, 168, 204)]

    def randomise(self):
        self.clutter.value = 1 + int(rand() * 10)
        self.kit.value = int(rand() * 4)
        self.late.value = int(rand() * 11)
        self.scatter.value = int(rand() * 11)

    def param_changed(self):
        tune.clear(self.d1)         # the kit may have changed every shape
        self.draw_main()

    def _show(self, t):
        """Draw one thing where it should be now."""
        d = self.d1
        x, at, fall = t[0], t[1], t[2]
        if at < fall:               # falling, faster and faster
            y = _TOP + (_FLOOR - _TOP) * at * at // (fall * fall)
        elif at < fall + _BOUNCE:   # one bounce
            u = at - fall
            y = _FLOOR - 28 * 4 * u * (_BOUNCE - u) // (_BOUNCE * _BOUNCE)
        else:                       # lying where it fell
            y = _FLOOR
        c = theme.HUES[self.hue][0] if fall <= at < fall + _BOUNCE else theme.WHITE
        t[3] = y
        kit = self.kit.value
        if kit == 0:                # metal: a coin
            d.blit(x - 4, y - 4, 9, 9, gfx.dot_sprite(4, c))
        elif kit == 1:              # wood: a matchstick
            d.fill_rect(x - 2, y - 5, 4, 10, c)
        elif kit == 2:              # paper: a flat slip
            d.fill_rect(x - 5, y + 1, 11, 4, c)
        else:                       # glass: a hollow bead
            d.fill_rect(x - 4, y - 4, 9, 9, c)
            d.fill_rect(x - 1, y - 1, 3, 3, theme.GROUND)

    def _wipe(self):
        for t in self._things:
            if t[3] is not None:
                self.d1.fill_rect(t[0] - 5, t[3] - 5, 11, 11, theme.GROUND)
                t[3] = None

    def draw_main(self):
        self.d1.fill_rect(12, _FLOOR + 6, 296, 2, theme.SOFT)       # the line they land on
        for t in self._things:
            self._show(t)

    def play_changed(self):
        self._wipe()
        self._things = []
        if not self.playing:
            self._rest()
            self.draw_main()

    def on_step(self):
        # Decide one step ahead, so the thing can be let go now and land
        # (and sound) on the next step, or a little after it if it is late.
        nxt = (self.steps + 1) % BAR
        if rand() * 10 < _ODDS[nxt] * self.clutter.value / 10:
            lag = int(rand() * self.late.value * self.step_ms) // 12
            x = 160 + int((rand() * 2 - 1) * (6 + self.scatter.value * 14))
            self._things.append([x, 0, self.step_ms + lag, None])

    def frame(self, ms):
        if not self.playing:
            return
        # Things may overlap, so rub them all out and draw them all again:
        # at most eight of them, three writes each.
        self._wipe()
        if len(self._things) > _KEEP:       # the oldest one lying there goes
            self._things.pop(0)
        for t in self._things:
            was = t[1]
            t[1] += ms
            if was < t[2] <= t[1]:
                self.note(0, 3 + self.kit.value)        # it lands: the click
            self._show(t)
