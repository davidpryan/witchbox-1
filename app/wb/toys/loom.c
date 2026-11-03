/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 113 (Witchbox IV, lead): Loom. A loop of notes on a shift register, the classic Turing
 * machine sequencer: locked it repeats for ever; with Change up, a note is rewoven each time it
 * comes round, so the tune wanders without ever quite leaving. */
#include "toy_util.h"
#undef F
#define F(n) lm_##n

static const uint8_t LM_LEN[3] = {8, 16, 32};
static const char *const LM_LEN_N[3] = {"8 steps", "16 steps", "32 steps"};
typedef struct { int8_t reg[32]; int at; } loom_t;
#define CHANGE 0
#define LENGTH 1
#define RANGE 2
#define RESTS 3

static void F(weave)(toy_t *t, int i)
{
    loom_t *s = ST(loom_t);
    s->reg[i] = (int8_t)(dice_lt(P(RESTS), 11) ? 99 : dice_int(P(RANGE) + 1));
}

static void F(deal)(toy_t *t)
{
    int i;
    for (i = 0; i < 32; i++)
        F(weave)(t, i);
    ST(loom_t)->at = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Change", 0, 10, 2, "How often a note is rewoven as it comes round");
    add_pick(t, "Length", 3, 1, LM_LEN_N, "How long the loop is");
    add(t, "Range", 2, 14, 7, "How many scale steps the notes span");
    add(t, "Rests", 0, 10, 3, "How many of the steps are rests");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(CHANGE) = (int16_t)dice_int(11);
    P(LENGTH) = (int16_t)dice_int(3);
    P(RANGE) = (int16_t)(2 + dice_int(13));
    P(RESTS) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    loom_t *s = ST(loom_t);
    int len = LM_LEN[P(LENGTH)], v;
    if (s->at >= len)
        s->at = 0;
    if (dice_lt(P(CHANGE), 40))
        F(weave)(t, s->at);
    v = s->reg[s->at++];
    if (v != 99)
        toy_note(t, toy_root(t, 0) + 7 + v, 0, 0);
}

#ifdef WB_PICTURES
/* the picture: the loop as a wheel, a point a step, each pushed out from the ring by its note so the
 * thread of chords between them is the tune's shape; rests are grains on the ring; the read head a
 * needle, the thread being read in the hue */
static void F(draw)(toy_t *t)
{
    loom_t *s = ST(loom_t);
    int len = LM_LEN[P(LENGTH)], i, cx = PIC_CX, cy = PIC_CY, head = (s->at + len - 1) % len, prev = -1, pr = 0;
    pic_circle(cx, cy, 64, K_SAND);
    pic_circle(cx, cy, 34, K_SAND);
    for (i = 0; i <= len; i++) {
        int k = i % len, v = s->reg[k], r = v == 99 ? 0 : 36 + v * 2;
        if (v == 99)
            continue;
        if (prev >= 0) {
            int a = pic_ang(prev, len), b = pic_ang(k, len);
            cv_line(pic_px(cx, pr, a), pic_py(cy, pr, a), pic_px(cx, r, b), pic_py(cy, r, b), K_LINE);
        }
        prev = k;
        pr = r;
    }
    pic_needle(cx, cy, 32, head, len, t->playing ? pic_hue : K_LINE);
    for (i = 0; i < len; i++) {
        int v = s->reg[i], lit = i == head && t->playing;
        if (v == 99)
            pic_pt(cx, cy, 34, i, len, 0, K_SAND);
        else
            pic_pt(cx, cy, 36 + v * 2, i, len, lit ? 5 : len > 16 ? 2 : 3, lit ? pic_hue : K_TEXT);
    }
    pic_disc(cx, cy, 3, K_TEXT);
}
#endif

TOY(toy_loom, "loom", "Loom", "loom", ROLE_LEAD, HUE_ORANGE, 0, 3, "A loop of notes on a shift register. Locked it repeats for ever; with Change, a note is rewoven each time round.", loom_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef CHANGE
#undef LENGTH
#undef RANGE
#undef RESTS
