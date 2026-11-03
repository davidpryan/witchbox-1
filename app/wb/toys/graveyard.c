/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 91 (Witchbox III, chords): Graveyard Guitar. A guitar picking the chord one string at a
 * time — up, down, both ways or at random — and letting every string ring. */
#include "toy_util.h"
#undef F
#define F(n) gy_##n

static const uint8_t GY_EVERY[3] = {2, 1, 3};
static const char *const GY_SPEED_N[3] = {"Eighths", "Sixteenths", "In threes"};
static const char *const GY_ORDER_N[4] = {"Up", "Down", "Both", "Random"};
typedef struct { int at, dir; } graveyard_t;
#define SPEED 0
#define ORDER 1
#define SKIPS 2
#define SPREAD 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Speed", 3, 0, GY_SPEED_N, "How fast the strings are picked");
    add_pick(t, "Order", 4, 2, GY_ORDER_N, "The order the strings are picked in");
    add(t, "Skips", 0, 10, 2, "How often a pick is left out");
    add(t, "Spread", 0, 10, 5, "How far apart the chord's notes sit");
    ST(graveyard_t)->dir = 1;
}

static void F(randomise)(toy_t *t)
{
    P(SPEED) = (int16_t)dice_int(3);
    P(ORDER) = (int16_t)dice_int(4);
    P(SKIPS) = (int16_t)dice_int(11);
    P(SPREAD) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    graveyard_t *s = ST(graveyard_t);
    int8_t tones[16];
    int root = toy_root(t, 0) - 7, n, i;
    if (t->steps % GY_EVERY[P(SPEED)] != 0)
        return;
    n = tune_tones(root, root, root + 4 + P(SPREAD), tones);
    if (n > 6)
        n = 6;                                  /* six strings */
    switch (P(ORDER)) {
    case 0: i = s->at % n; s->at++; break;
    case 1: i = n - 1 - s->at % n; s->at++; break;
    case 2:
        i = s->at;
        s->at += s->dir;
        if (s->at >= n - 1 || s->at <= 0)
            s->dir = -s->dir;
        if (i >= n)
            i = n - 1;
        break;
    default: i = dice_int(n); break;
    }
    if (s->at >= 64)
        s->at = s->at % n;
    if (dice_lt(P(SKIPS), 14))
        return;
    toy_note(t, tones[i], 0, 1800);
}

#ifdef WB_PICTURES
/* the picture: the six strings as the six points of a hexagram seal, the ones the chord uses in
 * cream and the rest sand; the hand is the needle, and a string plucked swells in the hue and shivers
 * for a moment */
static void F(draw)(toy_t *t)
{
    graveyard_t *s = ST(graveyard_t);
    int8_t tones[16];
    int root = toy_root(t, 0) - 7, n = tune_tones(root, root, root + 4 + P(SPREAD), tones), i;
    if (n > 6) n = 6;
    pic_seal(PIC_CX, PIC_CY, 64, 6, 2, K_SAND);
    pic_needle(PIC_CX, PIC_CY, 58, s->at % imax(1, n), 6, t->playing ? K_SOFT : K_LINE);
    for (i = 0; i < 6; i++) {
        uint32_t ago = i < n ? pic_ago_deg(t, tones[i]) : 100000u;
        int a = pic_ang(i, 6), wob = ago < 300u ? ((int)(ago / 40u) % 2 * 2 - 1) * (int)(3 - ago / 120u) : 0;
        if (i < n)
            pic_disc(pic_px(PIC_CX, 58, a) + wob, pic_py(PIC_CY, 58, a), ago < 300u ? 6 : 4, ago < 300u ? pic_hue : K_TEXT);
        else
            pic_pt(PIC_CX, PIC_CY, 58, i, 6, 1, K_SAND);
    }
    pic_rune(PIC_CX - 4, PIC_CY - 6, P(ORDER) == 0 ? RUNE_UP : P(ORDER) == 1 ? RUNE_DOWN : P(ORDER) == 2 ? RUNE_HOLD : RUNE_CROSS, K_TEXT);
}
#endif

TOY(toy_graveyard, "graveyard", "Graveyard Guitar", "guitar", ROLE_CHORDS, HUE_GREY, 0, 2, "A guitar picking the chord one string at a time and letting it ring.", graveyard_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef SPEED
#undef ORDER
#undef SKIPS
#undef SPREAD
