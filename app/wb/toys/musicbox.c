/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 71 (Witchbox, lead): Music Box. A tinkling little tune that pays no attention to the chords
 * and keeps its own time: its spring runs down, then it is wound again. */
#include "toy_util.h"
#undef F
#define F(n) mb_##n

static const uint8_t MB_TUNE[16] = {0, 2, 4, 2, 7, 4, 2, 0, 1, 4, 5, 4, 2, 0, 3, 1};
typedef struct { int i, gap, tt, run, rewind, off, hot, hot_ms; } musicbox_t;
#define SIZE 0
#define WIND 1
#define SOUR 2
#define SLOW 3

static void F(make)(toy_t *t)
{
    musicbox_t *s = ST(musicbox_t);
    add(t, "Tune", 4, 16, 8, "How many notes are in the tune");
    add(t, "Wind", 2, 16, 4, "Bars it runs before it slows");
    add(t, "Sour", 0, 10, 3, "How out of tune the teeth are");
    add(t, "Slow-down", 1, 10, 5, "How far it slows before the rewind");
    s->gap = 250;
    s->hot = -1;
}

static void F(randomise)(toy_t *t)
{
    P(SIZE) = (int16_t)(4 + dice_int(13));
    P(WIND) = (int16_t)(2 + dice_int(6));
    P(SOUR) = (int16_t)dice_int(11);
    P(SLOW) = (int16_t)(1 + dice_int(10));
}

static void F(pluck)(toy_t *t)
{
    musicbox_t *s = ST(musicbox_t);
    int k = MB_TUNE[s->i], base;
    toy_note(t, 7 + k, NONE, 0);
    s->hot = k;
    s->hot_ms = 140;
    s->i = (s->i + 1) % P(SIZE);
    base = 2 * t->step_ms;          /* two notes to the beat */
    if (s->run < P(WIND) * BAR * t->step_ms) {
        s->gap = base;
    } else {                        /* running down */
        s->gap = s->gap * 6 / 5;
        if (s->gap > base * (10 + P(SLOW) * 4) / 10)
            s->rewind = 900;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    musicbox_t *s = ST(musicbox_t);
    int off;
    if (s->hot >= 0) {
        s->hot_ms -= ms;
        if (s->hot_ms <= 0)
            s->hot = -1;
    }
    if (!t->playing)
        return;
    if (s->rewind) {                /* being wound: the key spins, nothing plays */
        s->rewind = imax(0, s->rewind - ms);
        if (!s->rewind) {
            s->run = s->tt = 0;
            s->gap = 2 * t->step_ms;
        }
        return;
    }
    s->run += ms;
    s->tt += ms;
    off = s->off;
    if (s->tt >= s->gap) {          /* a pin has reached the comb */
        s->tt -= s->gap;
        F(pluck)(t);
        off = -1;
    }
    s->off = imin(15, s->tt * 16 / s->gap);
    (void)off;
}

static void F(play_changed)(toy_t *t)
{
    if (t->playing)
        ST(musicbox_t)->run = 0;
}

#ifdef WB_PICTURES
/* the picture: the tune as a seal: a point a note round the ring, higher notes further out, joined
 * in a crooked polygon (bent more by Sour); the note just plucked big in the hue; the pins' creep
 * as a needle easing round to the next; the spring an orb at the centre that drains as it runs
 * down and fills again, cream, while it is wound */
static void F(draw)(toy_t *t)
{
    musicbox_t *s = ST(musicbox_t);
    static const int8_t BENT[8] = {3, -2, 5, -4, 1, -5, 4, -1};
    int n = P(SIZE), i, cx = PIC_CX, cy = PIC_CY, wind = P(WIND) * BAR * t->step_ms, fill, a, last = (s->i + n - 1) % n;
    pic_seal(cx, cy, 64, 16, 0, K_SAND);
    for (i = 0; i < n; i++) {
        int k = MB_TUNE[i], k2 = MB_TUNE[(i + 1) % n];
        int r = 28 + k * 3 + BENT[k] * P(SOUR) / 5, r2 = 28 + k2 * 3 + BENT[k2] * P(SOUR) / 5;
        int a1 = pic_ang(i, n), a2 = pic_ang(i + 1, n);
        cv_line(pic_px(cx, r, a1), pic_py(cy, r, a1), pic_px(cx, r2, a2), pic_py(cy, r2, a2), K_SAND);
    }
    a = ((last * 16 + s->off) * 32 / (n * 16)) & 31;
    cv_line(cx, cy, pic_px(cx, 26, a), pic_py(cy, 26, a), K_LINE);
    for (i = 0; i < n; i++) {
        int k = MB_TUNE[i], r = 28 + k * 3 + BENT[k] * P(SOUR) / 5, hot = i == last && s->hot >= 0;
        pic_pt(cx, cy, r, i, n, hot ? 4 : 2, hot ? pic_hue : K_TEXT);
    }
    if (s->rewind)
        fill = 1000 - s->rewind * 1000 / 900;
    else if (s->run >= wind)
        fill = 150 * 2 * t->step_ms / imax(1, s->gap);
    else
        fill = 1000 - s->run * 850 / imax(1, wind);
    pic_orb(cx, cy, 14, imax(0, imin(1000, fill)), K_SAND, s->rewind ? K_TEXT : pic_hue);
}
#endif

TOY(toy_musicbox, "musicbox", "Music Box", "comb", ROLE_LEAD, HUE_PINK, 30, 1, "A tinkling, slightly sour tune in its own time, winding down and wound again.", musicbox_t, F(make), 0, 0, F(randomise), 0, F(play_changed), F(frame), PIC(F(draw)), 0);
#undef SIZE
#undef WIND
#undef SOUR
#undef SLOW
