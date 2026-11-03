/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 85 (Witchbox III, drums): Phonk Bell. A slow, heavy beat — kick, clap, hats — with a tune
 * played on a cowbell over it, from a few of the chord's notes. */
#include "toy_util.h"
#undef F
#define F(n) phk_##n

typedef struct { uint8_t on[BAR], rank[BAR]; int8_t deg[BAR]; } phonk_t;
#define NOTES 0
#define BELL 1
#define HATS 2
#define BOUNCE 3

static void F(lay)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int i;
    for (i = 0; i < BAR; i++)
        s->on[i] = s->rank[i] < P(BELL);
}

static void F(deal)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int8_t tones[8];
    int i, n = tune_tones(0, 0, 11, tones);     /* the chord's notes, as offsets from its root */
    for (i = 0; i < BAR; i++) {
        s->rank[i] = (uint8_t)dice_int(10);
        s->deg[i] = tones[dice_int(imin(n, P(NOTES)))];
    }
    F(lay)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Notes", 1, 5, 3, "How many different notes the bell plays");
    add(t, "Bell", 0, 10, 5, "How busy the bell is");
    add(t, "Hats", 0, 10, 6, "How busy the hats are");
    add(t, "Bounce", 0, 10, 4, "How often an extra kick bounces in");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(NOTES) = (int16_t)(1 + dice_int(5));
    P(BELL) = (int16_t)(2 + dice_int(9));
    P(HATS) = (int16_t)dice_int(11);
    P(BOUNCE) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(lay)(t); }

static void F(on_step)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int i = t->steps % BAR;
    if (i == 0 || i == 10)
        toy_kick(t);
    else if ((i == 6 || i == 14) && dice_lt(P(BOUNCE), 20))
        toy_kick(t);
    if (i == 8)
        toy_note(t, 0, 1, 0);
    if (i % 2 == 0 ? P(HATS) > 0 : dice_lt(P(HATS), 14))
        toy_note(t, 0, 3, 0);
    if (s->on[i])
        toy_note(t, toy_root(t, 0) + s->deg[i], 2, 0);
}

#ifdef WB_PICTURES
/* the picture: the bar as a wheel of sixteen; the bell's notes on the outer ring, out from the centre
 * by their height, the kicks as big points and the clap as a snare rune on the inner ring, the hats as
 * sand dots; the needle is the playhead and what sounds now is in the hue */
static void F(draw)(toy_t *t)
{
    phonk_t *s = ST(phonk_t);
    int i, now = t->steps % BAR;
    pic_seal(PIC_CX, PIC_CY, 64, BAR, 0, K_SAND);
    pic_circle(PIC_CX, PIC_CY, 26, K_SAND);
    pic_needle(PIC_CX, PIC_CY, 58, now, BAR, t->playing ? pic_hue : K_LINE);
    for (i = 0; i < BAR; i++) {
        int cur = i == now && t->playing;
        if (s->on[i])
            pic_pt(PIC_CX, PIC_CY, 40 + s->deg[i] * 2, i, BAR, cur ? 5 : 3, cur ? pic_hue : K_TEXT);
        else
            pic_pt(PIC_CX, PIC_CY, 58, i, BAR, 0, K_SAND);
        if (i == 0 || i == 10)
            pic_pt(PIC_CX, PIC_CY, 26, i, BAR, cur ? 6 : 4, cur ? pic_hue : K_SOFT);
        else if (i == 8)
            pic_rune_at(PIC_CX, PIC_CY, 26, i, BAR, RUNE_SNARE, cur ? pic_hue : K_SOFT);
        else if (i % 2 == 0 && P(HATS) > 0)
            pic_pt(PIC_CX, PIC_CY, 26, i, BAR, 1, cur ? K_TEXT : K_SAND);
    }
    pic_disc(PIC_CX, PIC_CY, 3, K_TEXT);
}
#endif

TOY(toy_phonk, "phonk", "Phonk Bell", "bell", ROLE_DRUMS, HUE_PURPLE, 0, 2, "A slow, heavy beat with a tune played on a cowbell over it.", phonk_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef NOTES
#undef BELL
#undef HATS
#undef BOUNCE
