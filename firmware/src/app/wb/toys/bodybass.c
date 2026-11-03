/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 86 (Witchbox III, bass): Body Bass. A plucked bass that carries a tune: it sits on the
 * chord's notes and, towards the end of the bar, walks a step at a time into the next chord. */
#include "toy_util.h"
#undef F
#define F(n) bb_##n

static const uint8_t BB_EVERY[3] = {2, 4, 0};
static const char *const BB_NOTES_N[3] = {"Eighths", "Quarters", "Mixed"};
typedef struct { int deg, target; } bodybass_t;
#define WALK 0
#define NOTES 1
#define RANGE 2
#define RESTS 3

static void F(make)(toy_t *t)
{
    add(t, "Walk", 0, 10, 6, "How often it walks into the next chord");
    add_pick(t, "Notes", 3, 2, BB_NOTES_N, "How fast the notes come");
    add(t, "Range", 0, 10, 4, "How far the notes stray from the root");
    add(t, "Rests", 0, 10, 2, "How often it rests");
    ST(bodybass_t)->deg = -7;
}

static void F(randomise)(toy_t *t)
{
    P(WALK) = (int16_t)dice_int(11);
    P(NOTES) = (int16_t)dice_int(3);
    P(RANGE) = (int16_t)dice_int(11);
    P(RESTS) = (int16_t)dice_int(8);
}

static void F(on_step)(toy_t *t)
{
    bodybass_t *s = ST(bodybass_t);
    int i = t->steps % BAR, every = BB_EVERY[P(NOTES)], root = toy_root(t, 0) - 7, deg;
    int8_t tones[12];
    if (every == 0)
        every = (i % 4 == 0 || dice_lt_q(26214)) ? 2 : 4;   /* mixed: a quaver 40% of the time */
    if (i % every != 0)
        return;
    if (i > 0 && dice_lt(P(RESTS), 16))
        return;
    if (i >= 12 && dice_lt(P(WALK), 11)) {     /* the last beat: walk towards the next root */
        int next = toy_root(t, BAR - i) - 7;
        if (next > s->deg + 3)
            next -= 7;
        else if (next < s->deg - 3)
            next += 7;
        deg = s->deg + (next > s->deg ? 1 : next < s->deg ? -1 : 0);
    } else if (i == 0) {
        deg = root;
    } else {
        int n = tune_tones(root, root, root + 1 + P(RANGE) * 3 / 5, tones);
        deg = tones[dice_int(n)];
    }
    s->deg = deg;
    toy_note(t, deg, 0, 0);
}

TOY(toy_bodybass, "bodybass", "Body Bass", "pluck", ROLE_BASS, HUE_ORANGE, 0, 2,
    "A plucked bass that carries a tune, walking between the notes of the chord.",
    bodybass_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef WALK
#undef NOTES
#undef RANGE
#undef RESTS
