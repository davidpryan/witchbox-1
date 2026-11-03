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

TOY(toy_murder, "murder", "Murder", "crow", ROLE_TEXTURE, HUE_GREY, 0, 2,
    "Crows calling from a wire, near and far, with a flurry of wings.",
    murder_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef CROWS
#undef CHATTER
#undef FAR
#undef WINGS
