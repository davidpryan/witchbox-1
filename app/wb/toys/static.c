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
/* the picture: the dice as a row of spires, the fast ones at the left and the slow one soft at the
 * right, each filled by its roll; their sum as an orb above that fills with the signal, in the hue
 * past the pop line, and sparks round it on the ring when a pop goes off */
static void F(draw)(toy_t *t)
{
    static_t *s = ST(static_t);
    int k, nd = P(DICE), sum = 0, w = 200 / nd, pop = 40 + (10 - P(POPS)) * 4, hot = pic_ago(t, 0) < 100u;
    for (k = 0; k < nd; k++) {
        sum += s->dice[k];
        pic_spire(20 + k * w + w / 2, 132, w - 10, 44, s->dice[k] * 1000 / 9, K_SAND, k == nd - 1 ? K_SOFT : K_TEXT);
    }
    pic_hline(12, PW - 12, 134, K_TEXT);
    sum = sum * 10 / nd;
    pic_circle(PIC_CX, 42, 30, K_SAND);
    pic_orb(PIC_CX, 42, 24, sum * 1000 / 90, K_SAND, sum > pop ? pic_hue : K_SOFT);
    pic_circle(PIC_CX, 42, 24 * pop / 90, K_LINE);
    if (hot)
        for (k = 0; k < 6; k++)
            pic_pt(PIC_CX, 42, 30, (int)(pic_hash(k, plat_ms() / 100u) & 31u), 32, 2, K_TEXT);
    if (pic_ago(t, 1) < (uint32_t)t->step_ms * 8u)
        pic_rune(PIC_CX - 4, 72, RUNE_NOISE, K_SOFT);
}
#endif

TOY(toy_static, "static", "Static", "static", ROLE_TEXTURE, HUE_GREY, 0, 3, "Crackle and rumble driven by 1/f noise: dice rerolled at different rates and summed, with the drift of a bad signal.", static_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef DICE
#undef POPS
#undef LOW
#undef SKIPS
