/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 97 (Witchbox III, texture): Murder. Crows calling from a wire, near and far: one call sets
 * the others off in a chain, and sometimes a flurry of wings. */
#include "toy_util.h"
#undef F
#define F(n) mu_##n

typedef struct { int chain; } murder_t;
#define CROWS 0
#define CHATTER 1
#define FAR 2
#define WINGS 3

static void F(make)(toy_t *t)
{
    add(t, "Crows", 1, 8, 4, "How many crows are on the wire");
    add(t, "Chatter", 0, 10, 5, "How often one call sets the others off");
    add(t, "Far", 0, 10, 4, "How many of the calls are a long way off");
    add(t, "Wings", 0, 10, 3, "How often there is a flurry of wings");
}

static void F(randomise)(toy_t *t)
{
    P(CROWS) = (int16_t)(1 + dice_int(8));
    P(CHATTER) = (int16_t)dice_int(11);
    P(FAR) = (int16_t)dice_int(11);
    P(WINGS) = (int16_t)dice_int(11);
}

static void F(call)(toy_t *t)
{
    int far = dice_lt(P(FAR), 11);
    toy_note(t, far ? -2 - dice_int(3) : dice_int(3) - 1, far ? 1 : 0, 0);
}

static void F(on_step)(toy_t *t)
{
    murder_t *s = ST(murder_t);
    if (s->chain > 0) {
        s->chain--;
        if (dice_lt_q(45875))                   /* 70%: the next crow answers */
            F(call)(t);
        return;
    }
    if (dice_lt(P(CROWS), 48)) {
        F(call)(t);
        if (dice_lt(P(CHATTER), 11))
            s->chain = 1 + dice_int(P(CROWS));
        if (dice_lt(P(WINGS), 11))
            toy_note(t, 0, 2, 0);
    }
}

#ifdef WB_PICTURES
/* the picture: the crows as the points of a seal, as many as are on the wire; the one calling swells
 * in the hue, a far call small in soft, and while the chain runs the chords from crow to crow light
 * up one by one; the wings are a needle that sweeps round */
static void F(draw)(toy_t *t)
{
    murder_t *s = ST(murder_t);
    int n = P(CROWS), i;
    uint32_t near = pic_ago(t, 0), far = pic_ago(t, 1), wings = pic_ago(t, 2);
    pic_seal(PIC_CX, PIC_CY, 64, n, n > 2 ? 1 : 0, K_SAND);
    if (s->chain > 0 && t->playing)
        for (i = 0; i < n - s->chain && i + 1 < n; i++)
            pic_chord(PIC_CX, PIC_CY, 58, i, i + 1, n, K_SOFT);
    if (wings < 500u)
        pic_needle(PIC_CX, PIC_CY, 58, (int)(wings * 32u / 500u), 32, K_TEXT);
    for (i = 0; i < n; i++) {
        int who = n - 1 - (s->chain > 0 ? s->chain % n : 0), calling = near < 200u && i == who, small = i % 3 == 2;
        int farcall = small && far < 200u;
        pic_pt(PIC_CX, PIC_CY, 58, i, n, calling ? 7 : farcall ? 4 : small ? 3 : 5, calling ? pic_hue : farcall ? K_SOFT : K_TEXT);
    }
    pic_rune(PIC_CX - 4, PIC_CY - 6, RUNE_EYE, near < 200u || far < 200u ? pic_hue : K_SOFT);
}
#endif

TOY(toy_murder, "murder", "Murder", "crow", ROLE_TEXTURE, HUE_GREY, 0, 2, "Crows calling from a wire, near and far, with a flurry of wings.", murder_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef CROWS
#undef CHATTER
#undef FAR
#undef WINGS
