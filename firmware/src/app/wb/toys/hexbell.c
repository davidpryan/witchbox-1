/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 93 (Witchbox III, lead): Hex Bell. A short tune on a bright bell, then a long silence, then
 * the tune again, sometimes with a quieter echo a couple of steps behind. */
#include "toy_util.h"
#undef F
#define F(n) hx_##n

static const uint8_t HX_SILENCE[3] = {16, 32, 64};
static const char *const HX_SILENCE_N[3] = {"1 bar", "2 bars", "4 bars"};
typedef struct { int8_t tune[8]; int echo_at, echo_deg; } hexbell_t;
#define NOTES 0
#define SILENCE 1
#define LEAP 2
#define ECHO 3

static void F(deal)(toy_t *t)
{
    hexbell_t *s = ST(hexbell_t);
    int i, deg = 7;
    for (i = 0; i < 8; i++) {
        s->tune[i] = (int8_t)deg;
        deg += dice_int(2 * P(LEAP) / 2 + 3) - (P(LEAP) / 2 + 1);
        deg = imax(3, imin(13, deg));
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Notes", 3, 8, 6, "How many notes the tune has");
    add_pick(t, "Silence", 3, 1, HX_SILENCE_N, "How long the silence after the tune is");
    add(t, "Leap", 0, 10, 4, "How far apart the tune's notes are");
    add(t, "Echo", 0, 10, 5, "How often a note is echoed, quieter");
    ST(hexbell_t)->echo_at = -1;
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(NOTES) = (int16_t)(3 + dice_int(6));
    P(SILENCE) = (int16_t)dice_int(3);
    P(LEAP) = (int16_t)dice_int(11);
    P(ECHO) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(deal)(t); }

static void F(on_step)(toy_t *t)
{
    hexbell_t *s = ST(hexbell_t);
    int n = P(NOTES), cycle = n * 2 + HX_SILENCE[P(SILENCE)], at = toy_now(t) % cycle, root = toy_root(t, 0);
    if (s->echo_at == at) {
        s->echo_at = -1;
        toy_note(t, s->echo_deg, 1, 0);
    }
    if (at % 2 == 0 && at / 2 < n) {
        int deg = root + s->tune[at / 2];
        if (at == 0)
            deg = tune_nearest(deg, root);
        toy_note(t, deg, 0, 0);
        if (dice_lt(P(ECHO), 11)) {
            s->echo_at = (at + 3) % cycle;
            s->echo_deg = deg;
        }
    }
}

TOY(toy_hexbell, "hexbell", "Hex Bell", "bell", ROLE_LEAD, HUE_YELLOW, 0, 2,
    "A short tune on a bright bell, then a long silence, then the tune again.",
    hexbell_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef NOTES
#undef SILENCE
#undef LEAP
#undef ECHO
