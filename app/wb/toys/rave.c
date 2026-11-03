/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 66 (Witchbox, chords): Rave Stab. Short chord stabs in a jumpy pattern, one fixed shape
 * moved whole to the chord's root and a little up or down off the beat. */
#include "toy_util.h"
#undef F
#define F(n) rv_##n

static const int8_t RV_SHAPES[3][4] = {{0, 2, 4, NONE}, {0, 2, 4, 6}, {0, 3, 4, NONE}};
static const uint8_t RV_ON[8] = {0, 8, 4, 12, 2, 10, 6, 14};
static const uint8_t RV_OFF[8] = {3, 11, 7, 15, 9, 1, 13, 5};
static const int8_t RV_LIFT[BAR] = {0, 0, 0, 2, 0, 0, 2, 4, 0, 0, 0, 2, 0, 4, 2, -1};
static const char *const RV_SHAPE_N[3] = {"Minor", "Minor 7", "Sus"};
typedef struct { int root; uint8_t stab[BAR]; int hot, ms, tick; } rave_t;
#define HITS 0
#define PUSH 1
#define SHAPE 2
#define HOLD 3

static void F(lay)(toy_t *t)
{
    rave_t *s = ST(rave_t);
    int off = imin(P(PUSH), P(HITS)), i;
    for (i = 0; i < BAR; i++)
        s->stab[i] = 0;
    for (i = 0; i < P(HITS) - off; i++)
        s->stab[RV_ON[i]] = 1;
    for (i = 0; i < off; i++)
        s->stab[RV_OFF[i]] = 1;
}

static void F(make)(toy_t *t)
{
    rave_t *s = ST(rave_t);
    add(t, "Hits", 1, 8, 5, "How many stabs are in a bar");
    add(t, "Push", 0, 8, 2, "How many stabs land off the beat");
    add_pick(t, "Shape", 3, 0, RV_SHAPE_N, "The chord's shape, which never changes");
    add(t, "Length", 1, 10, 4, "From a clipped stab to a held one");
    s->hot = s->tick = -1;
    F(lay)(t);
}

static void F(randomise)(toy_t *t)
{
    P(HITS) = (int16_t)(2 + dice_int(7));
    P(PUSH) = (int16_t)dice_int(5);
    P(SHAPE) = (int16_t)dice_int(3);
    P(HOLD) = (int16_t)(1 + dice_int(10));
}

static void F(changed)(toy_t *t) { F(lay)(t); }

static void F(on_step)(toy_t *t)
{
    rave_t *s = ST(rave_t);
    int st = t->steps % BAR, r = toy_root(t, 0);
    s->root = r;
    s->tick = st;
    if (s->stab[st]) {
        int v;
        s->hot = st;
        s->ms = t->step_ms * (1 + P(HOLD)) / 3;
        for (v = 0; v < 4 && RV_SHAPES[P(SHAPE)][v] != NONE; v++)
            toy_note(t, r + RV_LIFT[st] + RV_SHAPES[P(SHAPE)][v], v, 0);
    }
}

static void F(frame)(toy_t *t, int ms)
{
    rave_t *s = ST(rave_t);
    if (s->hot >= 0) {
        s->ms -= ms;
        if (s->ms <= 0)
            s->hot = -1;
    }
}

static void F(play_changed)(toy_t *t)
{
    rave_t *s = ST(rave_t);
    if (!t->playing)
        s->hot = s->tick = -1;
}

#ifdef WB_PICTURES
/* the picture: the bar as a seal of sixteen points, a stab a big point, pushed further out the more
 * it is lifted and bigger the longer it is held, in the hue while it sounds; at the centre the
 * chord's shape as a small seal of its notes, joined, in the hue with the stab; a needle for the step */
static void F(draw)(toy_t *t)
{
    rave_t *s = ST(rave_t);
    int st, k, cx = PIC_CX, cy = PIC_CY, n = 0, size = 3 + P(HOLD) / 4;
    pic_seal(cx, cy, 64, BAR, 0, K_SAND);
    if (s->tick >= 0)
        pic_needle(cx, cy, 44, s->tick, BAR, K_LINE);
    for (st = 0; st < BAR; st++) {
        if (s->stab[st])
            pic_pt(cx, cy, 48 + RV_LIFT[st] * 2, st, BAR, size, st == s->hot ? pic_hue : K_TEXT);
        else
            pic_pt(cx, cy, 48, st, BAR, 0, K_SAND);
    }
    pic_circle(cx, cy, 24, K_SAND);
    for (k = 0; k < 4 && RV_SHAPES[P(SHAPE)][k] != NONE; k++)
        n++;
    for (k = 0; k < n; k++) {
        int d = RV_SHAPES[P(SHAPE)][k], e = RV_SHAPES[P(SHAPE)][(k + 1) % n];
        uint16_t c = s->hot >= 0 ? pic_hue : K_SOFT;
        pic_chord(cx, cy, 18, d, e, 7, c);
        pic_pt(cx, cy, 18, d, 7, 2, c);
    }
}
#endif

TOY(toy_rave, "rave", "Rave Stab", "stab", ROLE_CHORDS, HUE_RED, 40, 1, "Short chord stabs in a jumpy pattern, like an old rave sample played on a keyboard.", rave_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef HITS
#undef PUSH
#undef SHAPE
#undef HOLD
