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
/* the picture: the loop as a row of threads, one a step, each at its note's height, rests as gaps;
 * the thread being read lit, and the ones just rewoven in the colour too */
static void F(draw)(toy_t *t)
{
    loom_t *s = ST(loom_t);
    static const uint8_t LEN[3] = {8, 16, 32};
    int len = LEN[P(LENGTH)], i, w = 224 / len;
    for (i = 0; i < len; i++) {
        int v = s->reg[i], x = 8 + i * w;
        if (v == 99)
            cv_rect(x, 120, w - 1, 2, K_SAND);
        else
            cv_rect(x, 120 - v * 7, w - 1, v * 7 + 2, i == (s->at + len - 1) % len && t->playing ? pic_hue : K_TEXT);
    }
    cv_rect(8 + ((s->at + len - 1) % len) * w, 6, w - 1, 3, pic_hue);
}
#endif

TOY(toy_loom, "loom", "Loom", "loom", ROLE_LEAD, HUE_ORANGE, 0, 3, "A loop of notes on a shift register. Locked it repeats for ever; with Change, a note is rewoven each time round.", loom_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef CHANGE
#undef LENGTH
#undef RANGE
#undef RESTS
