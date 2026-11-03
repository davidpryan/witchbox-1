/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 117 (Witchbox IV, texture): Whisper. Breathy voice fragments, quiet and far between, that
 * come in bursts: a few close together, then nothing for a while (a Poisson burst process). */
#include "toy_util.h"
#undef F
#define F(n) wh_##n

typedef struct { int burst, wait; } whisper_t;
#define DENSITY 0
#define BURSTS 1
#define LENGTH 2
#define DOUBLES 3

static void F(make)(toy_t *t)
{
    add(t, "Density", 0, 10, 3, "How often a burst of whispers starts");
    add(t, "Bursts", 1, 6, 3, "How many whispers in a burst");
    add(t, "Length", 1, 4, 2, "How long each whisper is, in steps");
    add(t, "Doubles", 0, 10, 3, "How often a second voice whispers too");
}

static void F(randomise)(toy_t *t)
{
    P(DENSITY) = (int16_t)dice_int(11);
    P(BURSTS) = (int16_t)(1 + dice_int(6));
    P(LENGTH) = (int16_t)(1 + dice_int(4));
    P(DOUBLES) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t) { ST(whisper_t)->burst = ST(whisper_t)->wait = 0; }

static void F(on_step)(toy_t *t)
{
    whisper_t *s = ST(whisper_t);
    int root = toy_root(t, 0), deg, hold = P(LENGTH) * t->step_ms - 30;
    if (s->wait > 0) {
        s->wait--;
        return;
    }
    if (s->burst <= 0) {
        if (!dice_lt(P(DENSITY), 50))
            return;
        s->burst = 1 + dice_int(P(BURSTS));
    }
    s->burst--;
    s->wait = P(LENGTH) + dice_int(3);
    deg = root + dice_int(7) - 3;
    toy_note(t, deg, 0, hold);
    if (dice_lt(P(DOUBLES), 11))
        toy_note(t, tune_nearest(deg + 2 + dice_int(3), root), 1, hold);
}

TOY(toy_whisper, "whisper", "Whisper", "whisper", ROLE_TEXTURE, HUE_PINK, 0, 3,
    "Breathy voice fragments, quiet and far between, that come in bursts and then fall silent.",
    whisper_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef DENSITY
#undef BURSTS
#undef LENGTH
#undef DOUBLES
