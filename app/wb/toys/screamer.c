/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 60 (Witchbox, lead): Screamer. Short bursts of fast dirty notes ending in a squeal that
 * bends up or down. The line it draws gates the next burst, so its geometry is kept exactly. */
#include "toy_util.h"
#undef F
#define F(n) sc_##n

#define SC_DX 30
#define SC_TOP 32                   /* tune.TOP + 6 */
#define SC_BOT 198                  /* tune.BOT - 22 */
#define SC_PTS 40
typedef struct {
    int16_t px[SC_PTS], py[SC_PTS];
    int npts, left, wait, n, ink, paint_c, paint_k, painting, hold;
} screamer_t;
#define BURSTS 0
#define SIZE 1
#define SQUEAL 2
#define DIRT 3
enum { INK_WHITE, INK_HUE, INK_SAND, INK_GROUND };

static int F(height)(int note)      /* tune.height(note, 4, 20, SC_BOT - 10, SC_TOP + 10) */
{
    int low = SC_BOT - 10, high = SC_TOP + 10;
    note = imax(4, imin(20, note));
    return low - floordiv((low - high) * (note - 4), 16);
}

static void F(start)(toy_t *t, int many)
{
    screamer_t *s = ST(screamer_t);
    int x;
    s->left = many;
    s->wait = 0;
    s->n = tune_nearest(8 + dice_int(7), toy_root(t, 0));
    x = 16 + dice_int(288 - many * SC_DX);
    s->px[0] = (int16_t)x;
    s->py[0] = (int16_t)F(height)(s->n);
    s->npts = 1;
}

static void F(next)(toy_t *t, int live)
{
    screamer_t *s = ST(screamer_t);
    int x = s->px[s->npts - 1], y = s->py[s->npts - 1], ny;
    s->left--;
    if (s->left) {                  /* a fast note: a few steps away */
        int d = dice_int(4) + 1, j;
        s->n = imax(5, imin(19, s->n + (dice_lt_q(32768) ? d : -d)));
        ny = F(height)(s->n);
        /* j = int((4 + dirt * 2) * (0.5 + rand() / 2)) = ((4 + dirt*2) * (32768 + k)) >> 16, exact */
        j = (int)(((4 + P(DIRT) * 2) * (32768 + (int)dice_k())) >> 16);
        j = s->left % 2 ? j : -j;
        if (s->npts + 2 <= SC_PTS) {
            s->px[s->npts] = (int16_t)(x + SC_DX / 2);
            s->py[s->npts++] = (int16_t)imax(SC_TOP, imin(SC_BOT, floordiv(y + ny, 2) + j));
            s->px[s->npts] = (int16_t)(x + SC_DX);
            s->py[s->npts++] = (int16_t)ny;
        }
    } else {                        /* the squeal: a spike up or down */
        int far = 12 + P(SQUEAL) * 8, up = 0;
        if (y - far >= SC_TOP) {    /* (rand() < 0.5 or y + far > BOT): short-circuit */
            if (dice_lt_q(32768) || y + far > SC_BOT)
                up = 1;
        }
        ny = up ? y - far : y + far;
        s->n += ny > y ? -P(SQUEAL) : P(SQUEAL);
        ny = imax(SC_TOP, imin(SC_BOT, ny));
        if (s->npts + 2 <= SC_PTS) {
            s->px[s->npts] = (int16_t)(x + 8);
            s->py[s->npts++] = (int16_t)(y + floordiv(ny - y, 4));
            s->px[s->npts] = (int16_t)(x + 14);
            s->py[s->npts++] = (int16_t)ny;
        }
        s->hold = 2 * t->step_ms;   /* the squeal rings for two steps */
    }
    if (live) {
        toy_note(t, s->n, NONE, 0);
        s->ink = INK_HUE;
    }
}

static void F(make)(toy_t *t)
{
    screamer_t *s = ST(screamer_t);
    add(t, "Bursts", 1, 10, 5, "How often a burst happens");
    add(t, "Length", 2, 8, 5, "How many notes are in a burst");
    add(t, "Squeal", 0, 10, 6, "How far the last note bends");
    add(t, "Dirt", 0, 10, 4, "From buzzy to pure noise");
    s->n = 10;
    s->ink = INK_WHITE;
    F(start)(t, 4);
    while (s->left)                 /* a line to show before anything has played */
        F(next)(t, 0);
}

static void F(randomise)(toy_t *t)
{
    P(BURSTS) = (int16_t)(1 + dice_int(10));
    P(SIZE) = (int16_t)(2 + dice_int(7));
    P(SQUEAL) = (int16_t)dice_int(11);
    P(DIRT) = (int16_t)dice_int(11);
}

static void F(repaint)(screamer_t *s, int c)
{
    s->painting = 1;
    s->paint_c = c;
    s->paint_k = 0;
}

static void F(on_step)(toy_t *t)
{
    screamer_t *s = ST(screamer_t);
    int b;
    if (s->npts || s->left || t->steps % 2)
        return;
    b = P(BURSTS);
    if (dice_lt(b * b + 4, 110))
        F(start)(t, P(SIZE));
}

static void F(rest)(toy_t *t)      /* paused: leave a whole line on show, in white */
{
    screamer_t *s = ST(screamer_t);
    if (!s->npts) {
        F(start)(t, P(SIZE));
        while (s->left)
            F(next)(t, 0);
    }
    F(repaint)(s, INK_WHITE);
}

static void F(frame)(toy_t *t, int ms)
{
    screamer_t *s = ST(screamer_t);
    if (s->painting) {              /* a few pieces a frame, in order */
        int k = s->paint_k;
        s->paint_k = k + 3;
        if (k + 3 >= s->npts / 1 - 1) {     /* len(pts) // 2 - 1 pieces: npts points */
            s->painting = 0;
            s->ink = s->paint_c;
            if (s->paint_c == INK_GROUND) {
                s->npts = 0;
                if (!t->playing)
                    F(rest)(t);
            }
        }
        return;
    }
    if (!t->playing)
        return;
    if (s->left) {
        s->wait -= ms;
        if (s->wait <= 0) {
            s->wait += t->step_ms / 2;
            F(next)(t, 1);
        }
    } else if (s->hold) {
        s->hold = imax(0, s->hold - ms);
        if (!s->hold) {             /* over: grey first, rubbed out later */
            F(repaint)(s, s->ink == INK_SAND ? INK_GROUND : INK_SAND);
            if (s->ink != INK_SAND)
                s->hold = 320;
        }
    }
}

static void F(play_changed)(toy_t *t)
{
    screamer_t *s = ST(screamer_t);
    s->left = 0;
    s->hold = t->playing && s->npts ? 320 : 0;
    if (!t->playing && !(s->painting && s->paint_c == INK_GROUND))
        F(rest)(t);                 /* (a line being rubbed out is finished first) */
}

#ifdef WB_PICTURES
/* the picture: a ruler of sixteen points round a seal, and each burst as a jagged line wound
 * round inside it, note to note, further out the higher, that shoots in or out at the squeal; in
 * the hue while it sounds, then sand, then gone; the noise rune in the middle, lit through a burst */
static int F(sx)(int cx, int r, int a)   /* a point at an angle in 256ths of a turn */
{
    int lo = (a >> 3) & 31, hi = (lo + 1) & 31, f = a & 7;
    return cx + r * (PIC_SIN[lo] * 8 + (PIC_SIN[hi] - PIC_SIN[lo]) * f) / 800;
}
static void F(draw)(toy_t *t)
{
    screamer_t *s = ST(screamer_t);
    int i, k, upto = s->painting ? s->paint_k : s->npts - 1, cx = PIC_CX, cy = PIC_CY;
    uint16_t c;
    pic_circle(cx, cy, 64, K_SAND);
    for (i = 0; i < 16; i++)
        pic_pt(cx, cy, 64, i, 16, i % 4 == 0 ? 2 : 1, K_SAND);
    pic_needle(cx, cy, 60, t->steps % BAR, BAR, K_LINE);
    pic_rune(cx - 4, cy - 6, RUNE_NOISE, s->left ? pic_hue : K_SOFT);
    if (s->ink == INK_GROUND && !s->painting)
        return;
    for (k = 0; k + 1 < s->npts; k++) {
        int ink = s->painting && k < upto ? s->paint_c : s->ink;
        int a0 = (s->px[k] - 16) * 256 / 288, a1 = (s->px[k + 1] - 16) * 256 / 288;
        int r0 = 20 + (SC_BOT - s->py[k]) * 38 / (SC_BOT - SC_TOP), r1 = 20 + (SC_BOT - s->py[k + 1]) * 38 / (SC_BOT - SC_TOP);
        if (ink == INK_GROUND)
            continue;
        c = ink == INK_HUE ? pic_hue : ink == INK_SAND ? K_SAND : K_TEXT;
        cv_line(F(sx)(cx, r0, a0), F(sx)(cy, -r0, a0 + 64), F(sx)(cx, r1, a1), F(sx)(cy, -r1, a1 + 64), c);
        cv_line(F(sx)(cx, r0, a0), F(sx)(cy, -r0, a0 + 64) + 1, F(sx)(cx, r1, a1), F(sx)(cy, -r1, a1 + 64) + 1, c);
    }
}
#endif

TOY(toy_screamer, "screamer", "Screamer", "bolt", ROLE_LEAD, HUE_RED, 30, 0, "Short bursts of fast, dirty notes that end in a squeal.", screamer_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef BURSTS
#undef SIZE
#undef SQUEAL
#undef DIRT
