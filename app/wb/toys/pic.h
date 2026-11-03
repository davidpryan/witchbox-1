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
