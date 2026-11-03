/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 111 (Witchbox IV, lead): Fractal. A motif of a few intervals, applied to itself: the tune is
 * the motif, played starting on each note of the motif, and that again one level deeper. */
#include "toy_util.h"
#undef F
#define F(n) fr_##n

typedef struct { int8_t motif[5]; int8_t tune[125]; int n; } fractal_t;
#define SIZE 0
#define DEPTH 1
#define REACH 2
#define RATE 3

static void F(build)(toy_t *t)
{
    fractal_t *s = ST(fractal_t);
    int m = P(SIZE), d = P(DEPTH), i, j, k, n = 1;
    s->tune[0] = 0;
    for (k = 0; k < d; k++) {                   /* each level: every note becomes the motif from there */
        int8_t next[125];
        int c = 0;
        for (i = 0; i < n && c + m <= 125; i++)
            for (j = 0; j < m; j++)
                next[c++] = (int8_t)(s->tune[i] + s->motif[j]);
        for (i = 0; i < c; i++)
            s->tune[i] = next[i];
        n = c;
    }
    s->n = n;
}

static void F(deal)(toy_t *t)
{
    fractal_t *s = ST(fractal_t);
    int i, r = P(REACH);
    s->motif[0] = 0;
    for (i = 1; i < 5; i++)
        s->motif[i] = (int8_t)(dice_int(2 * r + 1) - r);
    F(build)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Motif", 2, 5, 3, "How many notes the motif has");
    add(t, "Depth", 1, 3, 2, "How many levels deep it goes");
    add(t, "Reach", 1, 4, 2, "How far the motif's steps reach");
    add(t, "Rate", 1, 4, 2, "Steps between notes");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(SIZE) = (int16_t)(2 + dice_int(4));
    P(DEPTH) = (int16_t)(1 + dice_int(3));
    P(REACH) = (int16_t)(1 + dice_int(4));
    P(RATE) = (int16_t)(1 + dice_int(4));
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(build)(t); }

static void F(on_step)(toy_t *t)
{
    fractal_t *s = ST(fractal_t);
    int i, deg;
    if (t->steps % P(RATE) != 0)
        return;
    i = (t->steps / P(RATE)) % s->n;
    deg = toy_root(t, 0) + 7 + s->tune[i];
    if (i == 0)
        deg = tune_nearest(deg, toy_root(t, 0));
    toy_note(t, deg, 0, 0);
}

TOY(toy_fractal, "fractal", "Fractal", "fractal", ROLE_LEAD, HUE_CYAN, 0, 3,
    "A motif applied to itself: the motif, played from each of its own notes, and that again one level deeper.",
    fractal_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef SIZE
#undef DEPTH
#undef REACH
#undef RATE
