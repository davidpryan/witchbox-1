"""Drawing in the Toybox look. Hardware-free.

Everything goes through two display methods, fill_rect and blit, so this
runs the same on the panel and in tools/sim.py. All shapes are flat fills.

On the device the display is a pbdraw.Panel (C, built into the firmware),
which also has disc, line, rrect and rects. Where a display has those, the
whole shape is handed over in one call; otherwise it is drawn here from
rectangles. Both ways put down exactly the same pixels
(firmware/tools/test_shapes.py checks).
"""
from math import sqrt
import theme

class _Nowhere:
    """A display that keeps nothing. A toy given this still runs, but
    draws nowhere: used for the band's seats and while help is showing."""

    def fill_rect(self, x, y, w, h, c):
        pass

    def blit(self, x, y, w, h, buf):
        pass


NOWHERE = _Nowhere()

# ---------------------------------------------------------------- text

_gcache = {}
_gbytes = 0
_GCACHE_MAX = 24000         # bytes of converted glyphs kept


def _mix(bg, fg, n):
    """bg -> fg in thirds, in RGB565."""
    if n == 0:
        return bg
    if n == 3:
        return fg
    m = 3 - n
    r = ((bg >> 11) * m + (fg >> 11) * n + 1) // 3
    g = (((bg >> 5) & 63) * m + ((fg >> 5) & 63) * n + 1) // 3
    b = ((bg & 31) * m + (fg & 31) * n + 1) // 3
    return (r << 11) | (g << 5) | b


def _glyph(font, i, w, h, fg, bg):
    global _gbytes
    key = (font[0], i, fg, bg)
    buf = _gcache.get(key)
    if buf is None:
        data = font[3]
        off = font[2][i + 5] | (font[2][i + 6] << 8)
        pal = (bg, _mix(bg, fg, 1), _mix(bg, fg, 2), fg)
        buf = bytearray(w * h * 2)
        for p in range(w * h):
            c = pal[(data[off + (p >> 2)] >> (6 - ((p & 3) << 1))) & 3]
            buf[p * 2] = c >> 8
            buf[p * 2 + 1] = c & 255
        if _gbytes > _GCACHE_MAX:
            _gcache.clear()
            _gbytes = 0
        _gcache[key] = buf
        _gbytes += len(buf)
    return buf


def forget():
    """Drop every ready-made glyph, to hand the memory back."""
    global _gbytes
    _gcache.clear()
    _gbytes = 0


def text(d, x, base, s, font, fg, bg):
    """Draw s with its baseline at y=base, on a flat bg. Returns the end x."""
    idx = font[2]
    for ch in s:
        o = ord(ch)
        if o < 32 or o > 126:
            o = 63
        i = (o - 32) * 8
        w = idx[i + 3]
        if w:
            h = idx[i + 4]
            d.blit(x + idx[i + 1] - 16, base - idx[i + 2] + 16, w, h,
                   _glyph(font, i, w, h, fg, bg))
        x += idx[i]
    return x


def text_width(s, font):
    idx = font[2]
    n = 0
    for ch in s:
        o = ord(ch)
        if o < 32 or o > 126:
            o = 63
        n += idx[(o - 32) * 8]
    return n


def text_mid(d, x, cy, s, font, fg, bg):
    """Left-aligned, capitals centred on cy."""
    return text(d, x, cy + (font[1] + 1) // 2, s, font, fg, bg)


def text_right(d, xr, cy, s, font, fg, bg):
    text_mid(d, xr - text_width(s, font), cy, s, font, fg, bg)


def text_centre(d, cx, cy, s, font, fg, bg):
    text_mid(d, cx - text_width(s, font) // 2, cy, s, font, fg, bg)


def wrap(s, font, maxw):
    lines = []
    line = ""
    for word in s.split():
        t = line + " " + word if line else word
        if line and text_width(t, font) > maxw:
            lines.append(line)
            line = word
        else:
            line = t
    if line:
        lines.append(line)
    return lines


# ---------------------------------------------------------------- blocks

_insets = {}


def _corner(r):
    """Left inset of each of the r rows of a rounded corner."""
    t = _insets.get(r)
    if t is None:
        t = tuple(r - int(sqrt(r * r - (r - i - 0.5) ** 2) + 0.5) for i in range(r))
        _insets[r] = t
    return t


def rrect(d, x, y, w, h, r, c):
    """Solid rounded rectangle."""
    f = getattr(d, "rrect", None)       # the panel can draw whole shapes itself (pbdraw)
    if f:
        return f(x, y, w, h, r, c)
    ins = _corner(r)
    for i in range(r):
        n = ins[i]
        d.fill_rect(x + n, y + i, w - 2 * n, 1, c)
        d.fill_rect(x + n, y + h - 1 - i, w - 2 * n, 1, c)
    d.fill_rect(x, y + r, w, h - 2 * r, c)


def tile(d, x, y, w, h, r, lip, hue):
    """A lipped block: face w x h at (x, y), lip in the deep shade below it."""
    fill, deep, _ = theme.HUES[hue]
    ins = _corner(r)
    d.fill_rect(x, y + h - r, w, lip, deep)
    for i in range(r):
        n = ins[r - 1 - i]
        d.fill_rect(x + n, y + h - r + lip + i, w - 2 * n, 1, deep)
    rrect(d, x, y, w, h, r, fill)


_discs = {}


def disc(d, cx, cy, r, c):
    """Solid circle, 2r + 1 pixels across, centred on the pixel (cx, cy)."""
    f = getattr(d, "disc", None)
    if f:
        return f(cx, cy, r, c)
    rows = _discs.get(r)
    if rows is None:
        rows = []
        k = (r + 0.5) * (r + 0.5)
        for dy in range(-r, r + 1):
            half = int(sqrt(k - dy * dy))
            if rows and rows[-1][2] == half:
                rows[-1][1] += 1
            else:
                rows.append([dy, 1, half])
        rows = tuple(tuple(t) for t in rows)
        if len(_discs) > 16:
            _discs.clear()
        _discs[r] = rows
    for dy, h, half in rows:
        d.fill_rect(cx - half, cy + dy, 2 * half + 1, h, c)


def line(d, x0, y0, x1, y1, c, t=2):
    """A straight line t pixels thick, as runs of rectangles."""
    f = getattr(d, "line", None)
    if f:
        return f(x0, y0, x1, y1, c, t)
    dx = x1 - x0
    dy = y1 - y0
    if abs(dx) >= abs(dy):
        if dx < 0:
            x0, y0, x1, y1, dx, dy = x1, y1, x0, y0, -dx, -dy
        if dx == 0:
            d.fill_rect(x0, y0, t, t, c)
            return
        start = x0
        prev = y0
        for x in range(x0 + 1, x1 + 1):
            y = y0 + ((x - x0) * dy * 2 + dx) // (2 * dx)
            if y != prev:
                d.fill_rect(start, prev, x - start, t, c)
                start = x
                prev = y
        d.fill_rect(start, prev, x1 + 1 - start, t, c)
    else:
        if dy < 0:
            x0, y0, x1, y1, dx, dy = x1, y1, x0, y0, -dx, -dy
        start = y0
        prev = x0
        for y in range(y0 + 1, y1 + 1):
            x = x0 + ((y - y0) * dx * 2 + dy) // (2 * dy)
            if x != prev:
                d.fill_rect(prev, start, t, y - start, c)
                start = y
                prev = x
        d.fill_rect(prev, start, t, y1 + 1 - start, c)


_dots = {}


def dot_sprite(r, c):
    """A disc of radius r on the ground, as a (2r + 1) square buffer for
    blit: one write to draw a moving dot."""
    buf = _dots.get((r, c))
    if buf is None:
        n = 2 * r + 1
        g = theme.GROUND
        buf = bytearray(bytes((g >> 8, g & 255)) * (n * n))
        k = (r + 0.5) * (r + 0.5)
        for p in range(n * n):
            dx, dy = p % n - r, p // n - r
            if dx * dx + dy * dy <= k:
                buf[p * 2] = c >> 8
                buf[p * 2 + 1] = c & 255
        if len(_dots) > 12:
            _dots.clear()
        _dots[(r, c)] = buf
    return buf


def ring(d, x, y, w, h, c):
    """Focus ring around a block x, y, w, h (lip included): square corners,
    RING thick, RING clear of the block. Draw it in GROUND to remove it."""
    t = theme.RING
    o = t * 2
    d.fill_rect(x - o, y - o, w + 2 * o, t, c)
    d.fill_rect(x - o, y + h + t, w + 2 * o, t, c)
    d.fill_rect(x - o, y - t, t, h + 2 * t, c)
    d.fill_rect(x + w + t, y - t, t, h + 2 * t, c)


def band(d, hue, title, font, h=theme.BAND_H, x=12):
    """Full-bleed title band across the top."""
    fill, _, on = theme.HUES[hue]
    d.fill_rect(0, 0, theme.W, h, fill)
    text_mid(d, x, h // 2, title, font, on, fill)


def body(d, top=theme.BAND_H, c=theme.GROUND):
    """Clear the ground between the two bands."""
    d.fill_rect(0, top, theme.W, theme.H - top - theme.STATUS_H, c)


def ellipse(d, cx, cy, rx, ry, c):
    """Solid ellipse, 2rx + 1 by 2ry + 1 pixels."""
    k = ry + 0.5
    for dy in range(-ry, ry + 1):
        half = int(rx * sqrt(1 - (dy / k) * (dy / k)) + 0.5)
        d.fill_rect(cx - half, cy + dy, 2 * half + 1, 1, c)


def status(d, s, font, message=False):
    """Closing band across the bottom; yellow when it carries a message."""
    bg = theme.HUES["yellow"][0] if message else theme.PANEL
    fg = theme.INK if message else theme.PAPER
    y = theme.H - theme.STATUS_H
    d.fill_rect(0, y, theme.W, theme.STATUS_H, bg)
    text_centre(d, theme.W // 2, y + theme.STATUS_H // 2, s, font, fg, bg)


# ---------------------------------------------------------------- glyphs
#
# Solid geometric marks on a 24-unit square, as in the style guide:
#   ("c", cx, cy, r)        disc
#   ("h", cx, cy, r)        hole cut from everything before it
#   ("r", x, y, w, h)       rectangle
#   ("q", x, y, w, h, r)    rounded rectangle
#   ("p", (x, y, x, y...))  convex polygon

GLYPHS = {
    "disc":     (("c", 12, 12, 10),),
    "square":   (("q", 2, 2, 20, 20, 4),),
    "triangle": (("p", (12, 2, 23, 22, 1, 22)),),
    "diamond":  (("p", (12, 1, 23, 12, 12, 23, 1, 12)),),
    "plus":     (("r", 9, 2, 6, 20), ("r", 2, 9, 20, 6)),
    "ring":     (("c", 12, 12, 10), ("h", 12, 12, 4.5)),
    "bars":     (("r", 3, 12, 4.5, 10), ("r", 9.8, 3, 4.5, 19), ("r", 16.5, 8, 4.5, 14)),
    "dots":     (("c", 6.5, 6.5, 4.5), ("c", 17.5, 6.5, 4.5),
                 ("c", 6.5, 17.5, 4.5), ("c", 17.5, 17.5, 4.5)),
    "star":     (("p", (12, 1, 21.5, 17.5, 2.5, 17.5)), ("p", (12, 23, 2.5, 6.5, 21.5, 6.5))),
    "stairs":   (("r", 2, 16, 7, 6), ("r", 8.5, 9.5, 7, 12.5), ("r", 15, 3, 7, 19)),
}
GLYPH_NAMES = ("disc", "square", "triangle", "diamond", "plus",
               "ring", "bars", "dots", "star", "stairs")

_rects = {}


def _span(p, y):
    k = p[0]
    if k == "c" or k == "h":
        dy = y - p[2]
        if abs(dy) >= p[3]:
            return None
        dx = sqrt(p[3] * p[3] - dy * dy)
        return p[1] - dx, p[1] + dx
    if k == "r":
        if y < p[2] or y >= p[2] + p[4]:
            return None
        return p[1], p[1] + p[3]
    if k == "q":
        if y < p[2] or y >= p[2] + p[4]:
            return None
        r = p[5]
        dy = max(p[2] + r - y, y - (p[2] + p[4] - r), 0)
        n = r - sqrt(r * r - dy * dy)
        return p[1] + n, p[1] + p[3] - n
    pts = p[1]
    lo = hi = None
    n = len(pts)
    for i in range(0, n, 2):
        x0, y0 = pts[i], pts[i + 1]
        x1, y1 = pts[(i + 2) % n], pts[(i + 3) % n]
        if (y0 <= y < y1) or (y1 <= y < y0):
            x = x0 + (y - y0) * (x1 - x0) / (y1 - y0)
            if lo is None or x < lo:
                lo = x
            if hi is None or x > hi:
                hi = x
    return None if lo is None else (lo, hi)


def _build(name, size, shape=None):
    """Scan the glyph into (x, y, w, h) pixel rects, merging equal rows."""
    k = size / 24
    out = []
    prev = ()
    start = 0
    for j in range(size + 1):
        row = []
        if j < size:
            y = (j + 0.5) / k + 0.001      # never exactly on an edge
            for p in shape or GLYPHS[name]:
                s = _span(p, y)
                if s is None:
                    continue
                a = int(s[0] * k + 0.5)
                b = int(s[1] * k + 0.5)
                if p[0] == "h":
                    cut = []
                    for x0, x1 in row:
                        if a > x0:
                            cut.append((x0, min(x1, a)))
                        if b < x1:
                            cut.append((max(x0, b), x1))
                    row = [t for t in cut if t[1] > t[0]]
                elif b > a:
                    row.append((a, b))
            row.sort()
            merged = []
            for a, b in row:
                if merged and a <= merged[-1][1]:
                    if b > merged[-1][1]:
                        merged[-1] = (merged[-1][0], b)
                else:
                    merged.append((a, b))
            row = merged
        row = tuple(row)
        if row != prev:
            for a, b in prev:
                out.append((a, start, b - a, j - start))
            prev = row
            start = j
    return tuple(out)


def glyph(d, name, x, y, size, c, shape=None):
    """Draw a glyph in a size x size box at (x, y), one colour. A toy's icon
    comes with its shape (from its page file); the plain ones are in GLYPHS."""
    key = (name, size)
    rects = _rects.get(key)
    if rects is None:
        rects = _build(name, size, shape)
        if len(_rects) > 24:
            _rects.clear()
        _rects[key] = rects
    f = getattr(d, "rects", None)
    if f:
        return f(x, y, rects, c)
    for rx, ry, rw, rh in rects:
        d.fill_rect(x + rx, y + ry, rw, rh, c)
