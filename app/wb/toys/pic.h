/* SPDX-License-Identifier: GPL-3.0-only */
/* The screen's colours, and what a toy's picture draws with. A toy's draw(t) is called with the toy page's
 * canvas begun (240 x 144, K_BG), the toy's hue in pic_hue; it draws its picture from its state, every frame
 * (Playbox drew the same pictures a change at a time on a 320 x 194 body; here each is redrawn whole, at
 * three quarters of the size). ui.c includes this before the toys, so fm1-base's cv_* are in scope. */
#pragma once

#define K_BG RGB(20, 20, 26)
#define K_PANEL RGB(40, 40, 50)
#define K_LINE RGB(66, 66, 80)
#define K_DIM RGB(140, 136, 150)
#define K_TEXT RGB(248, 242, 226)
#define K_SAND RGB(92, 94, 106)
#define K_ACC RGB(255, 84, 164)
#define K_LIT RGB(255, 196, 61)             /* Playbox's yellow: playheads and what sounds now */
#define K_SOFT RGB(185, 180, 196)           /* guide lines */
static const uint16_t HUE_C[NHUES] = {RGB(140, 92, 230), RGB(66, 134, 244), RGB(60, 200, 220), RGB(84, 196, 110),
                                      RGB(240, 200, 60), RGB(244, 140, 50), RGB(232, 70, 70), RGB(240, 110, 180),
                                      RGB(150, 150, 158)};
static uint16_t pic_hue;                    /* the toy's colour, set before draw(t) */

#define PW 240
#define PH 144

/* centre x of column i of n across the picture (Playbox's tune.col) */
static inline int pic_col(int i, int n, int margin) { return margin + (PW - 2 * margin) * (2 * i + 1) / (2 * n); }

/* y for a note between lo and hi: higher notes sit higher (tune.height) */
static inline int pic_height(int note, int lo, int hi, int low, int high)
{
    if (hi <= lo)
        return low;
    note = note < lo ? lo : note > hi ? hi : note;
    return low - (low - high) * (note - lo) / (hi - lo);
}
#define PIC_LOW (PH - 16)
#define PIC_HIGH 16

static void pic_disc(int cx, int cy, int r, uint16_t c)
{
    int dy;
    if (r <= 0) {
        cv_rect(cx, cy, 1, 1, c);
        return;
    }
    for (dy = -r; dy <= r; dy++) {
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r * r + r)
            dx++;
        cv_rect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
    }
}

static void pic_ring(int cx, int cy, int r, int hole, uint16_t c)   /* a disc with a hole: a ring */
{
    pic_disc(cx, cy, r, c);
    pic_disc(cx, cy, hole, K_BG);
}

static void pic_marker(int x, int w, int on)    /* a small playhead under a column */
{
    cv_rect(x - w / 2, PH - 7, w, 3, on ? K_LIT : K_BG);
}

static void pic_bar(int steps, int n)           /* the bar's steps along the bottom, the current one lit */
{
    int i;
    for (i = 0; i < n; i++)
        cv_rect(2 + i * (PW - 4) / n, PH - 5, (PW - 4) / n - 2, 3, i == steps % n ? K_LIT : K_SAND);
}

static void pic_vline(int x, int y0, int y1, uint16_t c) { cv_rect(x, y0, 1, y1 - y0, c); }
static void pic_hline(int x0, int x1, int y, uint16_t c) { cv_rect(x0, y, x1 - x0, 1, c); }

static void pic_rrect(int x, int y, int w, int h, int r, uint16_t c)   /* a rounded rectangle */
{
    int j;
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (j = 0; j < h; j++) {
        int dx = 0;
        if (j < r || j >= h - r) {
            int dy = j < r ? r - j : j - (h - r) + 1, q = 0;
            while (q < r && (q + 1) * (q + 1) + dy * dy <= r * r) q++;
            dx = r - q;
        }
        cv_rect(x + dx, y + j, w - 2 * dx, 1, c);
    }
}
static const int8_t PIC_SIN[32] = {0, 20, 38, 56, 71, 83, 92, 98, 100, 98, 92, 83, 71, 56, 38, 20,
                                   0, -20, -38, -56, -71, -83, -92, -98, -100, -98, -92, -83, -71, -56, -38, -20};

/* small string helpers (no libc on the device): numbers and capitals for the screen */
static void itoa_u(uint32_t v, char *b)
{
    char t[12];
    int n = 0;
    do {
        t[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    while (n)
        *b++ = t[--n];
    *b = 0;
}
static void itoa_s(int v, char *b)
{
    if (v < 0) {
        *b++ = '-';
        v = -v;
    }
    itoa_u((uint32_t)v, b);
}
static void itoa_w(uint32_t v, char *b, int w)   /* zero padded */
{
    char t[12];
    int n = 0, i;
    itoa_u(v, t);
    while (t[n])
        n++;
    for (i = 0; i < w - n; i++)
        *b++ = '0';
    for (i = 0; i < n; i++)
        *b++ = t[i];
    *b = 0;
}
static void caps(char *d, const char *s, int n)
{
    int i;
    for (i = 0; i < n - 1 && s[i]; i++)
        d[i] = (char)(s[i] >= 'a' && s[i] <= 'z' ? s[i] - 32 : s[i]);
    d[i] = 0;
}
static void cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

/* ms since this seat last sounded (voice -1: any voice; else that voice), from the band's note log;
 * a big number when it has not. For pictures that light up when something sounds */
static uint32_t pic_ago(const toy_t *t, int voice)
{
    uint32_t now = plat_ms(), best = 100000u, i;
    for (i = 0; i < NOTE_LOG; i++) {
        const note_log_t *l = &note_log[i];
        if (l->at && l->seat == (uint8_t)t->seat && (voice < 0 || l->voice == (uint8_t)voice) && now - l->at < best)
            best = now - l->at;
    }
    return best;
}
/* the same for a degree: ms since the seat sounded it, or a big number */
static uint32_t pic_ago_deg(const toy_t *t, int degree)
{
    uint32_t now = plat_ms(), best = 100000u, i;
    for (i = 0; i < NOTE_LOG; i++) {
        const note_log_t *l = &note_log[i];
        if (l->at && l->seat == (uint8_t)t->seat && l->degree == degree && now - l->at < best)
            best = now - l->at;
    }
    return best;
}
/* a small hash, for scatter that does not touch the toy's dice */
static uint32_t pic_hash(uint32_t a, uint32_t b) { a = (a ^ 0x9E3779B9u) * 0x85EBCA6Bu; a ^= b * 0xC2B2AE35u; a ^= a >> 13; a *= 0x27D4EB2Fu; return a ^ (a >> 16); }

/* ---- the sigil vocabulary: every picture is a seal. Rings, points on a ring, chords between them,
 * runes for the things a toy does, and glyphs that fill. Angles are 32nds of a turn, 0 at the top,
 * clockwise (PIC_SIN below). ---- */
#define PIC_CX 120
#define PIC_CY 72
static inline int pic_px(int cx, int r, int a) { return cx + r * PIC_SIN[a & 31] / 100; }
static inline int pic_py(int cy, int r, int a) { return cy - r * PIC_SIN[(a + 8) & 31] / 100; }
/* angle of point i of n round a ring, in 32nds (i may exceed n) */
static inline int pic_ang(int i, int n) { return n ? (i * 32 / n) & 31 : 0; }
/* ring of 1 px, radius r (a finer ring than pic_ring's, for the seal's lines) */
static void pic_circle(int cx, int cy, int r, uint16_t c)   /* midpoint circle, 1 px */
{
    int x = r, y = 0, e = 1 - r;
    if (r <= 0) {
        cv_rect(cx, cy, 1, 1, c);
        return;
    }
    while (x >= y) {
        cv_rect(cx + x, cy + y, 1, 1, c); cv_rect(cx - x, cy + y, 1, 1, c);
        cv_rect(cx + x, cy - y, 1, 1, c); cv_rect(cx - x, cy - y, 1, 1, c);
        cv_rect(cx + y, cy + x, 1, 1, c); cv_rect(cx - y, cy + x, 1, 1, c);
        cv_rect(cx + y, cy - x, 1, 1, c); cv_rect(cx - y, cy - x, 1, 1, c);
        y++;
        if (e < 0)
            e += 2 * y + 1;
        else {
            x--;
            e += 2 * (y - x) + 1;
        }
    }
}
/* the seal: a ring at r and one at r - 6, with n points round the inner one joined by chords of
 * step `skip` (0: no chords; 1: a polygon; 2..: a star) */
static void pic_seal(int cx, int cy, int r, int n, int skip, uint16_t c)
{
    int i;
    pic_circle(cx, cy, r, c);
    pic_circle(cx, cy, r - 6, c);
    if (skip > 0 && n > 1)
        for (i = 0; i < n; i++) {
            int a = pic_ang(i, n), b = pic_ang(i + skip, n);
            cv_line(pic_px(cx, r - 6, a), pic_py(cy, r - 6, a), pic_px(cx, r - 6, b), pic_py(cy, r - 6, b), c);
        }
}
/* a point on the ring: a disc at point i of n round radius r */
static void pic_pt(int cx, int cy, int r, int i, int n, int size, uint16_t c)
{
    int a = pic_ang(i, n);
    pic_disc(pic_px(cx, r, a), pic_py(cy, r, a), size, c);
}
/* a chord between points i and j of n on radius r */
static void pic_chord(int cx, int cy, int r, int i, int j, int n, uint16_t c)
{
    int a = pic_ang(i, n), b = pic_ang(j, n);
    cv_line(pic_px(cx, r, a), pic_py(cy, r, a), pic_px(cx, r, b), pic_py(cy, r, b), c);
}
/* a needle from the centre to the ring at point i of n (a playhead) */
static void pic_needle(int cx, int cy, int r, int i, int n, uint16_t c)
{
    int a = pic_ang(i, n);
    cv_line(cx, cy, pic_px(cx, r, a), pic_py(cy, r, a), c);
}
/* a rune: one of sixteen 8 x 12 marks, drawn with its top-left at x, y. The alphabet is fixed so a
 * role always has the same mark: 0 kick, 1 snare, 2 hat, 3 perc, 4 bass, 5 chord, 6 lead, 7 voice,
 * 8 noise, 9 rest, 10 up, 11 down, 12 hold, 13 cross, 14 wave, 15 eye */
static void pic_rune(int x, int y, int k, uint16_t c)
{
    switch (k & 15) {
    case 0: cv_line(x, y, x, y + 12, c); cv_line(x, y, x + 8, y + 6, c); cv_line(x + 8, y + 6, x, y + 12, c); break;
    case 1: cv_line(x, y + 12, x + 4, y, c); cv_line(x + 4, y, x + 8, y + 12, c); cv_line(x + 2, y + 7, x + 6, y + 7, c); break;
    case 2: cv_line(x, y + 2, x + 8, y + 2, c); cv_line(x + 4, y + 2, x + 4, y + 12, c); break;
    case 3: cv_line(x + 4, y, x + 4, y + 12, c); cv_line(x, y + 4, x + 8, y + 4, c); cv_line(x, y + 8, x + 8, y + 8, c); break;
    case 4: cv_line(x, y + 12, x, y, c); cv_line(x, y, x + 8, y + 4, c); cv_line(x + 8, y + 4, x, y + 8, c); cv_line(x, y + 8, x + 8, y + 12, c); break;
    case 5: cv_line(x, y + 12, x, y + 3, c); cv_line(x, y + 3, x + 4, y, c); cv_line(x + 4, y, x + 8, y + 3, c); cv_line(x + 8, y + 3, x + 8, y + 12, c); cv_line(x, y + 7, x + 8, y + 7, c); break;
    case 6: cv_line(x, y + 12, x + 4, y, c); cv_line(x + 4, y, x + 8, y + 12, c); break;
    case 7: cv_line(x, y, x + 4, y + 12, c); cv_line(x + 4, y + 12, x + 8, y, c); break;
    case 8: cv_line(x, y, x + 8, y + 12, c); cv_line(x + 8, y, x, y + 12, c); cv_line(x + 4, y, x + 4, y + 12, c); break;
    case 9: cv_line(x + 2, y + 6, x + 6, y + 6, c); break;
    case 10: cv_line(x + 4, y, x + 4, y + 12, c); cv_line(x, y + 4, x + 4, y, c); cv_line(x + 4, y, x + 8, y + 4, c); break;
    case 11: cv_line(x + 4, y, x + 4, y + 12, c); cv_line(x, y + 8, x + 4, y + 12, c); cv_line(x + 4, y + 12, x + 8, y + 8, c); break;
    case 12: cv_line(x, y + 2, x + 8, y + 2, c); cv_line(x, y + 10, x + 8, y + 10, c); cv_line(x + 4, y + 2, x + 4, y + 10, c); break;
    case 13: cv_line(x, y, x + 8, y + 12, c); cv_line(x + 8, y, x, y + 12, c); break;
    case 14: cv_line(x, y + 8, x + 2, y + 4, c); cv_line(x + 2, y + 4, x + 4, y + 8, c); cv_line(x + 4, y + 8, x + 6, y + 4, c); cv_line(x + 6, y + 4, x + 8, y + 8, c); break;
    default: pic_circle(x + 4, y + 6, 4, c); cv_rect(x + 3, y + 5, 3, 3, c); break;
    }
}
#define RUNE_KICK 0
#define RUNE_SNARE 1
#define RUNE_HAT 2
#define RUNE_PERC 3
#define RUNE_BASS 4
#define RUNE_CHORD 5
#define RUNE_LEAD 6
#define RUNE_VOICE 7
#define RUNE_NOISE 8
#define RUNE_REST 9
#define RUNE_UP 10
#define RUNE_DOWN 11
#define RUNE_HOLD 12
#define RUNE_CROSS 13
#define RUNE_WAVE 14
#define RUNE_EYE 15
/* a rune at point i of n on radius r, centred there */
static void pic_rune_at(int cx, int cy, int r, int i, int n, int k, uint16_t c)
{
    int a = pic_ang(i, n);
    pic_rune(pic_px(cx, r, a) - 4, pic_py(cy, r, a) - 6, k, c);
}
/* a glyph that fills: a triangle (apex up) of base w at (x, base y), outlined in c and filled from
 * the base up by `fill` of 1000 in fc */
static void pic_spire(int x, int y, int w, int h, int fill, uint16_t c, uint16_t fc)
{
    int j, fh = h * fill / 1000;
    for (j = 0; j < fh; j++) {
        int ww = w * (h - j) / h;
        cv_rect(x - ww / 2, y - j, ww, 1, fc);
    }
    cv_line(x - w / 2, y, x + w / 2, y, c);
    cv_line(x - w / 2, y, x, y - h, c);
    cv_line(x + w / 2, y, x, y - h, c);
}
/* a glyph that fills from the centre: a disc of radius r * fill / 1000 inside a ring of r */
static void pic_orb(int cx, int cy, int r, int fill, uint16_t c, uint16_t fc)
{
    pic_circle(cx, cy, r, c);
    if (fill > 0)
        pic_disc(cx, cy, r * fill / 1000, fc);
}
/* the row of the seal's points as a strip: n small marks along y, lit where set (a bar line under a seal) */
static void pic_strip(int y, int n, uint32_t bits, int at, uint16_t c)
{
    int i, w = (PW - 40) / n;
    for (i = 0; i < n; i++)
        cv_rect(20 + i * w + w / 2 - 1, y - (i == at ? 2 : 0), 3, i == at ? 5 : 3, (bits >> i) & 1u ? c : K_SAND);
}
