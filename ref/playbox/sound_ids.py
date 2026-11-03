"""The sounds the audio engine knows, by number. This list is the one place
they are named: tools in firmware/tools turn it into the C header, so the
Python side and the engine can never disagree. Only ever add at the END of
a list, or saved toys will point at the wrong sound.
"""
INSTRUMENTS = (
    "Sub", "808 Boom", "808 Slide", "Acid", "Reese", "Saw Bass", "FM Bass", "Pluck Bass",
    "Soft Keys", "Electric Piano", "Dull Piano", "Toy Piano", "Clav", "Music Box",
    "Soul Organ", "Church Organ", "Choir Aah", "Choir Ooh",
    "Warm Pad", "Dark Strings", "Glass Pad", "Saw Pad", "Coven Saws", "Drone",
    "Saw Lead", "Square Lead", "Chip Lead", "Glide Lead", "Screamer", "Whistle",
    "Glass Bell", "Kalimba", "Marimba", "Harp", "Guitar", "Trap Bell",
    "Rave Stab", "Minor Stab", "Ghost Voice", "Screwed Voice",
    # textures and one-off recordings
    "Crackle", "Rain", "Rumble", "City Hum", "Riser", "Impact", "Woosh", "Door", "Crow",
    "Skip", "Vox Hit",
)
DRUMS = (
    "Kick", "Long Kick", "Hard Kick", "Snare", "Rim", "Clap", "Long Clap",
    "Closed Hat", "Open Hat", "Pedal Hat", "Crash", "Ride",
    "Low Tom", "Mid Tom", "High Tom", "Shaker", "Cowbell", "Conga", "Click", "Found",
)
KITS = ("808", "909", "Acoustic", "Lo-fi", "Witch", "Garage", "Chip", "Tin")
DRUM_BASE = 64              # a drum's sound number is DRUM_BASE + its place in DRUMS

# recordings built into the firmware, in this order (firmware/tools/make_samples.py)
SAMPLES = ("crackle", "skip", "hum", "woosh", "impact", "thud", "door", "crow", "choir",
           "voice", "vox_hit", "piano", "bell", "glass", "coins", "ting", "wood", "paper",
           "ping", "snap", "tick", "pop", "kick_808", "kick_boom", "tom_lo", "tom_mid",
           "tom_hi", "snare", "clap_long", "hat_closed", "hat_open", "crash")


def inst(name):
    return INSTRUMENTS.index(name)


def drum(name):
    return DRUM_BASE + DRUMS.index(name)
