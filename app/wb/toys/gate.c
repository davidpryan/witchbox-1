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

#ifdef WB_PICTURES
/* the picture: the loop as a wheel, a point a step; the steps that hit are cream and joined into
 * a figure by chords, the rests sand dots; the hit the chord shows through is big in the hue, the
 * needle on the step, and an orb in the middle drains as the chop rings on */
static void F(draw)(toy_t *t)
{
    gate_t *s = ST(gate_t);
    int n = imax(1, imin(32, P(COUNT))), i, now = t->pos % n, last = -1, first = -1, cx = PIC_CX, cy = PIC_CY;
    int full = t->step_ms * (1 + P(SMEAR)) / 4;
    pic_circle(cx, cy, 64, K_SAND);
    pic_circle(cx, cy, 58, K_SAND);
    for (i = 0; i < n; i++)
        if (s->pat[i]) {
            if (last >= 0)
                pic_chord(cx, cy, 50, last, i, n, K_LINE);
            else
                first = i;
            last = i;
        }
    if (first >= 0 && last != first)
        pic_chord(cx, cy, 50, last, first, n, K_LINE);
    pic_needle(cx, cy, 50, now, n, t->playing ? K_SOFT : K_LINE);
    for (i = 0; i < n; i++)
        if (!s->pat[i])
            pic_pt(cx, cy, 50, i, n, 0, K_SAND);
        else if (i == s->open)
            pic_pt(cx, cy, 50, i, n, 5, pic_hue);
        else
            pic_pt(cx, cy, 50, i, n, n > 16 ? 2 : 3, K_TEXT);
    pic_orb(cx, cy, 16, s->open >= 0 && full ? 1000 * s->ms / full : 0, K_SAND, pic_hue);
}
#endif

TOY(toy_gate, "gate", "Stutter Gate", "comb", ROLE_CHORDS, HUE_CYAN, 30, 0, "The chord chopped into a rhythm of evenly spread hits.", gate_t, F(make), F(length), F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef HITS
#undef COUNT
#undef ROTATE
#undef SMEAR
