"""Turns what a toy plays into what the sound engine is asked for.

A toy only ever says note(step of the scale, voice). This module decides
which instrument or drum that is, which actual note, and how loud, and
passes it to the engine (pbaudio, in the firmware). It touches no hardware:
`out` is whatever has note() and set(), so on a computer the simulator
plugs in a stand-in and the tests can see what would have sounded.

    sound.out = pbaudio          main.py does this when the engine is there
    sound.play(...)              settings.heard calls this for every note
    sound.hush()                 let every held note go (pause, leaving a toy)

Which sound a toy gets:
  * Witchbox toys each have their own sounds, listed in WITCH below.
  * The drum toys on page two have a kit and a drum per voice, in DRUMMERS.
  * Every other toy plays the sound picked in its strip (settings.SOUNDS);
    in the band, the sound picked for its seat.
"""
import settings
from sound_ids import inst, drum, KITS, DRUM_BASE

out = None                  # the engine, or a stand-in; None = silence

KEY = 60                    # scale step 0 is middle C

# Semitones of each scale's seven steps. The five- and six-note scales are
# spread over seven steps (a note or two repeated), so that "seven steps up"
# is an octave and "two and four steps up" make a chord in every scale,
# which is what the toys' chord logic counts on.
_SCALES = (
    (0, 2, 4, 5, 7, 9, 11),     # Major
    (0, 2, 3, 5, 7, 8, 10),     # Minor
    (0, 2, 4, 4, 7, 9, 9),      # Penta
    (0, 3, 5, 6, 7, 10, 10),    # Blues
    (0, 2, 3, 5, 7, 9, 10),     # Dorian
    (0, 2, 4, 5, 7, 9, 10),     # Mixo
    (0, 1, 3, 5, 7, 8, 10),     # Phryg
    (0, 2, 3, 5, 7, 8, 11),     # Harmon
    (0, 3, 5, 5, 7, 10, 10),    # Pent m
)


def _fit(note):
    """Fold a note into the range the instruments sound good in (C1 to C7)."""
    while note > 96:
        note -= 12
    while note < 24:
        note += 12
    return note


def semis(degree):
    """Semitones above the key note for a scale step (which may be negative)."""
    return 12 * (degree // 7) + _SCALES[settings.SCALE.value][degree % 7]


# What each name in the strip's Sound list plays (settings.SOUNDS, same order).
_STRIP = ("Glass Bell", "Kalimba", "Warm Pad", "Saw Bass", "Soul Organ", None,     # None = drums
          "Soft Keys", "Electric Piano", "Dull Piano", "Saw Lead", "Square Lead", "Chip Lead",
          "Sub", "Acid", "Choir Aah", "Harp", "Guitar", "Marimba", "Dark Strings", "Whistle",
          "Rave Stab", "Music Box")
# The "Drum" sound on a tune: each scale step is a different drum.
_DRUM_STEPS = ("Kick", "Snare", "Closed Hat", "Low Tom", "Clap", "Mid Tom", "Open Hat")

# Page-two drum toys: kit, then the drum for each voice number the toy sends.
DRUMMERS = {
    "griddrift": ("808", ("Kick", "Snare", "Closed Hat")),
    "bounce": ("909", ("Kick", "Snare", "Mid Tom", "Closed Hat")),
    "subtract": ("Acoustic", ("Kick", "Rim", "Closed Hat")),
    "clapping": ("Tin", ("Clap", "Rim", "Cowbell")),
    "caller": ("Acoustic", ("Kick", "Snare", "Closed Hat")),
}

# Witchbox toys. Either one instrument for every voice:
#     ("Sub", octaves)
# or a kit and one entry per voice number. An entry is a drum name, or
#     ("i", instrument)        an instrument, played at the note asked for
#     ("f", n) / ("c", n)      a found sound / a click recording (engine.cpp, kFound)
#     a list of entries        all of them at once
WITCH = {
    "twostep": ("Garage", ("Kick", "Rim", "Closed Hat", ("c", 2))),
    "hatroll": ("Witch", (None, None, "Closed Hat", "Open Hat")),
    "stomp": ("Witch", ("Long Kick", "Long Clap")),
    "pocket": ("Lo-fi", (None, None, None, ("f", 0), ("f", 2), ("f", 3), ("f", 4))),
    "limp": ("Lo-fi", ("Kick", "Snare")),
    "fourfloor": ("909", ("Hard Kick", "Clap", "Closed Hat", "Open Hat")),
    "rush": ("909", (None, "Snare", None, "Crash")),
    "clock": ("Lo-fi", ("Kick", ("c", 3), ("c", 1))),
    "toms": ("Witch", (None, None, None, "Low Tom", "Mid Tom", "High Tom")),
    "riser": ("Witch", (("i", "Riser"), ("i", "Glide Lead"), ("i", "Impact"), ("i", "Woosh"))),
    "crackle": ("Lo-fi", (("c", 2), [("i", "Rain"), ("i", "Crackle")], ("i", "Rumble"), ("i", "Skip"))),
    "nightbus": ("Lo-fi", (("i", "City Hum"), ("i", "Glide Lead"), ("i", "Woosh"), "Shaker", ("i", "Door"))),
    "blackout": ("Witch", (("i", "Riser"),)),
    "undertow": ("Sub", -1), "slide808": ("808 Slide", -1), "octpump": ("Saw Bass", 0),
    "reese": ("Reese", 0), "drone": ("Drone", 1),
    "pumppad": ("Warm Pad", 0), "coven": ("Coven Saws", 0), "gate": ("Saw Pad", 0),
    "wetpiano": ("Dull Piano", 0), "cathedral": ("Church Organ", 0),
    "glassarp": ("Glass Bell", 0), "bitarp": ("Chip Lead", 0), "hook": ("Saw Lead", 0),
    "lost": ("Whistle", 0), "screamer": ("Screamer", 0),
    "rave": ("Rave Stab", 0), "swell": ("Warm Pad", 0), "tapeloop": ("Soft Keys", 0),
    "musicbox": ("Music Box", -1), "siren": ("Glide Lead", 0), "shadow": ("Soft Keys", 0),
    "ghost": ("Ghost Voice", 0), "chop": ("Vox Hit", 0), "screwed": ("Screwed Voice", 0),
    "breath": ("Choir Ooh", 0),
    "screw": None, "arc": None,         # conductors: no sound of their own
}

# The Sound a toy starts with when it is opened from the toy box (a name
# from settings.SOUNDS). Toys not listed start on Bell. Saved toys keep
# whatever was picked when they were saved.
STARTS = {
    "euclid": "Mallet", "gears": "Pluck", "pachinko": "Mallet", "plants": "Harp", "mountain": "Pluck",
    "chorus": "Choir", "marbles": "Mallet", "coast": "Flute", "life": "Keys", "ribbon": "E Piano",
    "dominoes": "Pluck", "wanderer": "Flute", "compass": "Pad", "swarm": "Mallet", "tap": "Pluck",
    "tides": "Pad",
    "looplock": "Saw", "question": "Keys", "snowball": "E Piano", "habit": "Square", "arp": "Pluck",
    "acid": "Acid", "walker": "Bass", "subpulse": "Sub", "riff": "Bass", "slap": "Bass",
    "strum": "Guitar", "stabs": "Stab", "bloom": "Pad", "comp": "E Piano", "borrowed": "Keys",
}


def start_sound(module):
    """Set the strip's Sound to this toy's starting sound."""
    settings.SOUND.value = settings.SOUNDS.index(STARTS.get(module, "Bell"))


seat_sound = [0] * settings.SEATS       # the band's Sound pick for each seat
seat_level = [80] * settings.SEATS      # and its volume, 0..100
_sent = {}                              # what the engine was last told, to send only changes


def _tell(what, seat, value, arg=0):
    key = (what, seat)
    if _sent.get(key) != (value, arg):
        _sent[key] = (value, arg)
        out.set(what, seat, value, arg)


def _settings(seat):
    """Pass on anything that changed: volumes, Screw's pitch drop, echo time."""
    _tell(out.MASTER, 0, settings.VOLUME.value * 127 // 100)
    _tell(out.LEVEL, seat, (seat_level[seat] if settings.band else 80) * 127 // 100)
    _tell(out.DROP, 0, settings.drop * 8)           # 10 = five semitones down
    _tell(out.ECHO, 0, 64, min(400, 45000 // settings.TEMPO.value))    # three steps long


def _one(seat, entry, kit, degree, vel, hold):
    if entry is None:
        return
    if isinstance(entry, list):
        for e in entry:
            _one(seat, e, kit, degree, vel, hold)
    elif isinstance(entry, str):                    # a drum; the scale step tunes it
        tune = max(-24, min(24, semis(degree))) if degree else 0
        out.note(seat, drum(entry), tune, vel, kit)
    elif entry[0] == "i":
        out.note(seat, inst(entry[1]), _fit(KEY + semis(degree)), vel, hold)
    elif entry[0] == "f":                           # found sounds come in pairs; take either
        out.note(seat, drum("Found"), entry[1] + (degree & 1 if entry[1] in (0, 4) else 0), vel, kit)
    else:
        out.note(seat, drum("Click"), entry[1], vel, kit)


def play(role, degree, voice, module, seat=0, hold=0):
    """A toy sounded a note."""
    if out is None:
        return
    _settings(seat)
    v = voice or 0
    vel = 100
    w = WITCH.get(module, 0)
    if w is None:
        return
    if w:
        if isinstance(w[1], int):                   # one instrument
            if module == "glassarp" and v == 1:
                vel = 55                            # the echo is quieter
            out.note(seat, inst(w[0]), _fit(KEY + 12 * w[1] + semis(degree)), vel, hold)
        elif v < len(w[1]):
            _one(seat, w[1][v], KITS.index(w[0]), degree, vel, hold)
        return
    d = DRUMMERS.get(module)
    if d:
        out.note(seat, drum(d[1][v % len(d[1])]), 0, vel, KITS.index(d[0]))
        return
    if module == "rain":                            # Rain fall: each drum has its own Sound, sent as the voice
        pick = _STRIP[v % len(_STRIP)]
    else:
        pick = _STRIP[(seat_sound[seat] if settings.band else settings.SOUND.value) % len(_STRIP)]
    if pick is None:
        out.note(seat, drum(_DRUM_STEPS[degree % 7]), 0, vel, 0)
    else:
        out.note(seat, inst(pick), _fit(KEY + semis(degree)), vel, hold)


def hush():
    if out is not None:
        out.set(out.ALL_OFF, 0, 0)


def load():
    """(percent of the audio core in use, worst lately, notes dropped, voices) or None."""
    return out.load() if out is not None and hasattr(out, "load") else None
