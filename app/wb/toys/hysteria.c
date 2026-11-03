/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 107 (Witchbox IV, bass): Hysteria. The logistic map, x -> r x (1 - x), picks every note: at
 * low Chaos it settles on one note, then flips between two, then four, then goes wild. */
#include "toy_util.h"
#undef F
#define F(n) hy_##n

#define HY_HIST 24
typedef struct { int x; uint16_t xs[HY_HIST]; int hp; } hysteria_t;   /* x 0..4095; xs: the picture's history */
#define CHAOS 0
#define RANGE 1
#define RATE 2
#define SKIPS 3

static void F(make)(toy_t *t)
{
    add(t, "Chaos", 0, 10, 6, "From one steady note, through doubling, to chaos");
    add(t, "Range", 1, 8, 4, "How many chord notes it can reach");
    add(t, "Rate", 1, 4, 2, "Steps between notes");
    add(t, "Skips", 0, 10, 3, "How often a low x is a rest instead");
    ST(hysteria_t)->x = 1000;
}

static void F(randomise)(toy_t *t)
{
    P(CHAOS) = (int16_t)dice_int(11);
    P(RANGE) = (int16_t)(1 + dice_int(8));
    P(RATE) = (int16_t)(1 + dice_int(4));
    P(SKIPS) = (int16_t)dice_int(11);
    ST(hysteria_t)->x = 500 + dice_int(3000);
}

static void F(play_changed)(toy_t *t)
{
    if (t->playing)
        ST(hysteria_t)->x = 1000;
}

static void F(on_step)(toy_t *t)
{
    hysteria_t *s = ST(hysteria_t);
    int r, root, n, k;
    int8_t tones[12];
    if (t->steps % P(RATE) != 0)
        return;
    r = 180 + P(CHAOS) * 76 / 10;               /* r in 64ths: 2.8 .. 4.0 (256 = 4.0) */
    s->x = (int)(((int32_t)s->x * (4096 - s->x) >> 6) * r >> 12);   /* x in 0..4095 */
    if (s->x < 1)
        s->x = 1;
    if (s->x > 4095)
        s->x = 4095;
    s->xs[s->hp] = (uint16_t)s->x;
    s->hp = (s->hp + 1) % HY_HIST;
    if (s->x < 820 && dice_lt(P(SKIPS), 11))
        return;
    root = toy_root(t, 0) - 7;
    n = tune_tones(root, root, root + 13, tones);
    if (n > P(RANGE))
        n = P(RANGE);
    k = s->x * n / 4096;
    toy_note(t, tones[k], 0, 0);
}

#ifdef WB_PICTURES
/* the picture: the map's recent values as points round a ring, x as the angle, so a steady note is
 * one point, a flip two across from each other, four lines four, and chaos a scatter; the newest in
 * the hue; the orb at the centre fills on the way to chaos */
static void F(draw)(toy_t *t)
{
    hysteria_t *s = ST(hysteria_t);
    int i, cx = PIC_CX, cy = PIC_CY, last = (s->hp + HY_HIST - 1) % HY_HIST;
    pic_seal(cx, cy, 64, 4, 0, K_SAND);
    for (i = 0; i < HY_HIST; i++) {
        int k = (s->hp + i) % HY_HIST, a = s->xs[k] * 32 / 4096, age = HY_HIST - 1 - i;
        int x = pic_px(cx, 50, a), y = pic_py(cy, 50, a);
        if (k == last)
            cv_line(cx, cy, x, y, K_LINE);
        pic_disc(x, y, k == last ? 5 : age < 6 ? 3 : 2, k == last ? pic_hue : age < 6 ? K_TEXT : K_SOFT);
    }
    pic_orb(cx, cy, 18, P(CHAOS) * 100, K_SAND, pic_ago(t, -1) < 120u ? pic_hue : K_SOFT);
}
#endif

TOY(toy_hysteria, "hysteria", "Hysteria", "chaos", ROLE_BASS, HUE_RED, 0, 3, "The logistic map picks every note: one steady note, then two, then four, then chaos.", hysteria_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef CHAOS
#undef RANGE
#undef RATE
#undef SKIPS
