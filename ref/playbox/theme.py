"""Toybox style guide tokens as RGB565, and the 320 x 240 panel metrics."""


def _c(h):
    return ((h >> 8) & 0xF800) | ((h >> 5) & 0x07E0) | ((h >> 3) & 0x001F)


GROUND = _c(0x3C3E45)       # every screen's ground: dark grey (not the guide's cream)
TEXT = _c(0xFFF6E0)         # text and the focus ring on the ground (10:1)
SOFT = _c(0xB9B4C4)         # secondary text and guide lines on the ground (5.3:1)
DOT = _c(0xBFC2CB)          # light grey: where an unpicked knob is set
SAND = _c(0x5C606B)         # idle things: a step lighter than the ground, so they sit back
PAPER = _c(0xFFF6E0)        # cream: status text, the picked cell in a toy's strip
INK = _c(0x2B2D42)          # text on cream and on yellow
PANEL = _c(0x2B2D42)        # status band
WHITE = _c(0xFFFFFF)

# hue name -> (fill, lip, what sits on it)
HUES = {
    "purple": (_c(0x8C5CE6), _c(0x6238AC), WHITE),
    "red":    (_c(0xEF476F), _c(0xB4284A), WHITE),
    "blue":   (_c(0x3A6CE0), _c(0x2044A4), WHITE),
    "go":     (_c(0x06BE8C), _c(0x008562), WHITE),
    "cyan":   (_c(0x00B6DE), _c(0x007D9C), WHITE),
    "orange": (_c(0xF68D29), _c(0xBD6110), WHITE),
    "pink":   (_c(0xF659B4), _c(0xBD3483), WHITE),
    "yellow": (_c(0xFFC43D), _c(0xCD9510), INK),
    # the Witchbox pair: black bars, and white tiles that carry a coloured mark
    "black":  (_c(0x000000), _c(0x000000), WHITE),
    "white":  (WHITE, _c(0xB4B7C2), _c(0x000000)),
}
SHELF_HUE = "purple"        # the shelf's section colour

# Panel metrics (the guide's 2x sizes, halved)
W, H = 320, 240
BAND_H = 26                 # title band
STATUS_H = 20               # status band
EDGE = 5                    # screen edge margin
GAP = 5                     # between tiles
RADIUS = 6                  # radius-md on the panel
LIP = 2                     # tile lip
RING = 2                    # focus line, and its gap from the block
