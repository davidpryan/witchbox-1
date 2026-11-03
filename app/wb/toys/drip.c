/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 88 (Witchbox III, bass): Acid Drip. A squelching bass line in quick notes: a sixteen-step
 * pattern of chord notes, some accented, held long enough to bend into the next. */
#include "toy_util.h"
#undef F
#define F(n) ad_##n

typedef struct { uint8_t rank[BAR], acc[BAR]; int8_t deg[BAR]; } drip_t;
#define GLIDE 0
#define ACCENT 1
#define NOTES 2
#define RANGE 3

static void F(deal)(toy_t *t)
{
    drip_t *s = ST(drip_t);
    int8_t tones[12];
    int i, n = tune_tones(0, -7, P(RANGE) - 1, tones);
    for (i = 0; i < BAR; i++) {
        s->rank[i] = (uint8_t)dice_int(10);
        s->acc[i] = (uint8_t)dice_int(10);
        s->deg[i] = tones[dice_int(n)];
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Glide", 0, 10, 4, "How long a glide takes, instant to slow");
    add(t, "Accent", 0, 10, 5, "How hard the accented notes bite");
    add(t, "Notes", 0, 10, 7, "How many of the 16 steps sound");
    add(t, "Range", 0, 10, 4, "How far up the notes leap");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(GLIDE) = (int16_t)dice_int(11);
    P(ACCENT) = (int16_t)dice_int(11);
    P(NOTES) = (int16_t)(3 + dice_int(8));
    P(RANGE) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(deal)(t); }

static void F(on_step)(toy_t *t)
{
    drip_t *s = ST(drip_t);
    int i = t->steps % BAR, hold;
    if (!(i == 0 || s->rank[i] < P(NOTES)))
        return;
    hold = t->step_ms * (10 + P(GLIDE) * 3) / 10;   /* a long hold overlaps the next note: it bends */
    toy_note(t, toy_root(t, 0) + s->deg[i], s->acc[i] * 2 < P(ACCENT) ? 2 : 0, hold);
}

TOY(toy_drip, "drip", "Acid Drip", "drip", ROLE_BASS, HUE_GO, 0, 2,
    "A squelching bass line in quick notes that bend into each other.",
    drip_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef GLIDE
#undef ACCENT
#undef NOTES
#undef RANGE
