/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 116 (Witchbox IV, texture): Chimes. Wind chimes: gusts come at random, and each gust sets off
 * a cluster of bell notes from the chord, close together in time, dying away as the wind drops. */
#include "toy_util.h"
#undef F
#define F(n) chm_##n

typedef struct { int gust, strength; } chimes_t;
#define WIND 0
#define CHIMES 1
#define CLUSTER 2
#define HIGH 3

static void F(make)(toy_t *t)
{
    add(t, "Wind", 0, 10, 4, "How often a gust comes");
    add(t, "Chimes", 3, 7, 5, "How many chimes hang on the frame");
    add(t, "Cluster", 0, 10, 5, "How many notes a gust sets off");
    add(t, "High", 0, 10, 5, "How high the chimes are hung");
}

static void F(randomise)(toy_t *t)
{
    P(WIND) = (int16_t)dice_int(11);
    P(CHIMES) = (int16_t)(3 + dice_int(5));
    P(CLUSTER) = (int16_t)dice_int(11);
    P(HIGH) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t) { ST(chimes_t)->gust = 0; }

static void F(on_step)(toy_t *t)
{
    chimes_t *s = ST(chimes_t);
    int8_t tones[12];
    int root = toy_root(t, 0) + P(HIGH) * 7 / 10, n;
    if (s->gust <= 0 && dice_lt(P(WIND), 60)) {
        s->gust = 2 + dice_int(2 + P(CLUSTER));
        s->strength = 100;
    }
    if (s->gust <= 0)
        return;
    s->gust--;
    s->strength -= 100 / (2 + P(CLUSTER));
    n = tune_tones(root, root, root + 9, tones);
    if (n > P(CHIMES))
        n = P(CHIMES);
    if (dice_lt(s->strength + 20, 130)) {
        int k = dice_int(n);
        toy_note(t, tones[k], s->strength > 50 ? 0 : 1, 0);
        if (dice_lt(P(CLUSTER), 14))           /* two chimes knock together */
            toy_note(t, tones[(k + 1) % n], 1, 0);
    }
}

TOY(toy_chimes, "chimes", "Chimes", "chimes", ROLE_TEXTURE, HUE_CYAN, 0, 3,
    "Wind chimes. Gusts come at random and each sets off a cluster of bell notes that dies away.",
    chimes_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef WIND
#undef CHIMES
#undef CLUSTER
#undef HIGH
