/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 46 (Witchbox, bass): Undertow. A deep sub bass: a long note on the chord's root one step
 * behind the kick, beating slowly against itself, often resting, now and then up to the fifth. */
#include "toy_util.h"
#undef F
#define F(n) ut_##n

static const uint8_t UT_HOLD[4] = {4, 8, 16, 32};
static const char *const UT_HOLD_N[4] = {"1 beat", "2 beats", "1 bar", "2 bars"};

typedef struct { int left, want, lvl, amp, ph, deg; } undertow_t;

#define HOLD 0
#define WOBBLE 1
#define MOVES 2
#define GAPS 3

static void F(make)(toy_t *t)
{
    undertow_t *s = ST(undertow_t);
    add_pick(t, "Hold", 4, 2, UT_HOLD_N, "How long each note lasts");
    add(t, "Wobble", 0, 10, 4, "How fast the note beats, still to fast");
    add(t, "Movement", 0, 10, 2, "How often it leaves the root");
    add(t, "Gaps", 0, 10, 4, "How often it rests");
    s->want = s->lvl = 150;
    s->deg = -7;
}

static void F(randomise)(toy_t *t)
{
    P(HOLD) = (int16_t)dice_int(4);
    P(WOBBLE) = (int16_t)dice_int(11);
    P(MOVES) = (int16_t)dice_int(6);
    P(GAPS) = (int16_t)dice_int(8);
}

static void F(play_changed)(toy_t *t)
{
    if (!t->playing)
        ST(undertow_t)->left = 0;
}

static void F(on_step)(toy_t *t)
{
    undertow_t *s = ST(undertow_t);
    int i = t->steps % BAR;
    if (s->left > 0)
        s->left--;
    /* a note may start one step after beat 1, and after beat 3 for short notes */
    if ((i == 1 || (i == 9 && P(HOLD) < 2)) && s->left <= 0) {
        int deg;
        if (dice_lt(P(GAPS), 11))
            return;                 /* a rest */
        deg = toy_root(t, 0) - 7;
        if (dice_lt(P(MOVES), 20))  /* rarely: up to the fifth */
            deg += 4;
        toy_note(t, deg, 0, 0);
        s->left = UT_HOLD[P(HOLD)];
        s->deg = deg;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    undertow_t *s = ST(undertow_t);
    int on = t->playing && s->left > 0;
    int amp = on ? 6 + P(WOBBLE) * 2 : 0;
    if (t->playing)
        s->ph += ms * P(WOBBLE);
    if (s->amp != amp)
        s->amp += imax(-2, imin(2, amp - s->amp));
    if (s->lvl != s->want)
        s->lvl += imax(-6, imin(6, s->want - s->lvl));
}

TOY(toy_undertow, "undertow", "Undertow", "wave", ROLE_BASS, HUE_BLUE, 40, 0,
    "A deep sub bass. Long notes arrive just after the kick, with a slow wobble.",
    undertow_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef HOLD
#undef WOBBLE
#undef MOVES
#undef GAPS
