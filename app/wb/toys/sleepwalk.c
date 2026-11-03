/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 105 (Witchbox IV, bass): Sleepwalk. A random walk on the chord's notes: each step up or down
 * by a little, with momentum (it keeps going the way it went) and a pull back to the root. */
#include "toy_util.h"
#undef F
#define F(n) slw_##n

static const uint8_t SLW_EVERY[3] = {4, 2, 1};
static const char *const SLW_RATE_N[3] = {"Quarters", "Eighths", "Sixteenths"};
typedef struct { int deg, dir; } sleepwalk_t;
#define STEP 0
#define MOMENTUM 1
#define RATE 2
#define RETURN 3

static void F(make)(toy_t *t)
{
    add(t, "Step", 1, 4, 1, "How far one step of the walk can go");
    add(t, "Momentum", 0, 10, 5, "How much it keeps going the same way");
    add_pick(t, "Rate", 3, 1, SLW_RATE_N, "How often it steps");
    add(t, "Return", 0, 10, 4, "How hard the root pulls it back");
    ST(sleepwalk_t)->deg = -7;
    ST(sleepwalk_t)->dir = 1;
}

static void F(randomise)(toy_t *t)
{
    P(STEP) = (int16_t)(1 + dice_int(4));
    P(MOMENTUM) = (int16_t)dice_int(11);
    P(RATE) = (int16_t)dice_int(3);
    P(RETURN) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    sleepwalk_t *s = ST(sleepwalk_t);
    int root = toy_root(t, 0) - 7, dir, deg;
    if (t->steps % SLW_EVERY[P(RATE)] != 0)
        return;
    if (t->steps % BAR == 0 && dice_lt(P(RETURN), 11)) {
        deg = root;                             /* pulled home on the bar */
    } else {
        dir = dice_lt(P(MOMENTUM), 12) ? s->dir : (dice_lt_q(32768) ? 1 : -1);
        if (s->deg >= root + 7 && dir > 0)
            dir = -1;
        if (s->deg <= root - 5 && dir < 0)
            dir = 1;
        s->dir = dir;
        deg = tune_nearest(s->deg + dir * (1 + dice_int(P(STEP))), root);
    }
    s->deg = deg;
    toy_note(t, deg, 0, 0);
}

TOY(toy_sleepwalk, "sleepwalk", "Sleepwalk", "walk", ROLE_BASS, HUE_BLUE, 0, 3,
    "A random walk on the chord's notes, with momentum and a pull back to the root.",
    sleepwalk_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef STEP
#undef MOMENTUM
#undef RATE
#undef RETURN
