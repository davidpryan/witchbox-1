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
/* the picture: a ruler of beats, and each burst as a jagged line, note to note, that curls and shoots
 * off at the squeal; in colour while it sounds, then grey, then gone */
static void F(draw)(toy_t *t)
{
    screamer_t *s = ST(screamer_t);
    int i, k, upto = s->painting ? s->paint_k : s->npts - 1;
    uint16_t c;
    for (i = 0; i < 17; i++)
        cv_rect(11 + i * 14, 134, 2, i % 4 == 0 ? 5 : 3, K_SAND);
    if (s->ink == INK_GROUND && !s->painting)
        return;
    for (k = 0; k + 1 < s->npts; k++) {
        int ink = s->painting && k < upto ? s->paint_c : s->ink;
        if (ink == INK_GROUND)
            continue;
        c = ink == INK_HUE ? pic_hue : ink == INK_SAND ? K_SAND : K_TEXT;
        cv_line(s->px[k] * 3 / 4, (s->py[k] - 26) * 3 / 4, s->px[k + 1] * 3 / 4, (s->py[k + 1] - 26) * 3 / 4, c);
    }
}
#endif

TOY(toy_screamer, "screamer", "Screamer", "bolt", ROLE_LEAD, HUE_RED, 30, 0, "Short bursts of fast, dirty notes that end in a squeal.", screamer_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef BURSTS
#undef SIZE
#undef SQUEAL
#undef DIRT
