/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 95 (Witchbox III, lead): Helium. A squeaky, pitched-up voice singing a quick phrase, then,
 * after a gap, its answer: the same phrase lower, higher or backwards. */
#include "toy_util.h"
#undef F
#define F(n) he_##n

static const uint8_t HE_GAP[2] = {16, 32};
static const char *const HE_GAP_N[2] = {"1 bar", "2 bars"};
static const char *const HE_ANSWER_N[3] = {"Lower", "Higher", "Backwards"};
typedef struct { int8_t phrase[6]; } helium_t;
#define PHRASE 0
#define ANSWER 1
#define GAP 2
#define JUMP 3

static void F(deal)(toy_t *t)
{
    helium_t *s = ST(helium_t);
    int i, deg = 7;
    for (i = 0; i < 6; i++) {
        s->phrase[i] = (int8_t)deg;
        deg += dice_int(P(JUMP) / 2 * 2 + 3) - (P(JUMP) / 2 + 1);
        deg = imax(4, imin(12, deg));
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Phrase", 2, 6, 4, "How many notes the phrase has");
    add_pick(t, "Answer", 3, 0, HE_ANSWER_N, "How the answer differs from the call");
    add_pick(t, "Gap", 2, 0, HE_GAP_N, "How long between the call and the answer");
    add(t, "Jump", 0, 10, 4, "How far the phrase jumps about");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(PHRASE) = (int16_t)(2 + dice_int(5));
    P(ANSWER) = (int16_t)dice_int(3);
    P(GAP) = (int16_t)dice_int(2);
    P(JUMP) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(deal)(t); }

static void F(on_step)(toy_t *t)
{
    helium_t *s = ST(helium_t);
    int gap = HE_GAP[P(GAP)], n = P(PHRASE), at = toy_now(t) % (gap * 2), deg, k;
    if (at % 2 != 0)
        return;
    k = (at % gap) / 2;
    if (k >= n)
        return;
    if (at < gap) {
        deg = s->phrase[k];
    } else {
        switch (P(ANSWER)) {
        case 0: deg = s->phrase[k] - 3; break;
        case 1: deg = s->phrase[k] + 2; break;
        default: deg = s->phrase[n - 1 - k]; break;
        }
    }
    deg += toy_root(t, 0);
    if (k == n - 1)
        deg = tune_nearest(deg, toy_root(t, 0));
    toy_note(t, deg, 0, t->step_ms * 3 / 2);
}

#ifdef WB_PICTURES
/* the picture: two seals, the call's and the answer's, each with the phrase's notes as points out from
 * the centre by their height (the answer's lower, higher or turned round); the seal singing is in
 * cream with its note in the hue, the other sleeps in sand; the orb inside each rises with the voice */
static void F(draw)(toy_t *t)
{
    helium_t *s = ST(helium_t);
    int gap = HE_GAP[P(GAP)], n = P(PHRASE), at = toy_now(t) % (gap * 2), k = (at % gap) / 2, h, i;
    uint32_t ago = pic_ago(t, -1);
    int lift = ago < 1500u ? (int)(1500u - ago) * 1000 / 1500 : 0;
    for (h = 0; h < 2; h++) {
        int cx = h ? 176 : 64, live = (at >= gap) == h && t->playing;
        pic_seal(cx, PIC_CY, 50, n, 0, live ? K_SOFT : K_SAND);
        pic_orb(cx, PIC_CY, 14, live ? lift : 0, K_SAND, pic_hue);
        for (i = 0; i < n; i++) {
            int deg = h ? (P(ANSWER) == 0 ? s->phrase[i] - 3 : P(ANSWER) == 1 ? s->phrase[i] + 2 : s->phrase[n - 1 - i]) : s->phrase[i];
            int cur = live && k == i && k < n && at % 2 == 0;
            pic_pt(cx, PIC_CY, 20 + deg * 2, i, n, cur ? 5 : 3, cur ? pic_hue : live ? K_TEXT : K_SAND);
        }
        if (live && k < n)
            pic_needle(cx, PIC_CY, 44, k, n, K_LINE);
    }
    pic_rune(PIC_CX - 4, PIC_CY - 6, P(ANSWER) == 0 ? RUNE_DOWN : P(ANSWER) == 1 ? RUNE_UP : RUNE_CROSS, K_TEXT);
}
#endif

TOY(toy_helium, "helium", "Helium", "balloon", ROLE_LEAD, HUE_PINK, 0, 2, "A squeaky, pitched-up voice singing a quick phrase and its answer.", helium_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef PHRASE
#undef ANSWER
#undef GAP
#undef JUMP
