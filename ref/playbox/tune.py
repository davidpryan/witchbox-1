"""Small helpers shared by the page-two toys: where things go on screen 1,
and which notes belong to a chord."""
import theme, gfx

TOP = theme.BAND_H                      # screen 1's body runs TOP .. BOT
BOT = theme.H - theme.STATUS_H
LIT = theme.HUES["yellow"][0]


def col(i, n, margin=12):
    """Centre x of column i of n spread across the screen."""
    return margin + (theme.W - 2 * margin) * (2 * i + 1) // (2 * n)


def height(note, lo, hi, low=BOT - 20, high=TOP + 20):
    """y for a note between lo and hi: higher notes sit higher."""
    if hi <= lo:
        return low
    note = max(lo, min(hi, note))
    return low - (low - high) * (note - lo) // (hi - lo)


def tones(root, lo, hi):
    """The chord's notes between lo and hi: its root, third and fifth."""
    return [n for n in range(lo, hi + 1) if (n - root) % 7 in (0, 2, 4)]


def nearest(note, root):
    """The chord note closest to this one."""
    for d in (0, -1, 1, -2, 2):
        if (note + d - root) % 7 in (0, 2, 4):
            return note + d
    return note


def marker(d, x, w, on):
    """A small playhead under a column."""
    d.fill_rect(x - w // 2, BOT - 7, w, 3, LIT if on else theme.GROUND)


def clear(d, y0=TOP, y1=BOT):
    d.fill_rect(0, y0, theme.W, y1 - y0, theme.GROUND)
