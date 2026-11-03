/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 81 (Witchbox III, drums): Heartbeat. Only a kick drum, beating like a heart: a lub and a
 * quieter dub, now and then a flutter (two quick beats) or a missed one, and a skip at phrase ends. */
#include "toy_util.h"
#undef F
#define F(n) ht_##n

static const uint8_t HT_PERIOD[3] = {8, 6, 4};
static const char *const HT_PACE_N[3] = {"Resting", "Walking", "Racing"};
typedef struct { int flutter, hot; } heart_t;
#define PACE 0
#define DUB 1
#define FLUTTER 2
#define SKIPS 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Pace", 3, 1, HT_PACE_N, "How fast the heart beats");
    add(t, "Dub", 0, 10, 7, "How often the second, softer beat sounds");
    add(t, "Flutter", 0, 10, 3, "How often it flutters or misses a beat");
    add(t, "Skips", 0, 10, 4, "How often the kick skips a phrase end");
}

static void F(randomise)(toy_t *t)
{
    P(PACE) = (int16_t)dice_int(3);
    P(DUB) = (int16_t)dice_int(11);
    P(FLUTTER) = (int16_t)dice_int(11);
    P(SKIPS) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    heart_t *s = ST(heart_t);
    int period = HT_PERIOD[P(PACE)], i = t->steps % period;
    if (i == 0) {
        int end = toy_now(t) % 64 >= 64 - period;        /* the last beat of a four-bar phrase */
        if (end && dice_lt(P(SKIPS), 12))
            return;
        if (dice_lt(P(FLUTTER), 40))                      /* a missed beat */
            return;
        s->flutter = dice_lt(P(FLUTTER), 30);
        toy_kick(t);
        s->hot = 2;
    } else if (i == 1 && s->flutter) {
        s->flutter = 0;
        toy_note(t, 0, 1, 0);
    } else if (i == period / 2 + 1 && dice_lt(P(DUB), 11)) {
        toy_note(t, 0, 1, 0);
    }
}

TOY(toy_heart, "heart", "Heartbeat", "heart", ROLE_DRUMS, HUE_RED, 0, 2,
    "Only a kick drum, beating like a heart. Now and then it flutters or misses.",
    heart_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef PACE
#undef DUB
#undef FLUTTER
#undef SKIPS
