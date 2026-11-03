/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 107 (Witchbox IV, bass): Hysteria. The logistic map, x -> r x (1 - x), picks every note: at
 * low Chaos it settles on one note, then flips between two, then four, then goes wild. */
#include "toy_util.h"
#undef F
#define F(n) hy_##n

typedef struct { int x; } hysteria_t;           /* 0..4095 */
#define CHAOS 0
#define RANGE 1
#define RATE 2
#define SKIPS 3

static void F(make)(toy_t *t)
{
    add(t, "Chaos", 0, 10, 6, "From one steady note, through doubling, to chaos");
    add(t, "Range", 1, 8, 4, "How many chord notes it can reach");
    add(t, "Rate", 1, 4, 2, "Steps between notes");
    add(t, "Skips", 0, 10, 3, "How often a low x is a rest instead");
    ST(hysteria_t)->x = 1000;
}

static void F(randomise)(toy_t *t)
{
    P(CHAOS) = (int16_t)dice_int(11);
    P(RANGE) = (int16_t)(1 + dice_int(8));
    P(RATE) = (int16_t)(1 + dice_int(4));
    P(SKIPS) = (int16_t)dice_int(11);
    ST(hysteria_t)->x = 500 + dice_int(3000);
}

static void F(play_changed)(toy_t *t)
{
    if (t->playing)
        ST(hysteria_t)->x = 1000;
}

static void F(on_step)(toy_t *t)
{
    hysteria_t *s = ST(hysteria_t);
    int r, root, n, k;
    int8_t tones[12];
    if (t->steps % P(RATE) != 0)
        return;
    r = 180 + P(CHAOS) * 76 / 10;               /* r in 64ths: 2.8 .. 4.0 (256 = 4.0) */
    s->x = (int)(((int32_t)s->x * (4096 - s->x) >> 6) * r >> 12);   /* x in 0..4095 */
    if (s->x < 1)
        s->x = 1;
    if (s->x > 4095)
        s->x = 4095;
    if (s->x < 820 && dice_lt(P(SKIPS), 11))
        return;
    root = toy_root(t, 0) - 7;
    n = tune_tones(root, root, root + 13, tones);
    if (n > P(RANGE))
        n = P(RANGE);
    k = s->x * n / 4096;
    toy_note(t, tones[k], 0, 0);
}

TOY(toy_hysteria, "hysteria", "Hysteria", "chaos", ROLE_BASS, HUE_RED, 0, 3,
    "The logistic map picks every note: one steady note, then two, then four, then chaos.",
    hysteria_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef CHAOS
#undef RANGE
#undef RATE
#undef SKIPS
