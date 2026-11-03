/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 92 (Witchbox III, chords): Shiver. Dark strings bowed fast and nervously: the chord's notes
 * repeated quickly, swelling up over each phrase and dying back, the voicing opening as they swell. */
#include "toy_util.h"
#undef F
#define F(n) sv_##n

static const uint8_t SV_SWELL[3] = {8, 16, 32};
static const char *const SV_SWELL_N[3] = {"Half bar", "Bar", "2 bars"};
typedef struct { int k; } shiver_t;
#define TREMBLE 0
#define SWELL 1
#define OPEN 2
#define NERVES 3

static void F(make)(toy_t *t)
{
    add(t, "Tremble", 0, 10, 6, "How fast the bow shakes");
    add_pick(t, "Swell", 3, 1, SV_SWELL_N, "How long each swell lasts");
    add(t, "Open", 0, 10, 5, "How far the tone opens over each chord");
    add(t, "Nerves", 0, 10, 3, "How often a stroke stumbles");
}

static void F(randomise)(toy_t *t)
{
    P(TREMBLE) = (int16_t)dice_int(11);
    P(SWELL) = (int16_t)dice_int(3);
    P(OPEN) = (int16_t)dice_int(11);
    P(NERVES) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    shiver_t *s = ST(shiver_t);
    int period = SV_SWELL[P(SWELL)], at = toy_now(t) % period, every = P(TREMBLE) >= 5 ? 1 : 2;
    int loud = at * 2 >= period, peak = at >= period - 2, root = toy_root(t, 0), deg, top;
    if (t->steps % every != 0)
        return;
    if (dice_lt(P(NERVES), 20))
        return;                                 /* a stumble: the bow misses */
    top = 2 + (loud ? P(OPEN) * 4 / 10 : 0);    /* how many chord notes up the voicing reaches */
    deg = root + 2 * (s->k % (top + 1));
    s->k++;
    if (at == 0)
        s->k = 0;
    toy_note(t, deg, peak ? 2 : loud ? 0 : 1, t->step_ms * every + 40);
}

TOY(toy_shiver, "shiver", "Shiver", "strings", ROLE_CHORDS, HUE_CYAN, 0, 2,
    "Dark strings bowed fast and nervously, swelling up and dying back.",
    shiver_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef TREMBLE
#undef SWELL
#undef OPEN
#undef NERVES
