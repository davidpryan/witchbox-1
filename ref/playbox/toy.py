"""What every toy does the same way. A toy subclasses Toy (see euclid.py).

Left knob     pick what to change        press: back to the toy box
Right knob    change it                  press: go to the band
PLAY play / pause   SCALE next scale   RANDOM new values   HELP help pages

After the strip's four settings the left knob reaches one more stop, Save.
There, turning the right knob saves the toy as it is now (store.py).

Screen 1 is the toy's own picture; the subclass draws it.
Screen 2 is drawn here: a 32px strip with the toy's name and the shared
scale, sound, tempo and volume, then one knob per toy parameter. The picked
parameter's knob lights up in the toy's hue. Every knob carries a dot at
its value: white on the picked knob, light grey on the others.
Up to five parameters sit in one row of big knobs; more go in a 3-wide grid.

A subclass provides (all optional):
    FRAME_MS             if set, frame(ms) is called about that often
    make_params()        -> tuple of settings.Param
    length()             -> steps in its loop (the clock wraps there)
    param_changed()      after any value changes (fix up limits, mark dirty)
    randomise()          new values for its own params
    draw_main()          draw all of screen 1's body from the toy's state
    on_step()            the clock moved on (only while playing): self.pos is
                         the step within the loop, self.steps counts from Play
    play_changed()       play or pause was pressed
    idle(ms)             every poll, for deferred drawing
    frame(ms)            animation; ms since the last frame

    self.note(degree, voice)   call this when the toy sounds a note (sound.py
                               turns it into a sound)

handle() returns "exit" (toy box) or "band"; the app acts on it.
"""
from math import sin, cos, pi
import random
import theme, gfx, fonts, settings, store, sound, dice

STRIP_H = 32
_NAME_X = 8
_SAVE_HINT = "Turn the right knob to save this toy"

_KEYS_HELP = (
    ("Left knob", "Turn to pick what to change"),
    ("Right knob", "Turn to change it"),
    ("Press left", "Back to the toy box"),
    ("Press right", "Go to the band"),
    ("Play", "Play and pause"),
    ("Scale", "Next scale"),
    ("Random", "New settings and a new scale"),
    ("Help", "Open and close these pages"),
    ("Save", "Pick Save, then turn the right knob"),
)


rand = dice.rand             # 0 <= r < 1, from the running toy's dice


def help_page(d, title, rows):
    """A blue menu page: bold name, plain words, one row each."""
    gfx.band(d, "blue", title, fonts.BOLD)
    gfx.body(d)
    pitch = 22 if len(rows) <= 8 else 20 if len(rows) == 9 else 14
    y = theme.BAND_H + 6 + pitch // 2
    for name, words in rows:
        gfx.text_mid(d, 12, y, name, fonts.BOLD, theme.TEXT, theme.GROUND)
        gfx.text_mid(d, 98, y, words, fonts.LABEL, theme.TEXT, theme.GROUND)
        y += pitch
    gfx.status(d, "Help again to close", fonts.LABEL)


class Toy:
    FRAME_MS = 0
    SAVABLE = True                      # has a Save stop after the strip
    SHARED = settings.SHARED            # what the strip shows after the name
    CELLS = ((128, 54), (182, 50), (232, 40), (272, 48))     # x, width of each
    KEYS_HELP = _KEYS_HELP
    SCALE_SET = None                    # if set, the only scales Scale and Random pick

    def __init__(self, d1, d2, mode, number, seed=0):
        self.seed = settings.Param("Seed", 0, 9999, seed, fmt="%04d", help="Its dice. Same seed, same tune")
        self._dice = dice.start(seed)
        dice.use(self._dice)            # make_params may roll
        self.d1 = d1
        self.d2 = d2
        self.mode = mode
        self.number = number
        self.hue = mode["hue"]
        self._kbar = "black" if mode.get("dark") else self.hue     # the title bars
        self.playing = False
        self.pos = 0
        self.steps = 0                  # clock steps since Play; never wraps
        self.step_ms = 125
        self._acc = 0
        self._facc = 0
        self._help = False
        self.own = self.make_params()
        self.params = tuple(self.own) + self.SHARED
        self.slot = None                # the saved slot this toy came from
        self.sel = 0
        self._ktext = [None] * len(self.params)      # what screen 2 shows now
        self._kdots = [None] * len(self.own)            # each knob's dot as drawn, (x, y)
        self._layout()

    # -- for the subclass ---------------------------------------------------

    def make_params(self):
        return ()

    def length(self):
        return 16

    def param_changed(self):
        pass

    def randomise(self):
        pass

    def draw_main(self):
        pass

    def on_step(self):
        pass

    def play_changed(self):
        pass

    def idle(self, ms):
        pass

    def frame(self, ms):
        pass

    def note(self, degree, voice=None, hold=0):
        """A toy calls this to sound a scale degree. voice picks between the
        toy's sounds, if it has several (sound.py says which is which). hold
        is how long to hold the note in ms; 0 leaves it to the sound.
        Other toys can listen in through settings.heard (kicks, lead notes)."""
        settings.heard(self.mode.get("role"), degree, voice, self.mode["module"], 0, hold)

    def close(self):
        """The toy is being put away."""
        sound.hush()

    # -- saving -------------------------------------------------------------

    def state(self):
        """Everything needed to bring this toy back as it is now."""
        return {"m": self.mode["module"],
                "v": [p.saved() for p in self.own],
                "s": [p.value for p in settings.SHARED],
                "c": settings.CHORDS.saved(),
                "t": [[p.name, p.shown()] for p in self.own]}

    def load(self, values):
        """Take saved knob values. Call before enter()."""
        for p, v in zip(self.own, values):
            p.load(v)
        self.param_changed()                # lets the toy fix up its limits
        for p in self.own:
            p.load(max(p.lo, min(p.hi, p.saved())))
        self.param_changed()

    def _stops(self):
        return len(self.params) + (1 if self.SAVABLE else 0)

    def _draw_save(self, said=None):
        """The Save stop lives in screen 2's status band."""
        if self.sel == len(self.params):
            gfx.status(self.d2, said or _SAVE_HINT, fonts.LABEL, True)
        else:
            gfx.status(self.d2, "Left knob picks. Right knob changes.", fonts.LABEL)

    def _save(self):
        slot = store.save(self)
        if slot is None:
            self._draw_save("No room. Delete a saved toy first.")
        else:
            self._draw_save("Saved as number %d" % (slot + 1))

    # -- events -------------------------------------------------------------

    def enter(self):
        self._draw_all()

    def handle(self, ev):
        kind, what = ev
        if kind == "press":
            if what == "ENC1":
                return "exit"
            if what == "ENC2":
                return "band"
            if what == "HELP":
                self._help = not self._help
                if self._help:
                    self._draw_help()
                    self._screens = (self.d1, self.d2)      # carry on, drawing nowhere
                    self.d1 = self.d2 = gfx.NOWHERE
                else:
                    self.d1, self.d2 = self._screens
                    self._draw_all()
            elif self._help:
                pass
            elif what == "PLAY":
                self.playing = not self.playing
                if self.playing:
                    self.restart()
                self._draw_playing()
                if not self.playing:
                    sound.hush()                    # let held notes go
                self.play_changed()
            elif what == "SCALE":
                if self.SCALE_SET:
                    k = self.SCALE_SET
                    at = k.index(settings.SCALE.value) if settings.SCALE.value in k else -1
                    settings.SCALE.value = k[(at + 1) % len(k)]
                else:
                    settings.SCALE.turn(1)
                self._changed()
            elif what == "RANDOM":
                self.shake(dice.fresh())
        elif self._help:
            pass
        elif kind == "enc1":
            new = (self.sel + what) % self._stops()
            if new != self.sel:
                old = self.sel
                self.sel = new
                self._draw_param(old)
                self._draw_param(new)
        elif kind == "enc2":
            if self.sel == len(self.params):
                self._save()
            elif self.params[self.sel].turn(what):
                self._changed()

    # -- seeds --------------------------------------------------------------

    def restart(self, seed=None):
        """Back to the top: the dice return to the seed and the toy is built
        again from nothing, then given the knob values it has now."""
        if seed is not None:
            self.seed.value = seed
        vals = [p.saved() for p in self.own]
        self._dice = dice.start(self.seed.value)
        dice.use(self._dice)
        self.own = self.make_params()
        self.params = tuple(self.own) + self.SHARED
        for p, v in zip(self.own, vals):
            p.load(v)
        self.param_changed()
        self.draw_main()                # as enter() did: some toys set up state there
        self.pos = 0
        self.steps = 0
        self._acc = 0
        self._facc = 0
        self.play_changed()

    def shake(self, seed):
        """Random: this seed, and the settings that go with it (rolled on a
        second set of dice so the tune's own run is not disturbed), then
        from the top."""
        self.seed.value = seed
        dice.use(dice.aside(seed))
        self.randomise()
        for p in self.own:
            if hasattr(p, "roll"):
                p.randomise()
        if self.SCALE_SET:
            settings.SCALE.value = self.SCALE_SET[dice.pick(len(self.SCALE_SET))]
            if not settings.band:           # in the band, its Random owns the chords
                settings.CHORDS.randomise()
        else:
            settings.SCALE.value = dice.pick(len(settings.SCALES))
        self.restart()
        self._refresh()

    def _advance(self, ms):
        """Run the clock. on_step() sees self.steps as the step being played."""
        if settings.speed <= 0:                          # the tape has stopped (Screw)
            return
        self.step_ms = 1500000 // (settings.TEMPO.value * settings.speed)   # four steps to the beat
        if self.playing:
            self._acc += ms
            if self._acc >= self.step_ms:
                n = self._acc // self.step_ms
                self._acc -= n * self.step_ms
                self.pos = (self.pos + n) % self.length()
                self.on_step()
                self.steps += n

    def tick(self, ms):
        dice.use(self._dice)
        self._advance(ms)
        if self._help:
            return
        self.idle(ms)
        if self.FRAME_MS:
            self._facc += ms
            if self._facc >= self.FRAME_MS:
                ms = self._facc
                self._facc = 0
                self.frame(ms if ms < 150 else 150)

    def _changed(self):
        self.param_changed()
        self.pos %= self.length()
        self._refresh()

    # -- screen 1 chrome ----------------------------------------------------

    def _draw_playing(self):
        fill, _, on = theme.HUES[self._kbar]
        d = self.d1
        d.fill_rect(theme.W - 70, 0, 70, theme.BAND_H, fill)
        gfx.text_right(d, theme.W - 12, theme.BAND_H // 2,
                       "Playing" if self.playing else "Paused", fonts.BOLD, on, fill)

    def _draw_all(self):
        d = self.d1
        gfx.band(d, self._kbar, self.mode["name"], fonts.BOLD)
        self._draw_playing()
        gfx.body(d)
        gfx.status(d, "Help shows what everything does", fonts.LABEL)
        self.draw_main()

        d = self.d2
        gfx.band(d, self._kbar, self.mode["name"], fonts.BOLD, STRIP_H, _NAME_X)
        gfx.body(d, STRIP_H)
        for i in range(self._stops()):
            self._draw_param(i)
        if not self.SAVABLE:
            self._draw_save()

    # -- screen 2 -----------------------------------------------------------

    def _layout(self):
        n = len(self.own)
        self._kgrid = n > 5
        if self._kgrid:              # 3 wide, small knobs, name and value on one line
            self._kat = tuple((53 + (i % 3) * 107, 52 + (i // 3) * 62) for i in range(n))
            self._kr, self._klip, self._kdot, self._kdot_at = 15, 2, 2, 9
            self._kcolw = 106
        else:                       # one row, big knobs, name over value
            w = min(100, theme.W // max(n, 1))
            x0 = (theme.W - n * w) // 2 + w // 2
            self._kat = tuple((x0 + i * w, 92) for i in range(n))
            self._kr, self._klip, self._kdot, self._kdot_at = 25, 3, 3, 15
            self._kcolw = w

    def _draw_param(self, i):
        """Draw parameter i from scratch, picked or not."""
        if i < len(self.own):
            self._draw_knob(i)
            self._draw_value(i, True)
        elif i < len(self.params):
            self._draw_cell(i)
        else:
            self._draw_save()

    def _draw_cell(self, i):
        d = self.d2
        fill, _, on = theme.HUES[self._kbar]
        x, w = self.CELLS[i - len(self.own)]
        s = self.params[i].shown()
        bg, fg = (theme.PAPER, theme.INK) if i == self.sel else (fill, on)
        d.fill_rect(x, 0, w, STRIP_H, bg)
        gfx.text_centre(d, x + w // 2, STRIP_H // 2, s, fonts.BOLD, fg, bg)
        self._ktext[i] = s

    def _dot_xy(self, i):
        a = (self.params[i].frac() * 270 - 135) * pi / 180
        cx, cy = self._kat[i]
        r = self._kdot_at
        return (cx + int(r * sin(a) + 100.5) - 100, cy - int(r * cos(a) + 100.5) + 100)

    def _draw_knob(self, i):
        d = self.d2
        cx, cy = self._kat[i]
        r = self._kr
        d.fill_rect(cx - r, cy - r, 2 * r + 1, 2 * r + 1 + self._klip, theme.GROUND)
        if i == self.sel:
            fill, deep, on = theme.HUES[self.hue]
            gfx.disc(d, cx, cy + self._klip, r, deep)
            gfx.disc(d, cx, cy, r, fill)
        else:
            gfx.disc(d, cx, cy, r, theme.SAND)       # idle: flat
            on = theme.DOT                           # but still shows its setting
        at = self._kdots[i] = self._dot_xy(i)
        gfx.disc(d, at[0], at[1], self._kdot, on)

    def _draw_value(self, i, force=False):
        d = self.d2
        p = self.params[i]
        s = p.shown()
        if s == self._ktext[i] and not force:
            return
        cx, cy = self._kat[i]
        w = self._kcolw
        if self._kgrid:
            y = cy + 27
            d.fill_rect(cx - w // 2, y - 8, w, 17, theme.GROUND)
            x = cx - (gfx.text_width(p.name, fonts.LABEL) + 5
                      + gfx.text_width(s, fonts.BOLD)) // 2
            x = gfx.text_mid(d, x, y, p.name, fonts.LABEL, theme.TEXT, theme.GROUND)
            gfx.text_mid(d, x + 5, y, s, fonts.BOLD, theme.TEXT, theme.GROUND)
        else:
            if force:
                gfx.text_centre(d, cx, cy + 48, p.name, fonts.LABEL, theme.TEXT, theme.GROUND)
            d.fill_rect(cx - w // 2, cy + 58, w, 17, theme.GROUND)
            gfx.text_centre(d, cx, cy + 66, s, fonts.BOLD, theme.TEXT, theme.GROUND)
        self._ktext[i] = s

    def _refresh(self):
        """Redraw only the values on screen 2 that no longer match."""
        if self._help:
            return
        for i, p in enumerate(self.params):
            if i < len(self.own):
                self._draw_value(i)
                at = self._dot_xy(i)
                was = self._kdots[i]
                if at != was:                       # move the dot, picked or not
                    d = self.d2
                    face, on = theme.SAND, theme.DOT
                    if i == self.sel:
                        face, _, on = theme.HUES[self.hue]
                    if was:
                        gfx.disc(d, was[0], was[1], self._kdot, face)
                    gfx.disc(d, at[0], at[1], self._kdot, on)
                    self._kdots[i] = at
            elif p.shown() != self._ktext[i]:
                self._draw_cell(i)

    # -- help ---------------------------------------------------------------

    def _draw_help(self):
        help_page(self.d1, "What the knobs and keys do", self.KEYS_HELP)
        help_page(self.d2, "What you can change",
                  tuple((p.name, p.help) for p in self.params))


class Placeholder(Toy):
    """An empty slot: the shared screen 2 and keys, nothing to play with."""
    SAVABLE = False
    KEYS_HELP = _KEYS_HELP[:8]

    def draw_main(self):
        d = self.d1
        hue = self.hue
        gfx.tile(d, 124, 62, 72, 68, 10, 4, hue)
        gfx.glyph(d, self.mode["glyph"], 138, 74, 44, theme.HUES[hue][2],
                  self.mode.get("shape"))
        gfx.text_centre(d, theme.W // 2, 160, "Nothing here yet", fonts.HERO,
                        theme.TEXT, theme.GROUND)
        gfx.text_centre(d, theme.W // 2, 184, "This toy has not been built.", fonts.LABEL,
                        theme.SOFT, theme.GROUND)
