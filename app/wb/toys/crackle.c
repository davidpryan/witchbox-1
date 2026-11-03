/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 69 (Witchbox, texture): Crackle. Record noise: vinyl pops, rain hiss, a low rumble, and now
 * and then a skip. (The trembling rumble bar and the rain rolled dice in the picture too.) */
#include "toy_util.h"
#undef F
#define F(n) cr_##n

typedef struct { int ang, specks[6], nspecks, drop; } crackle_t;
#define POPS 0
#define RAIN 1
#define RUMBLE 2
#define WEAR 3

static void F(bar)(toy_t *t)       /* the rumble bar's width trembles: one roll */
{
    if (P(RUMBLE) && t->playing)
        dice_int(4 + 2 * P(RUMBLE));
}

static void F(make)(toy_t *t)
{
    add(t, "Crackle", 0, 10, 5, "How many pops");
    add(t, "Rain", 0, 10, 4, "How loud the hiss is");
    add(t, "Rumble", 0, 10, 4, "How loud the low noise is");
    add(t, "Wear", 0, 10, 3, "How often the record skips");
}

static void F(randomise)(toy_t *t)
{
    P(POPS) = (int16_t)dice_int(11);
    P(RAIN) = (int16_t)dice_int(11);
    P(RUMBLE) = (int16_t)dice_int(11);
    P(WEAR) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t) { F(bar)(t); }
static void F(opened)(toy_t *t) { F(bar)(t); }
static void F(play_changed)(toy_t *t) { F(bar)(t); }

static void F(on_step)(toy_t *t)
{
    crackle_t *s = ST(crackle_t);
    int i = t->steps % BAR;
    if (i == 0) {                   /* the hiss and rumble are beds: set going each bar */
        if (P(RAIN))
            toy_note(t, 0, 1, 0);
        if (P(RUMBLE))
            toy_note(t, 0, 2, 0);
    }
    if (i % 4 == 0 && dice_lt(P(WEAR), 30)) {
        toy_note(t, 0, 3, 0);       /* a skip: the needle jumps back */
        s->ang -= 1800 / 8;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    crackle_t *s = ST(crackle_t);
    int i, j;
    for (i = 0; i < s->nspecks; i++)
        s->specks[i] -= ms;
    while (s->nspecks && s->specks[0] <= 0) {
        for (i = 1; i < s->nspecks; i++)
            s->specks[i - 1] = s->specks[i];
        s->nspecks--;
    }
    j = s->drop = (s->drop + 1) % 10;   /* one rain streak a frame */
    if (!t->playing)
        return;
    s->ang = fmod_(s->ang + ms, 1800);
    if (j < P(RAIN)) {
        dice_int(66);
        dice_k();
        dice_int(150);
    }
    if (j % 3 == 0)
        F(bar)(t);
    if (s->nspecks < 6 && dice_mul_lt(400, P(POPS) * ms)) {
        toy_note(t, 0, 0, 0);       /* a pop */
        dice_k();                   /* its angle */
        dice_int(14);               /* its radius */
        s->specks[s->nspecks++] = 120;
    }
}

TOY2(toy_crackle, "crackle", "Crackle", "record", ROLE_TEXTURE, HUE_PURPLE, 40, 1,
     "A bed of record noise: vinyl pops, rain, a low rumble, and now and then a skip.",
     crackle_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, 0, F(opened));
#undef POPS
#undef RAIN
#undef RUMBLE
#undef WEAR
