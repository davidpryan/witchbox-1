/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 53 (Witchbox, chords): Stutter Gate. The chord chopped into a Euclidean rhythm of 1 to 32
 * steps; on every hit the chord sounds for a moment (Smear). */
#include "toy_util.h"
#undef F
#define F(n) gt_##n

typedef struct { uint8_t pat[32]; int open, ms; } gate_t;
#define HITS 0
#define COUNT 1
#define ROTATE 2
#define SMEAR 3

static void F(pattern)(toy_t *t)
{
    gate_t *s = ST(gate_t);
    int n = imax(1, imin(32, P(COUNT))), hits = imax(0, imin(n, P(HITS))), r = imax(0, imin(n - 1, P(ROTATE))), i;
    uint8_t raw[32];
    for (i = 0; i < n; i++)
        raw[i] = (i * hits) % n < hits;
    for (i = 0; i < n; i++)         /* turned round by Rotate: new[i] = raw[(i + n - r) % n] */
        s->pat[i] = raw[(i + n - r) % n];
}

static void F(changed)(toy_t *t)
{
    int n = P(COUNT);
    t->p[HITS].hi = (int16_t)n;     /* no more hits than steps */
    P(HITS) = (int16_t)imin(P(HITS), n);
    t->p[ROTATE].hi = (int16_t)(n - 1);
    P(ROTATE) = (int16_t)imin(P(ROTATE), n - 1);
    F(pattern)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Hits", 0, 16, 7, "How many steps let the chord through");
    add(t, "Steps", 1, 32, 16, "How long the loop is, 1 to 32");
    add(t, "Rotate", 0, 15, 0, "Shifts the rhythm round the loop");
    add(t, "Smear", 1, 10, 4, "How long each chop rings on");
    ST(gate_t)->open = -1;
    F(pattern)(t);
}

static int F(length)(toy_t *t) { return P(COUNT); }

static void F(randomise)(toy_t *t)
{
    P(COUNT) = (int16_t)(2 + dice_int(31));
    P(HITS) = (int16_t)(1 + dice_int(P(COUNT)));
    P(ROTATE) = (int16_t)dice_int(P(COUNT));
    P(SMEAR) = (int16_t)(1 + dice_int(10));
}

static void F(play_changed)(toy_t *t)
{
    if (!t->playing)
        ST(gate_t)->open = -1;
}

static void F(on_step)(toy_t *t)
{
    gate_t *s = ST(gate_t);
    int i = t->pos % P(COUNT);
    if (s->pat[i]) {
        int r = toy_root(t, 0), v;
        for (v = 0; v < 3; v++)     /* the chord, all at once */
            toy_note(t, r + 2 * v, v, 0);
        s->open = i;
        s->ms = t->step_ms * (1 + P(SMEAR)) / 4;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    gate_t *s = ST(gate_t);
    if (s->open >= 0) {
        s->ms -= ms;
        if (s->ms <= 0)
            s->open = -1;
    }
}

TOY(toy_gate, "gate", "Stutter Gate", "comb", ROLE_CHORDS, HUE_CYAN, 30, 0,
    "The chord chopped into a rhythm of evenly spread hits.",
    gate_t, F(make), F(length), F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef HITS
#undef COUNT
#undef ROTATE
#undef SMEAR
