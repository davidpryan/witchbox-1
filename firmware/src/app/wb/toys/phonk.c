/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 85 (Witchbox III, drums): Phonk Bell. A slow, heavy beat — kick, clap, hats — with a tune
 * played on a cowbell over it, from a few of the chord's notes. */
#include "toy_util.h"
#undef F
#define F(n) phk_##n

typedef struct { uint8_t on[BAR], rank[BAR]; int8_t deg[BAR]; } phonk_t;
#define NOTES 0
#define BELL 1
#define HATS 2
#define BOUNCE 3

static void F(lay)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int i;
    for (i = 0; i < BAR; i++)
        s->on[i] = s->rank[i] < P(BELL);
}

static void F(deal)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int8_t tones[8];
    int i, n = tune_tones(0, 0, 11, tones);     /* the chord's notes, as offsets from its root */
    for (i = 0; i < BAR; i++) {
        s->rank[i] = (uint8_t)dice_int(10);
        s->deg[i] = tones[dice_int(imin(n, P(NOTES)))];
    }
    F(lay)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Notes", 1, 5, 3, "How many different notes the bell plays");
    add(t, "Bell", 0, 10, 5, "How busy the bell is");
    add(t, "Hats", 0, 10, 6, "How busy the hats are");
    add(t, "Bounce", 0, 10, 4, "How often an extra kick bounces in");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(NOTES) = (int16_t)(1 + dice_int(5));
    P(BELL) = (int16_t)(2 + dice_int(9));
    P(HATS) = (int16_t)dice_int(11);
    P(BOUNCE) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(lay)(t); }

static void F(on_step)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int i = t->steps % BAR;
    if (i == 0 || i == 10)
        toy_kick(t);
    else if ((i == 6 || i == 14) && dice_lt(P(BOUNCE), 20))
        toy_kick(t);
    if (i == 8)
        toy_note(t, 0, 1, 0);
    if (i % 2 == 0 ? P(HATS) > 0 : dice_lt(P(HATS), 14))
        toy_note(t, 0, 3, 0);
    if (s->on[i])
        toy_note(t, toy_root(t, 0) + s->deg[i], 2, 0);
}

TOY(toy_phonk, "phonk", "Phonk Bell", "bell", ROLE_DRUMS, HUE_PURPLE, 0, 2,
    "A slow, heavy beat with a tune played on a cowbell over it.",
    phonk_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef NOTES
#undef BELL
#undef HATS
#undef BOUNCE
