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

#ifdef WB_PICTURES
/* the picture: the tune's notes as a seal's points, each out from the centre by its height and joined
 * by chords, the one struck in the hue and its echo in soft; the silence runs round the outer ring
 * taking its sand points with it until the tune comes round again */
static void F(draw)(toy_t *t)
{
    hexbell_t *s = ST(hexbell_t);
    static const uint8_t SIL[3] = {16, 32, 64};
    int n = P(NOTES), cycle = n * 2 + SIL[P(SILENCE)], at = toy_now(t) % cycle, i, k, gone;
    pic_circle(PIC_CX, PIC_CY, 64, K_SAND);
    for (i = 0; i < n; i++) {
        int a = pic_ang(i, n), b = pic_ang(i + 1, n), r0 = 24 + s->tune[i] * 2, r1 = 24 + s->tune[(i + 1) % n] * 2;
        cv_line(pic_px(PIC_CX, r0, a), pic_py(PIC_CY, r0, a), pic_px(PIC_CX, r1, b), pic_py(PIC_CY, r1, b), K_LINE);
    }
    if (at < n * 2 && t->playing)
        pic_needle(PIC_CX, PIC_CY, 24 + s->tune[at / 2] * 2, at / 2, n, pic_hue);
    for (i = 0; i < n; i++) {
        int lit = at / 2 == i && at < n * 2 && t->playing;
        pic_pt(PIC_CX, PIC_CY, 24 + s->tune[i] * 2, i, n, lit ? 6 : 4, lit ? pic_hue : K_TEXT);
        if (s->echo_at >= 0 && s->echo_deg == toy_root(t, 0) + s->tune[i])
            pic_pt(PIC_CX, PIC_CY, 24 + s->tune[i] * 2, i, n, 2, K_SOFT);
    }
    gone = at >= n * 2 ? (at - n * 2) * 32 / (cycle - n * 2) : 0;
    for (k = 0; k < 32 - gone; k++)
        pic_pt(PIC_CX, PIC_CY, 60, k, 32, 1, K_SAND);
    pic_rune(PIC_CX - 4, PIC_CY - 6, at < n * 2 ? RUNE_LEAD : RUNE_REST, at < n * 2 ? K_TEXT : K_SAND);
}
#endif

TOY(toy_hexbell, "hexbell", "Hex Bell", "bell", ROLE_LEAD, HUE_YELLOW, 0, 2, "A short tune on a bright bell, then a long silence, then the tune again.", hexbell_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef NOTES
#undef SILENCE
#undef LEAP
#undef ECHO
