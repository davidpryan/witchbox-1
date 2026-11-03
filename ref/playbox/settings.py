"""What every toy shares: scale, sound, tempo, volume.

These live here, not in a toy, so they carry over from one toy to the next.
sound.py turns them into what the sound engine is asked to play.
"""


class Param:
    """One thing a knob can change: a number, or a pick from a list."""

    def __init__(self, name, lo, hi, value, step=1, choices=None, fmt="%d", help=""):
        self.name = name
        self.lo = lo
        self.hi = hi
        self.value = value
        self.step = step
        self.choices = choices      # names for lo..hi; a list of names wraps round
        self.fmt = fmt
        self.help = help

    def turn(self, detents):
        v = self.value + detents * self.step
        if self.choices:
            v = self.lo + (v - self.lo) % (self.hi - self.lo + 1)
        else:
            v = max(self.lo, min(self.hi, v))
        changed = v != self.value
        self.value = v
        return changed

    def load(self, v):
        """Take a saved value."""
        self.value = v

    def saved(self):
        """The value to save."""
        return self.value

    def shown(self):
        if self.choices:
            return self.choices[self.value - self.lo]
        return self.fmt % self.value

    def frac(self):
        """0..1 position of the value, for drawing a knob."""
        if self.hi == self.lo:
            return 0
        return (self.value - self.lo) / (self.hi - self.lo)


SCALES = ("Major", "Minor", "Penta", "Blues", "Dorian", "Mixo",
          "Phryg", "Harmon", "Pent m")     # Phrygian, harmonic minor, minor pentatonic
DARK_SCALES = (1, 6, 7, 4, 8)           # the set the Witchbox toys cycle through
# The Sound list in a toy's strip. sound.py says what each one plays. Saved
# toys keep the place in this list, so only ever add at the end.
SOUNDS = ("Bell", "Pluck", "Pad", "Bass", "Organ", "Drum", "Keys", "E Piano", "Piano", "Saw",
          "Square", "Chip", "Sub", "Acid", "Choir", "Harp", "Guitar", "Mallet", "Strings",
          "Flute", "Stab", "Box")

SCALE = Param("Scale", 0, len(SCALES) - 1, 0, choices=SCALES,
              help="The set of notes the toy picks from")
SOUND = Param("Sound", 0, len(SOUNDS) - 1, 0, choices=SOUNDS,
              help="The instrument that plays")
TEMPO = Param("Tempo", 40, 240, 120, step=2,
              help="Speed, in beats per minute")
VOLUME = Param("Volume", 0, 100, 80, step=5, fmt="%d%%",
               help="How loud this toy is")

SHARED = (SCALE, SOUND, TEMPO, VOLUME)

# ---- chord progressions -----------------------------------------------------
# Roots as scale steps (0 = I ... 6 = vii), one chord to the bar. A chord is
# its root and the steps 2 and 4 above it. Choice 0 is Random: four chords
# rolled from the scale, rolled again by the Random button.
PROGRESSIONS = (
    ("Pop", (0, 4, 5, 3)),                      # I V vi IV
    ("Fifties", (0, 5, 3, 4)),                  # I vi IV V
    ("Sad", (5, 3, 0, 4)),                      # vi IV I V
    ("Rock", (0, 3, 4, 3)),                     # I IV V IV
    ("Jazz", (1, 4, 0, 5)),                     # ii V I vi
    ("Canon", (0, 4, 5, 2, 3, 0, 3, 4)),        # I V vi iii IV I IV V
    ("Blues", (0, 0, 0, 0, 3, 3, 0, 0, 4, 3, 0, 4)),    # twelve-bar
    ("Royal", (3, 4, 2, 5)),                    # IV V iii vi, the "royal road"
    ("Epic", (5, 4, 3, 4)),                     # vi V IV V
    ("Rise", (0, 3, 5, 4)),                     # I IV vi V
    # darker ones, at home in a minor scale
    ("Anthem", (0, 5, 2, 6)),                   # i VI III VII
    ("Sway", (0, 3)),                           # i iv
    ("Witch", (0, 1)),                          # i bII (in Phrygian)
    ("Dusk", (0, 6, 5, 6)),                     # i VII VI VII
    ("Lift", (5, 6, 0, 0)),                     # VI VII i i
)
ROMAN = ("I", "ii", "iii", "IV", "V", "vi", "vii")
BAR = 16                    # clock steps in a bar

band = False                # True while the band is open: its Chords rule
band_steps = 0              # the band's clock, so every seat changes chord together

RATE = Param("Rate", 0, 2, 0, choices=("1 bar", "2 bars", "4 bars"),
             help="How long the band stays on each chord")
_RATES = (1, 2, 4)


def chord_bar(steps):
    """Which chord of the progression clock step `steps` falls in. The band
    has its own clock and its Rate; a toy alone changes chord every bar."""
    if band:
        return band_steps // (BAR * _RATES[RATE.value])
    return steps // BAR


# ---- what toys tell each other ----------------------------------------------
# Small shared state, so parts can react to one another. The band owns it;
# a toy playing alone uses the same names with only itself listening.
SEATS = 4
kicks = 0                   # counts kick drums: pads duck when it moves on
drummer = False             # True if a drum toy is seated (else pads pump by themselves)
lead = None                 # the last note a lead toy played
leads = 0                   # counts lead notes
roles = [None] * SEATS      # each band seat's job: "drums", "bass", "chords", "lead", ...
cuts = [False] * SEATS      # seats silenced for now (Blackout)
levels = [100] * SEATS      # each seat's intensity, 0..100 (Arc)
speed = 100                 # clock speed in percent (Screw); 0 = stopped
drop = 0                    # how far everything is pitched down, 0..10 (Screw); for the sound engine


def heard(role, degree, voice, module=None, seat=0, hold=0):
    """A toy sounded a note: make the sound, and pass on what the other
    toys listen for."""
    global kicks, lead, leads
    sound.play(role, degree, voice, module, seat, hold)
    if role == "drums" and voice == 0:
        kicks += 1
    elif role == "lead":
        lead = degree
        leads += 1


def calm():
    """Undo everything a conductor has done."""
    global speed, drop
    speed = 100
    drop = 0
    for i in range(SEATS):
        cuts[i] = False
        levels[i] = 100


class ChordsParam(Param):
    """A toy's Chords knob. On its own it is an ordinary knob. While the
    band is open it stands for the band's shared Chords instead: it shows
    that value, turning it changes that value, and the toy's own choice is
    kept aside untouched for when it plays alone again."""

    def __init__(self, shared=False):
        self.name = "Chords"
        self.lo = 0
        self.hi = len(PROGRESSIONS)
        self.step = 1
        self.choices = ("Random",) + tuple(p[0] for p in PROGRESSIONS)
        self.fmt = "%d"
        self.help = "The run of chords it follows"
        self.shared = shared
        self._own = 1
        self.rolled = (0, 4, 5, 3)
        self.roll()

    def _src(self):
        return CHORDS if band and not self.shared else self

    @property
    def value(self):
        return self._src()._own

    @value.setter
    def value(self, v):
        self._src()._own = v

    def load(self, v):
        self._own = max(self.lo, min(self.hi, v))

    def saved(self):
        return self._own

    def roll(self):
        """A new Random progression: starts at home, then three more chords."""
        import dice
        self.rolled = (0,) + tuple(dice.pick(7) for _ in range(3))

    def randomise(self):
        """What the Random button does to this knob. The band's choice is
        left alone when a single seat is shaken up."""
        import dice
        if band and not self.shared:
            return
        self._own = dice.pick(self.hi + 1)
        self.roll()

    def progression(self):
        s = self._src()
        return s.rolled if s._own == 0 else PROGRESSIONS[s._own - 1][1]

    def root(self, steps, ahead=0):
        """The chord root for clock step `steps` (the band's clock if open)."""
        p = self.progression()
        return p[(chord_bar(steps) + ahead) % len(p)]


CHORDS = ChordsParam(shared=True)       # the band's

import sound                            # at the end: sound.py reads the settings above
