"""Page four (Witchbox): more drums, chords and leads, then voices, textures
and the three conductors.

Loaded by modes.py only while it is needed, then let go. Each toy is
    (name, icon, hue, module, class, words for screen 2, job in a band or None)
and GLYPHS holds the icons these toys use (see gfx.glyph for the format).
"""
DARK = True     # drawn inverted: white tile, coloured icon, black bars

TOYS = (
    ("Four Floor", "four", "red", "fourfloor", "FourFloor",
     "A straight dance beat: kick on every beat, hat between, clap "
     "on two and four.", "drums"),
    ("Snare Rush", "rush", "orange", "rush", "Rush",
     "Snare rolls that speed up into the start of a bar.", "drums"),
    ("Broken Clock", "clockb", "cyan", "clock", "Clock",
     "A simple beat that stutters on small slices of itself and "
     "drops tiny silences.", "drums"),
    ("Fog Toms", "toms", "purple", "toms", "Toms",
     "Slow, booming toms tuned to the chord, with long tails.", "drums"),
    ("Riser", "rampg", "orange", "riser", "Riser",
     "A sound that climbs into the end of a phrase, then an "
     "impact.", "texture"),
    ("Rave Stab", "stack3", "red", "rave", "Rave",
     "Short chord stabs in a jumpy pattern. The chord keeps one "
     "shape and moves up and down whole.", "chords"),
    ("Swell", "wedge", "pink", "swell", "Swell",
     "Chords that fade in backwards and cut dead on the downbeat.", "chords"),
    ("Tape Loop", "reels", "cyan", "tapeloop", "TapeLoop",
     "A short chord loop that drifts against the beat and slowly "
     "wears out.", "chords"),
    ("Crackle", "record", "purple", "crackle", "Crackle",
     "Vinyl pops, rain and rumble, with the odd skip.", "texture"),
    ("Night Bus", "lights", "blue", "nightbus", "NightBus",
     "The city at night, far off: a hum, passing cars, now and then "
     "a siren.", "texture"),
    ("Music Box", "windkey", "pink", "musicbox", "MusicBox",
     "A slightly sour little tune that winds down, then is wound up "
     "again.", "lead"),
    ("Siren", "scurve", "purple", "siren", "Siren",
     "Long notes that glide from one to the next and never jump.", "lead"),
    ("Shadow", "shade", "blue", "shadow", "Shadow",
     "Follows whichever lead is playing in the band, in harmony or "
     "as an echo.", "lead"),
    ("Ghost Voice", "mouth", "pink", "ghost", "Ghost",
     "Short sung phrases that slide between notes and come and go.", "voice"),
    ("Stutter Chop", "slices", "orange", "chop", "Chop",
     "One voice note fired in fast bursts, stepping through the "
     "chord.", "voice"),
    ("Screwed Voice", "face", "purple", "screwed", "Screwed",
     "A slowed-down, pitched-down voice dragging long low notes.", "voice"),
    ("Breath Stack", "ribbons", "pink", "breath", "Breath",
     "Airy voices that come in one at a time, hold together, then "
     "leave.", "voice"),
    ("Blackout", "eclipse", "purple", "blackout", "Blackout",
     "A conductor. Cuts parts of the band dead for a moment, then "
     "brings them back.", "conductor"),
    ("Screw", "reel3", "purple", "screw", "Screw",
     "A conductor. Drags the whole band slower and lower, and pulls "
     "tape stops.", "conductor"),
    ("Arc", "arcg", "go", "arc", "Arc",
     "A conductor. Leads the band through a song: intro, build, "
     "drop, breakdown.", "conductor"),
)

GLYPHS = {
    "four": (("r", 1, 8, 4.5, 8), ("r", 7, 8, 4.5, 8), ("r", 13, 8, 4.5, 8), ("r", 19, 8, 4.5, 8)),
    "rush": (("r", 2, 4, 2.5, 16), ("r", 9, 4, 2.5, 16), ("r", 14, 4, 2.5, 16),
                 ("r", 17.5, 4, 2, 16), ("r", 20.5, 4, 2, 16)),
    "clockb": (("c", 12, 12, 10.5), ("h", 12, 12, 8), ("r", 11, 6, 2, 7), ("r", 11, 11, 6, 2)),
    "toms": (("c", 4.5, 17, 3), ("c", 11, 15, 4.5), ("c", 18.5, 12, 5.5)),
    "rampg": (("p", (1, 22, 22, 22, 22, 4)),),
    "stack3": (("r", 4, 3, 16, 5), ("r", 4, 9.5, 16, 5), ("r", 4, 16, 16, 5)),
    "wedge": (("p", (1, 12, 22, 3, 22, 21)),),
    "reels": (("c", 6.5, 13, 5.5), ("h", 6.5, 13, 2), ("c", 17.5, 13, 5.5), ("h", 17.5, 13, 2),
                 ("r", 6.5, 5, 11, 2)),
    "record": (("c", 12, 12, 10.5), ("h", 12, 12, 2.2)),
    "lights": (("c", 6.5, 12, 5), ("c", 17.5, 12, 5)),
    "windkey": (("c", 7, 8, 5.5), ("h", 7, 8, 2.2), ("c", 17, 8, 5.5), ("h", 17, 8, 2.2),
                 ("r", 10.5, 9, 3, 13)),
    "scurve": (("r", 5, 3, 15, 4), ("r", 5, 3, 4, 10), ("r", 5, 10, 14, 4),
                 ("r", 15, 10, 4, 10), ("r", 4, 17, 15, 4)),
    "shade": (("c", 15, 14, 8.5), ("h", 15, 14, 6), ("c", 9, 10, 7.5)),
    "mouth": (("q", 4, 2, 16, 20, 8), ("h", 12, 12, 4.5)),
    "slices": (("r", 2, 8, 4, 8), ("r", 7.5, 3, 4, 18), ("r", 13, 6, 4, 12), ("r", 18.5, 10, 4, 6)),
    "face": (("q", 5, 1, 14, 22, 7), ("h", 12, 15.5, 3.5), ("h", 9, 7, 1.4), ("h", 15, 7, 1.4)),
    "ribbons": (("r", 2, 3, 20, 3), ("r", 2, 8.5, 20, 3), ("r", 2, 14, 20, 3), ("r", 2, 19.5, 20, 3)),
    "eclipse": (("c", 11, 12, 9.5), ("h", 17, 8, 7)),
    "reel3": (("c", 12, 12, 10.5), ("h", 12, 5.5, 2.6), ("h", 6.4, 15.2, 2.6), ("h", 17.6, 15.2, 2.6)),
    "arcg": (("p", (1, 21, 4, 21, 9, 7, 6, 7)), ("r", 7, 5, 10, 3), ("p", (15, 7, 18, 7, 23, 21, 20, 21))),
}
