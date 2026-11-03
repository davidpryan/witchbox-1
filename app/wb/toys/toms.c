/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 64 (Witchbox, drums): Fog Toms. Slow booming toms tuned to the chord, in a sparse two-bar
 * pattern that wanders a little each time round. */
#include "toy_util.h"
#undef F
#define F(n) tm_##n

static const int8_t TM_UP[3] = {0, 3, 4};
typedef struct { uint8_t step[8], tom[8]; int ms[3]; } toms_t;
#define HITS 0
#define TAIL 1
#define DEPTH 2
#define WANDER 3

static void F(deal)(toms_t *s, int k)   /* one hit: on a half beat, mostly on a whole beat */
{
    int whole = dice_lt_q(39322u /* rand() < 0.6 */);
    s->step[k] = (uint8_t)(whole ? dice_int(8) * 4 : dice_int(16) * 2);
    s->tom[k] = (uint8_t)dice_int(3);
}

static void F(make)(toy_t *t)
{
    toms_t *s = ST(toms_t);
    int k;
    add(t, "Hits", 1, 8, 4, "How many hits in the two bars");
    add(t, "Tail", 0, 10, 6, "How long each tom rings on");
    add(t, "Depth", 0, 10, 5, "How low the toms are tuned");
    add(t, "Wander", 0, 10, 3, "How much the pattern changes each time");
    s->step[0] = s->tom[0] = 0;
    for (k = 1; k < 8; k++)
        F(deal)(s, k);
}

static void F(randomise)(toy_t *t)
{
    toms_t *s = ST(toms_t);
    int k;
    P(HITS) = (int16_t)(1 + dice_int(8));
    P(TAIL) = (int16_t)dice_int(11);
    P(DEPTH) = (int16_t)dice_int(11);
    P(WANDER) = (int16_t)dice_int(11);
    for (k = 1; k < 8; k++)
        F(deal)(s, k);
}

static int F(tail_ms)(toy_t *t) { return 300 + P(TAIL) * 200; }

static void F(changed)(toy_t *t)
{
    toms_t *s = ST(toms_t);
    int k;
    for (k = 0; k < 3; k++)         /* a shorter tail cuts short what is ringing */
        s->ms[k] = imin(s->ms[k], F(tail_ms)(t));
}

static void F(on_step)(toy_t *t)
{
    toms_t *s = ST(toms_t);
    int i = t->steps % (2 * BAR), k, low, struck = 0;
    if (i == 0 && t->steps)         /* round again: some hits wander off */
        for (k = 1; k < 8; k++)
            if (dice_lt(P(WANDER), 20))
                F(deal)(s, k);
    low = toy_root(t, 0) - 7 * (P(DEPTH) / 4);     /* Depth drops whole octaves */
    for (k = 0; k < P(HITS); k++) {
        int tm = s->tom[k];
        if (s->step[k] == i && !(struck & (1 << tm))) {
            struck |= 1 << tm;      /* two hits may share a step and a tom */
            toy_note(t, low + TM_UP[tm], 3 + tm, 0);
            s->ms[tm] = F(tail_ms)(t);
        }
    }
}

static void F(frame)(toy_t *t, int ms)
{
    toms_t *s = ST(toms_t);
    int k;
    for (k = 0; k < 3; k++)
        if (s->ms[k])
            s->ms[k] = imax(0, s->ms[k] - ms);
}

#ifdef WB_PICTURES
/* the picture: three toms, the root's the biggest, each swelling and going purple when struck, with a
 * ring of fog that rises off it and fades as the tail dies */
static void F(draw)(toy_t *t)
{
    toms_t *s = ST(toms_t);
    static const uint8_t X[3] = {187, 120, 52}, R[3] = {15, 13, 11};
    int k, tail = F(tail_ms)(t);
    for (k = 0; k < 3; k++) {
        int left = s->ms[k], r = R[k] + (left ? 3 * left / tail + 1 : 0);
        if (left) {
            int y = 4 + (86 - 4) * left / tail, x = X[k] - R[k], w = 2 * R[k];
            uint16_t c = left * 2 > tail ? K_SOFT : K_SAND;
            cv_rect(x + 2, y, w - 4, 2, c); cv_rect(x + 2, y + 5, w - 4, 2, c);
            cv_rect(x, y + 2, 2, 3, c); cv_rect(x + w - 2, y + 2, 2, 3, c);
        }
        pic_disc(X[k], 110, r, left ? pic_hue : K_TEXT);
    }
}
#endif

TOY(toy_toms, "toms", "Fog Toms", "toms", ROLE_DRUMS, HUE_PURPLE, 40, 1, "Slow, booming toms tuned to the chord, in a sparse pattern that wanders.", toms_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, F(frame), PIC(F(draw)), 0);
#undef HITS
#undef TAIL
#undef DEPTH
#undef WANDER
