/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 99 (Witchbox III, conductor): Fog. Makes the whole band duller and brighter, like fog
 * rolling in and lifting, over a few bars at a time. S.tone carries it to the sound. */
#include "toy_util.h"
#undef F
#define F(n) fg_##n

static const uint8_t FG_BARS[4] = {2, 4, 8, 16};
static const char *const FG_ROLL_N[4] = {"2 bars", "4 bars", "8 bars", "16 bars"};
static const char *const FG_SHAPE_N[2] = {"Slow", "Sudden"};
typedef struct { int ph; } fog_t;
#define DEPTH 0
#define ROLL 1
#define LIFT 2
#define SHAPE 3

static void F(make)(toy_t *t)
{
    add(t, "Depth", 0, 10, 7, "How dull the band gets when the fog is in");
    add_pick(t, "Roll", 4, 1, FG_ROLL_N, "How long the fog takes to roll in and out");
    add(t, "Lift", 0, 10, 3, "How bright it gets when the fog lifts");
    add_pick(t, "Shape", 2, 0, FG_SHAPE_N, "Rolls in slowly, or drops suddenly");
}

static void F(randomise)(toy_t *t)
{
    P(DEPTH) = (int16_t)(2 + dice_int(9));
    P(ROLL) = (int16_t)dice_int(4);
    P(LIFT) = (int16_t)dice_int(11);
    P(SHAPE) = (int16_t)dice_int(2);
}

static void F(close)(toy_t *t)
{
    (void)t;
    S.tone = 50;
}

static void F(play_changed)(toy_t *t)
{
    ST(fog_t)->ph = 0;
    if (!t->playing)
        S.tone = 50;
}

static void F(frame)(toy_t *t, int ms)
{
    fog_t *s = ST(fog_t);
    int period = FG_BARS[P(ROLL)] * BAR * t->step_ms, pos, tri, dull, bright;
    if (!t->playing || period <= 0)
        return;
    s->ph = (s->ph + ms) % period;
    pos = s->ph * 1000 / period;                /* 0..999 through one roll */
    tri = pos < 500 ? pos * 2 : 2000 - pos * 2; /* 0 (fog in) .. 1000 (lifted) */
    if (P(SHAPE) == 1)
        tri = tri < 200 ? 0 : tri > 800 ? 1000 : (tri - 200) * 1000 / 600;
    dull = 50 - P(DEPTH) * 5;
    bright = 50 + P(LIFT) * 5;
    S.tone = dull + (bright - dull) * tri / 1000;
}

TOY(toy_fog, "fog", "Fog", "fog", ROLE_CONDUCTOR, HUE_GREY, 50, 2,
    "A conductor. Makes the band duller and brighter, like fog rolling in and lifting.",
    fog_t, F(make), 0, 0, F(randomise), 0, F(play_changed), F(frame), 0, F(close));
#undef DEPTH
#undef ROLL
#undef LIFT
#undef SHAPE
