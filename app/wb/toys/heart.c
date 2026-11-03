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

#ifdef WB_PICTURES
/* the picture: the heart as an orb inside a seal of the beat's steps: it fills in the hue on every
 * lub and drains after; a dub, or a flutter, sends a ring out from it that fades as it spreads; a
 * needle for the step round the beat */
static void F(draw)(toy_t *t)
{
    int period = HT_PERIOD[P(PACE)], at = t->steps % period, cx = PIC_CX, cy = PIC_CY, fill, i;
    uint32_t lub = pic_ago(t, 0), dub = pic_ago(t, 1);
    pic_seal(cx, cy, 64, period, 0, K_SAND);
    for (i = 0; i < period; i++)
        pic_pt(cx, cy, 58, i, period, i == 0 ? 2 : 1, i == 0 ? K_TEXT : K_SAND);
    pic_needle(cx, cy, 58, at, period, K_LINE);
    fill = lub < 500u ? 1000 - (int)lub * 650 / 500 : 350;
    pic_orb(cx, cy, 40, fill, K_TEXT, pic_hue);
    if (dub < 600u)
        pic_circle(cx, cy, 42 + (int)dub * 20 / 600, dub < 200u ? K_TEXT : dub < 400u ? K_SOFT : K_SAND);
}
#endif

TOY(toy_heart, "heart", "Heartbeat", "heart", ROLE_DRUMS, HUE_RED, 0, 2, "Only a kick drum, beating like a heart. Now and then it flutters or misses.", heart_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef PACE
#undef DUB
#undef FLUTTER
#undef SKIPS
