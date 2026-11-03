/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 106 (Witchbox IV, bass): Pulsar. The root pulsed on a Euclidean rhythm: so many hits spread
 * as evenly as they can over so many steps, turned round, with octave jumps on the odd hit. */
#include "toy_util.h"
#undef F
#define F(n) pu_##n

typedef struct { uint8_t on[BAR], rank[BAR]; } pulsar_t;
#define HITS 0
#define STEPS 1
#define TURN 2
#define JUMPS 3

static void F(lay)(toy_t *t)
{
    pulsar_t *s = ST(pulsar_t);
    int n = P(STEPS), k = P(HITS), i;
    if (k > n)
        k = n;
    for (i = 0; i < BAR; i++)
        s->on[i] = 0;
    for (i = 0; i < n; i++)                     /* Bjorklund by the bucket: the same spread */
        s->on[(i + P(TURN)) % n] = ((i * k) / n) != (((i + 1) * k) / n) || (k == n);
}

static void F(deal)(toy_t *t)
{
    pulsar_t *s = ST(pulsar_t);
    int i;
    for (i = 0; i < BAR; i++)
        s->rank[i] = (uint8_t)dice_int(10);
    F(lay)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Hits", 1, 16, 5, "How many hits in the loop");
    add(t, "Steps", 4, 16, 16, "How many steps the loop is");
    add(t, "Turn", 0, 15, 0, "Turns the pattern round");
    add(t, "Jumps", 0, 10, 3, "How often a hit jumps up an octave");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(STEPS) = (int16_t)(8 + dice_int(9));
    P(HITS) = (int16_t)(1 + dice_int(P(STEPS)));
    P(TURN) = (int16_t)dice_int(P(STEPS));
    P(JUMPS) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(lay)(t); }
static int F(length)(toy_t *t) { return P(STEPS); }

static void F(on_step)(toy_t *t)
{
    pulsar_t *s = ST(pulsar_t);
    int i = t->pos % P(STEPS), deg;
    if (!s->on[i])
        return;
    deg = toy_root(t, 0) - 7;
    if (s->rank[i] < P(JUMPS))
        deg += 7;
    toy_note(t, deg, 0, 0);
}

TOY(toy_pulsar, "pulsar", "Pulsar", "pulse", ROLE_BASS, HUE_CYAN, 0, 3,
    "The root pulsed on a Euclidean rhythm: so many hits spread evenly over so many steps.",
    pulsar_t, F(make), F(length), F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef HITS
#undef STEPS
#undef TURN
#undef JUMPS
