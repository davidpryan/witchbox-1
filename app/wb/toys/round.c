/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 110 (Witchbox IV, chords): Round. A harp line through the chord, played as a canon: each
 * voice enters a little after the last with the same line, so the chord builds up on itself. */
#include "toy_util.h"
#undef F
#define F(n) rd_##n

static const uint8_t RD_DELAY[3] = {2, 4, 16};
static const char *const RD_DELAY_N[3] = {"2 steps", "1 beat", "1 bar"};
static const char *const RD_SHAPE_N[3] = {"Rising", "Falling", "Arch"};
typedef struct { int8_t line[8]; } round_t;
#define VOICES 0
#define DELAY 1
#define SHAPE 2
#define GAPS 3

static void F(lay)(toy_t *t)
{
    round_t *s = ST(round_t);
    static const int8_t RISE[8] = {0, 2, 4, 7, 9, 11, 14, 16};
    int i;
    for (i = 0; i < 8; i++)
        s->line[i] = P(SHAPE) == 0 ? RISE[i] : P(SHAPE) == 1 ? RISE[7 - i] : RISE[i < 4 ? i * 2 : (7 - i) * 2 + 1];
}

static void F(make)(toy_t *t)
{
    add(t, "Voices", 1, 4, 3, "How many voices join the round");
    add_pick(t, "Delay", 3, 1, RD_DELAY_N, "How long after the last each voice enters");
    add_pick(t, "Shape", 3, 0, RD_SHAPE_N, "The shape of the line");
    add(t, "Gaps", 0, 10, 3, "How often a note is left out");
    F(lay)(t);
}

static void F(randomise)(toy_t *t)
{
    P(VOICES) = (int16_t)(1 + dice_int(4));
    P(DELAY) = (int16_t)dice_int(3);
    P(SHAPE) = (int16_t)dice_int(3);
    P(GAPS) = (int16_t)dice_int(11);
    F(lay)(t);
}

static void F(changed)(toy_t *t) { F(lay)(t); }

static void F(on_step)(toy_t *t)
{
    round_t *s = ST(round_t);
    int v, now = t->steps, delay = RD_DELAY[P(DELAY)];
    for (v = 0; v < P(VOICES); v++) {
        int at = now - v * delay, k;
        if (at < 0)
            continue;
        if (at % 2 != 0)
            continue;                           /* the line is in quavers */
        k = (at / 2) % BAR;
        if (k >= 8)
            continue;                           /* the line is half a bar long, then rests */
        if (dice_lt(P(GAPS), 14))
            continue;
        toy_note(t, toy_root(t, 0) + s->line[k] - 7 + (v % 2 ? 7 : 0), v == 0 ? 0 : 1, 0);
    }
}

TOY(toy_round, "round", "Round", "harp", ROLE_CHORDS, HUE_YELLOW, 0, 3,
    "A harp line through the chord played as a canon: each voice enters a little after the last.",
    round_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef VOICES
#undef DELAY
#undef SHAPE
#undef GAPS
