/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 82 (Witchbox III, drums): Chopped Break. A two-bar drum break cut into eight slices of two
 * steps and put back in the wrong order: some slices swapped, some played twice. */
#include "toy_util.h"
#undef F
#define F(n) cb_##n

/* the break: bit 0 kick, 1 snare, 2 closed hat, 3 open hat, 4 crash */
static const uint8_t CB_BREAK[16] = {0x15, 0x04, 0x06, 0x04, 0x01, 0x05, 0x06, 0x0C,
                                     0x05, 0x04, 0x06, 0x01, 0x04, 0x05, 0x06, 0x0C};
typedef struct { uint8_t order[8], rank[8]; int hold; } chopbreak_t;
#define SWAPS 0
#define SHUFFLE 1
#define REPEATS 2
#define HATS 3

static void F(deal)(toy_t *t)
{
    chopbreak_t *s = ST(chopbreak_t);
    int i, n = P(SWAPS);
    for (i = 0; i < 8; i++)
        s->order[i] = (uint8_t)i;
    for (i = 0; i < n; i++) {                   /* swap this many pairs */
        int a = dice_int(8), b = dice_int(8), x = s->order[a];
        s->order[a] = s->order[b];
        s->order[b] = (uint8_t)x;
    }
    for (i = 0; i < 8; i++)
        s->rank[i] = (uint8_t)dice_int(10);
}

static void F(make)(toy_t *t)
{
    add(t, "Swaps", 0, 8, 3, "How many slices are out of place");
    add(t, "Shuffle", 0, 10, 2, "How often the slices are shuffled again");
    add(t, "Repeats", 0, 10, 3, "How often a slice repeats");
    add(t, "Hats", 0, 10, 6, "How busy the hats are");
    ST(chopbreak_t)->hold = -1;
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(SWAPS) = (int16_t)dice_int(9);
    P(SHUFFLE) = (int16_t)dice_int(11);
    P(REPEATS) = (int16_t)dice_int(11);
    P(HATS) = (int16_t)(3 + dice_int(8));
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    chopbreak_t *s = ST(chopbreak_t);
    int i = t->steps % BAR, slot = i / 2, sub = i % 2, slice, m;
    if (i == 0 && dice_lt(P(SHUFFLE), 11))
        F(deal)(t);
    if (sub == 0) {
        if (s->hold >= 0) {
            slice = s->hold;
            s->hold = -1;
        } else {
            slice = s->order[slot];
            if (dice_lt(P(REPEATS), 11))
                s->hold = slice;                /* the next slot plays this one again */
        }
        s->rank[7] = (uint8_t)slice;            /* remembered for the second step */
    } else {
        slice = s->rank[7];
    }
    m = CB_BREAK[slice * 2 + sub];
    if (m & 1)
        toy_kick(t);
    if (m & 2)
        toy_note(t, 0, 1, 0);
    if ((m & 4) && dice_lt(P(HATS), 10))
        toy_note(t, 0, 2, 0);
    if ((m & 8) && dice_lt(P(HATS), 10))
        toy_note(t, 0, 3, 0);
    if ((m & 16) && toy_now(t) % 64 == 0)
        toy_note(t, 0, 4, 0);
}

#ifdef WB_PICTURES
/* the picture: the break as a wheel of eight slots; a chord runs from each slot to where its slice
 * really belongs, so a swapped break is a tangle; the slot playing is the needle's, in the hue, and a
 * repeat shows as the hold rune at the centre */
static void F(draw)(toy_t *t)
{
    chopbreak_t *s = ST(chopbreak_t);
    int i, now = (t->steps % BAR) / 2, sub = t->steps % 2;
    pic_seal(PIC_CX, PIC_CY, 64, 8, 0, K_SAND);
    for (i = 0; i < 8; i++)
        if (s->order[i] != i)
            pic_chord(PIC_CX, PIC_CY, 50, i, s->order[i], 8, K_SOFT);
    pic_needle(PIC_CX, PIC_CY, 50, now, 8, t->playing ? pic_hue : K_LINE);
    for (i = 0; i < 8; i++) {
        int cur = i == now && t->playing;
        pic_pt(PIC_CX, PIC_CY, 50, i, 8, cur ? 6 - sub * 2 : s->order[i] == i ? 3 : 4, cur ? pic_hue : s->order[i] == i ? K_TEXT : K_SOFT);
    }
    for (i = 0; i < BAR; i++)
        pic_pt(PIC_CX, PIC_CY, 61, i, BAR, 0, i == t->steps % BAR && t->playing ? K_TEXT : K_SAND);
    if (s->hold >= 0)
        pic_rune(PIC_CX - 4, PIC_CY - 6, RUNE_HOLD, K_TEXT);
    else
        pic_rune(PIC_CX - 4, PIC_CY - 6, RUNE_KICK, K_SAND);
}
#endif

TOY(toy_chopbreak, "chopbreak", "Chopped Break", "chop", ROLE_DRUMS, HUE_ORANGE, 0, 2, "A drum break cut into eight slices and put back in the wrong order.", chopbreak_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef SWAPS
#undef SHUFFLE
#undef REPEATS
#undef HATS
