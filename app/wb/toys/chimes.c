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

#ifdef WB_PICTURES
/* the picture: chimes hung from a bar, longer for lower notes, each swinging when struck; the gust as
 * a breath of lines across them */
static void F(draw)(toy_t *t)
{
    chimes_t *s = ST(chimes_t);
    int8_t tones[12];
    int root = toy_root(t, 0) + P(HIGH) * 7 / 10, n = tune_tones(root, root, root + 9, tones), i;
    if (n > P(CHIMES)) n = P(CHIMES);
    cv_rect(40, 10, 160, 3, K_TEXT);
    for (i = 0; i < n; i++) {
        int x = pic_col(i, n, 50), len = 60 + (9 - (tones[i] - root)) * 5;
        uint32_t ago = pic_ago_deg(t, tones[i]);
        int sw = ago < 800u ? PIC_SIN[(ago / 25u) & 31] * (int)(800u - ago) / 100 / 160 : 0;
        cv_rect(x, 13, 1, 8, K_SOFT);
        cv_rect(x - 3 + sw, 21, 6, len, ago < 150u ? pic_hue : K_TEXT);
    }
    if (s->gust > 0)
        for (i = 0; i < 4; i++)
            cv_rect(10 + i * 8 + (int)((plat_ms() / 10u) % 200u), 30 + i * 22, 24, 1, K_SOFT);
}
#endif

TOY(toy_chimes, "chimes", "Chimes", "chimes", ROLE_TEXTURE, HUE_CYAN, 0, 3, "Wind chimes. Gusts come at random and each sets off a cluster of bell notes that dies away.", chimes_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef WIND
#undef CHIMES
#undef CLUSTER
#undef HIGH
