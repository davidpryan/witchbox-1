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

TOY(toy_musicbox, "musicbox", "Music Box", "comb", ROLE_LEAD, HUE_PINK, 30, 1,
    "A tinkling, slightly sour tune in its own time, winding down and wound again.",
    musicbox_t, F(make), 0, 0, F(randomise), 0, F(play_changed), F(frame), 0, 0);
#undef SIZE
#undef WIND
#undef SOUR
#undef SLOW
