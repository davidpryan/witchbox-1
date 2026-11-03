"""Page three (Witchbox): witch house and dark electronics.
Drums, bass, chords and leads, a row each.

Loaded by modes.py only while it is needed, then let go. Each toy is
    (name, icon, hue, module, class, words for screen 2, job in a band or None)
and GLYPHS holds the icons these toys use (see gfx.glyph for the format).
"""
DARK = True     # drawn inverted: white tile, coloured icon, black bars

TOYS = (
    ("Two-Step", "steps2", "blue", "twostep", "TwoStep",
     "A limping garage beat. The second kick lands somewhere odd, "
     "and every hit sits a little off the grid.", "drums"),
    ("Hat Roll", "hats", "purple", "hatroll", "HatRoll",
     "Steady hi-hats that break into fast rolls at the ends of "
     "bars.", "drums"),
    ("Slow Stomp", "vee", "purple", "stomp", "Stomp",
     "A half-speed beat: a long boom of a kick, then a huge clap.", "drums"),
    ("Pocket Change", "key", "orange", "pocket", "Pocket",
     "Keys, coins and clicks dropped around the beat as ghostly "
     "percussion.", "drums"),
    ("Limp", "metro", "pink", "limp", "Limp",
     "A drunk kick and snare. The snare drags, the kick rushes, and "
     "now and then a hit goes missing.", "drums"),
    ("Undertow", "wave", "blue", "undertow", "Undertow",
     "A deep sub bass. Long notes arrive just after the kick, with "
     "a slow wobble.", "bass"),
    ("808 Slide", "slide", "red", "slide808", "Slide808",
     "A booming bass that glides from note to note and drops an "
     "octave at the end of the bar.", "bass"),
    ("Octave Pump", "oct", "go", "octpump", "OctPump",
     "A buzzing bass that hammers the root and jumps up an octave "
     "on the off-beats.", "bass"),
    ("Reese", "twist", "blue", "reese", "Reese",
     "A growling bass made of two notes rubbing together, in slow "
     "short phrases.", "bass"),
    ("Drone", "hbar", "purple", "drone", "Drone",
     "One note held under everything. It ignores the chords and "
     "slowly shifts its tone.", "bass"),
    ("Pump Pad", "oval", "pink", "pumppad", "PumpPad",
     "A held chord that ducks on every kick and swells back.", "chords"),
    ("Coven Saws", "triangle", "purple", "coven", "Coven",
     "Thick detuned chords held for whole bars. Sometimes they slip "
     "up a step.", "chords"),
    ("Stutter Gate", "comb", "cyan", "gate", "Gate",
     "The chord chopped into a rhythm of evenly spread hits.", "chords"),
    ("Wet Piano", "pkeys", "blue", "wetpiano", "WetPiano",
     "Sparse, dull piano chords, strummed a little late like a worn "
     "tape.", "chords"),
    ("Cathedral", "arch", "purple", "cathedral", "Cathedral",
     "Slow choir and organ chords that fade in and ring on over "
     "each other.", "chords"),
    ("Glass Arp", "diamond", "cyan", "glassarp", "GlassArp",
     "A bright bell running through the chord, with skipped notes, "
     "leaps and echoes.", "lead"),
    ("Bitcrush Arp", "pixels", "go", "bitarp", "BitArp",
     "A crunchy arpeggio so fast it blurs into a chord, with "
     "glitchy repeats.", "lead"),
    ("Hook Machine", "hook", "red", "hook", "Hook",
     "Writes a two-bar riff, plays it three times, then changes it "
     "on the fourth.", "lead"),
    ("Lost Melody", "dotring", "purple", "lost", "Lost",
     "A sparse tune heard from far away. Each phrase may be "
     "answered lower down.", "lead"),
    ("Screamer", "bolt", "red", "screamer", "Screamer",
     "Short bursts of fast, dirty notes that end in a squeal.", "lead"),
)

GLYPHS = {
    "steps2": (("q", 3, 2, 7, 11, 3), ("c", 6.5, 16.5, 2.6), ("q", 14, 8, 7, 11, 3), ("c", 17.5, 4.5, 2.6)),
    "hats": (("r", 3, 4, 3, 16), ("r", 11.5, 4, 3, 16), ("r", 17, 4, 3, 16)),
    "vee": (("p", (2, 3, 22, 3, 12, 22)),),
    "key": (("c", 7, 12, 5.5), ("h", 7, 12, 2), ("r", 11.5, 10.5, 11, 3),
                 ("r", 16.5, 13, 2.5, 4), ("r", 20.5, 13, 2, 3)),
    "metro": (("p", (9, 6, 15, 6, 20, 22, 4, 22)), ("p", (11, 12, 13.5, 12, 19.5, 1, 17, 1))),
    "wave": (("p", (1, 15, 6, 7, 9, 7, 4, 15)), ("p", (6, 7, 9, 7, 14, 17, 11, 17)),
                 ("p", (11, 17, 14, 17, 19, 7, 16, 7)), ("p", (16, 7, 19, 7, 23, 14, 20, 14))),
    "slide": (("p", (3, 5, 6, 2, 16, 13, 13, 16)), ("p", (21, 21, 9, 19, 19, 9))),
    "oct": (("q", 6, 2, 12, 9, 2), ("q", 6, 13, 12, 9, 2)),
    "twist": (("p", (1, 6, 4, 6, 11, 18, 8, 18)), ("p", (1, 18, 4, 18, 11, 6, 8, 6)),
                 ("p", (13, 6, 16, 6, 23, 18, 20, 18)), ("p", (13, 18, 16, 18, 23, 6, 20, 6))),
    "hbar": (("q", 2, 9, 20, 6, 2),),
    "oval": (("q", 2, 6, 20, 12, 6),),
    "triangle": (("p", (12, 2, 23, 22, 1, 22)),),
    "comb": (("r", 2, 4, 20, 4), ("r", 2, 8, 3, 12), ("r", 7.7, 8, 3, 12),
                 ("r", 13.3, 8, 3, 12), ("r", 19, 8, 3, 12)),
    "pkeys": (("q", 2, 2, 6, 20, 2), ("q", 9, 2, 6, 14, 2), ("q", 16, 2, 6, 20, 2)),
    "arch": (("p", (12, 1, 21, 10, 21, 23, 3, 23, 3, 10)), ("h", 12, 14, 3.5)),
    "diamond": (("p", (12, 1, 23, 12, 12, 23, 1, 12)),),
    "pixels": (("r", 2, 17, 5, 5), ("r", 7, 12, 5, 5), ("r", 12, 7, 5, 5), ("r", 17, 2, 5, 5)),
    "hook": (("r", 14, 2, 4, 16), ("r", 5, 18, 13, 4), ("r", 5, 12, 4, 7)),
    "dotring": (("c", 12, 12, 10.5), ("h", 12, 12, 7.5), ("c", 12, 12, 3.5)),
    "bolt": (("p", (15, 1, 5, 13, 12, 13)), ("p", (12, 11, 19, 11, 9, 23))),
}
