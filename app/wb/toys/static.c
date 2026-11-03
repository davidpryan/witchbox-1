/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 118 (Witchbox IV, texture): Static. Record crackle and rumble driven by 1/f noise: several
 * dice, each rerolled half as often as the last (Voss's trick), summed. The slow dice bring the
 * rumble, the fast ones the pops, and the sum has the drift of a bad signal. */
#include "toy_util.h"
#undef F
#define F(n) stc_##n

typedef struct { uint8_t dice[5]; int n; } static_t;
#define DICE 0
#define POPS 1
#define LOW 2
#define SKIPS 3

static void F(make)(toy_t *t)
{
    add(t, "Dice", 2, 5, 4, "How many dice make the noise: more is slower drift");
    add(t, "Pops", 0, 10, 5, "How loud the signal must be for a pop");
    add(t, "Low", 0, 10, 4, "How often the rumble comes up");
    add(t, "Skips", 0, 10, 2, "How often the record skips");
}

static void F(randomise)(toy_t *t)
{
    P(DICE) = (int16_t)(2 + dice_int(4));
    P(POPS) = (int16_t)dice_int(11);
    P(LOW) = (int16_t)dice_int(11);
    P(SKIPS) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    static_t *s = ST(static_t);
    int k, sum = 0, nd = P(DICE), step = t->steps;
    for (k = 0; k < nd; k++) {                  /* die k is rerolled every 2^k steps */
        if (step % (1 << k) == 0)
            s->dice[k] = (uint8_t)dice_int(10);
        sum += s->dice[k];
    }
    sum = sum * 10 / nd;                        /* 0..90 */
    if (sum > 40 + (10 - P(POPS)) * 4)
        toy_note(t, 0, 0, 0);                   /* a pop */
    if (step % 8 == 0 && s->dice[nd - 1] * 11 < P(LOW) * 10)
        toy_note(t, 0, 1, t->step_ms * 8);      /* the rumble, on the slowest die */
    if (step % 4 == 0 && dice_lt(P(SKIPS), 60))
        toy_note(t, 0, 2, 0);
    if (sum < 30 && dice_lt_q(13107))
        toy_note(t, 0, 3, 0);                   /* a tick in the quiet */
}

#ifdef WB_PICTURES
/* the picture: the dice as bars, the slow ones at the left, their sum as the signal's level across the
 * top; pops as specks where the level is high */
static void F(draw)(toy_t *t)
{
    static_t *s = ST(static_t);
    int k, nd = P(DICE), sum = 0;
    for (k = 0; k < nd; k++) {
        sum += s->dice[k];
        cv_rect(30 + k * 36, 120 - s->dice[k] * 9, 24, s->dice[k] * 9 + 2, k == nd - 1 ? K_SOFT : K_TEXT);
    }
    sum = sum * 10 / nd;
    cv_rect(12, 36 - sum / 5, 216, 2, pic_hue);
    cv_rect(12, 36 - (40 + (10 - P(POPS)) * 4) / 5, 216, 1, K_LINE);
    if (pic_ago(t, 0) < 100u)
        for (k = 0; k < 6; k++)
            cv_rect(12 + (int)(pic_hash(k, plat_ms() / 100u) % 216u), 20 + (int)(pic_hash(k, 9) % 20u), 2, 2, K_TEXT);
    cv_text(30, 126, &FONT_XS, "FAST", K_DIM);
    cv_text(30 + (nd - 1) * 36, 126, &FONT_XS, "SLOW", K_DIM);
}
#endif

TOY(toy_static, "static", "Static", "static", ROLE_TEXTURE, HUE_GREY, 0, 3, "Crackle and rumble driven by 1/f noise: dice rerolled at different rates and summed, with the drift of a bad signal.", static_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef DICE
#undef POPS
#undef LOW
#undef SKIPS
