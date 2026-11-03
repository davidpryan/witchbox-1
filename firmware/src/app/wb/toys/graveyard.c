/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 91 (Witchbox III, chords): Graveyard Guitar. A guitar picking the chord one string at a
 * time — up, down, both ways or at random — and letting every string ring. */
#include "toy_util.h"
#undef F
#define F(n) gy_##n

static const uint8_t GY_EVERY[3] = {2, 1, 3};
static const char *const GY_SPEED_N[3] = {"Eighths", "Sixteenths", "In threes"};
static const char *const GY_ORDER_N[4] = {"Up", "Down", "Both", "Random"};
typedef struct { int at, dir; } graveyard_t;
#define SPEED 0
#define ORDER 1
#define SKIPS 2
#define SPREAD 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Speed", 3, 0, GY_SPEED_N, "How fast the strings are picked");
    add_pick(t, "Order", 4, 2, GY_ORDER_N, "The order the strings are picked in");
    add(t, "Skips", 0, 10, 2, "How often a pick is left out");
    add(t, "Spread", 0, 10, 5, "How far apart the chord's notes sit");
    ST(graveyard_t)->dir = 1;
}

static void F(randomise)(toy_t *t)
{
    P(SPEED) = (int16_t)dice_int(3);
    P(ORDER) = (int16_t)dice_int(4);
    P(SKIPS) = (int16_t)dice_int(11);
    P(SPREAD) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    graveyard_t *s = ST(graveyard_t);
    int8_t tones[16];
    int root = toy_root(t, 0) - 7, n, i;
    if (t->steps % GY_EVERY[P(SPEED)] != 0)
        return;
    n = tune_tones(root, root, root + 4 + P(SPREAD), tones);
    if (n > 6)
        n = 6;                                  /* six strings */
    switch (P(ORDER)) {
    case 0: i = s->at % n; s->at++; break;
    case 1: i = n - 1 - s->at % n; s->at++; break;
    case 2:
        i = s->at;
        s->at += s->dir;
        if (s->at >= n - 1 || s->at <= 0)
            s->dir = -s->dir;
        if (i >= n)
            i = n - 1;
        break;
    default: i = dice_int(n); break;
    }
    if (s->at >= 64)
        s->at = s->at % n;
    if (dice_lt(P(SKIPS), 14))
        return;
    toy_note(t, tones[i], 0, 1800);
}

TOY(toy_graveyard, "graveyard", "Graveyard Guitar", "guitar", ROLE_CHORDS, HUE_GREY, 0, 2,
    "A guitar picking the chord one string at a time and letting it ring.",
    graveyard_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef SPEED
#undef ORDER
#undef SKIPS
#undef SPREAD
