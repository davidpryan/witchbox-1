/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 42 (Witchbox, drums): Hat Roll. Steady hats, two to the beat, that break into a fast roll on
 * the last beat of some bars; Ramp makes the roll fall or rise. */
#include "toy_util.h"
#undef F
#define F(n) hr_##n

static const uint8_t HR_COUNT[3] = {4, 6, 8};  /* hats in a one-beat roll */
static const char *const RATE_N[3] = {"16ths", "Triplets", "32nds"};

typedef struct {
    uint8_t rolling;                /* does this bar end in a roll? */
    uint8_t opens[8];               /* which of the eight hats ring open */
    int lit, k, n, wait;
} hatroll_t;

#define CHANCE 0
#define RATE 1
#define RAMP 2
#define OPEN 3

static void F(make)(toy_t *t)
{
    hatroll_t *s = ST(hatroll_t);
    add(t, "Roll chance", 0, 10, 5, "How often a bar ends in a roll");
    add_pick(t, "Roll rate", 3, 1, RATE_N, "How fast the roll is");
    add(t, "Ramp", -5, 5, 3, "Roll falls (below 0) or rises (above 0)");
    add(t, "Open hat", 0, 10, 3, "How often an off-beat hat rings open");
    s->rolling = 1;
    s->lit = -1;
}

static void F(randomise)(toy_t *t)
{
    P(CHANCE) = (int16_t)dice_int(11);
    P(RATE) = (int16_t)dice_int(3);
    P(RAMP) = (int16_t)(dice_int(11) - 5);
    P(OPEN) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t)
{
    hatroll_t *s = ST(hatroll_t);
    s->k = s->n = 0;                /* a roll in flight would no longer match the picture */
}

static void F(play_changed)(toy_t *t)
{
    hatroll_t *s = ST(hatroll_t);
    s->k = s->n = 0;
    s->lit = -1;
}

static void F(on_step)(toy_t *t)
{
    hatroll_t *s = ST(hatroll_t);
    int i = t->steps % BAR;
    if (i == 0) {                   /* plan the bar: a roll or not, and the open hats */
        int slot;
        s->rolling = (uint8_t)dice_lt(P(CHANCE), 10);
        for (slot = 1; slot <= 7; slot += 2)
            s->opens[slot] = (uint8_t)dice_lt(P(OPEN), 12);
        s->lit = -1;
    }
    if (i == 12 && s->rolling) {
        s->lit = -1;
        s->k = 0;                   /* the roll itself is played in frame() */
        s->n = HR_COUNT[P(RATE)];
        s->wait = 0;
    } else if (i % 2 == 0 && !(s->rolling && i > 12)) {
        int slot = i / 2;
        s->lit = slot;
        toy_note(t, 0, s->opens[slot] ? 3 : 2, 0);     /* 3 open hat, 2 closed */
    }
}

static void F(frame)(toy_t *t, int ms)
{
    hatroll_t *s = ST(hatroll_t);
    if (!t->playing || s->k >= s->n)
        return;
    s->wait -= ms;
    while (s->wait <= 0 && s->k < s->n) {
        toy_note(t, fdiv(P(RAMP) * s->k, s->n - 1), 2, 0);     /* pitch follows the ramp */
        s->k++;
        s->wait += 4 * t->step_ms / s->n;
    }
}

TOY(toy_hatroll, "hatroll", "Hat Roll", "hats", ROLE_DRUMS, HUE_PURPLE, 20, 0,
    "Steady hi-hats that break into fast rolls at the ends of bars.",
    hatroll_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef CHANCE
#undef RATE
#undef RAMP
#undef OPEN
