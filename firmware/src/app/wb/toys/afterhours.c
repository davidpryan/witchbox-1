/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 89 (Witchbox III, chords): After Hours. Slow electric piano chords with extra notes on top
 * (sevenths, ninths), landing a step or two behind the beat. */
#include "toy_util.h"
#undef F
#define F(n) ah_##n

static const uint8_t AH_EVERY[3] = {16, 8, 4};
static const char *const AH_RATE_N[3] = {"Each bar", "Half bar", "Each beat"};
typedef struct { int last; } afterhours_t;
#define LATE 0
#define EXTRA 1
#define RATE 2
#define SPREAD 3

static void F(make)(toy_t *t)
{
    add(t, "Late", 0, 3, 1, "How many steps behind the beat it lands");
    add(t, "Extra", 0, 3, 1, "How many extra notes sit on top of the chord");
    add_pick(t, "Rate", 3, 1, AH_RATE_N, "How often the chord is played");
    add(t, "Spread", 0, 10, 4, "How far apart the chord's notes sit");
}

static void F(randomise)(toy_t *t)
{
    P(LATE) = (int16_t)dice_int(4);
    P(EXTRA) = (int16_t)dice_int(4);
    P(RATE) = (int16_t)dice_int(3);
    P(SPREAD) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    int every = AH_EVERY[P(RATE)], i = t->steps % BAR, k, n, root, hold;
    if ((i - P(LATE)) % every != 0 || i < P(LATE))
        return;
    root = toy_root(t, -P(LATE));
    n = 3 + P(EXTRA);
    hold = every * t->step_ms * 9 / 10;
    for (k = 0; k < n; k++) {
        int deg = root + 2 * k;
        if (k > 0 && dice_lt(P(SPREAD), 20))
            deg += 7;                           /* a note lifted an octave */
        if (k == 0 && P(SPREAD) > 5)
            deg -= 7;
        toy_note(t, deg, 0, hold);
    }
}

TOY(toy_afterhours, "afterhours", "After Hours", "keys", ROLE_CHORDS, HUE_BLUE, 0, 2,
    "Slow electric piano chords with extra notes on top, played a little late.",
    afterhours_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef LATE
#undef EXTRA
#undef RATE
#undef SPREAD
