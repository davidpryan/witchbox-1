/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 75 (Witchbox, voice): Stutter Chop. One voice note fired again and again in a fast burst,
 * stepping through the chord; Swell fades each repeat in. */
#include "toy_util.h"
#undef F
#define F(n) ch_##n

#define CH_N 12
static const char *const CH_RATE_N[2] = {"16ths", "32nds"};
typedef struct { uint8_t order[CH_N]; int left, count, wait, lit, fade, full; int8_t tones[12]; int ntones, from; } chop_t;
#define RATE 0
#define BURST 1
#define MOVE 2
#define SWELL 3

static void F(make)(toy_t *t)
{
    chop_t *s = ST(chop_t);
    int i;
    add_pick(t, "Rate", 2, 1, CH_RATE_N, "How fast the repeats come");
    add(t, "Burst", 2, 16, 8, "How many repeats in a burst");
    add(t, "Step", 0, 4, 1, "How far the pitch moves each repeat");
    add(t, "Swell", 0, 10, 4, "How much each repeat fades in");
    for (i = 0; i < CH_N; i++)
        s->order[i] = (uint8_t)i;
    s->lit = -1;
    s->full = 1;
    s->tones[0] = 0;
    s->ntones = 1;
}

static void F(randomise)(toy_t *t)
{
    P(RATE) = (int16_t)dice_int(2);
    P(BURST) = (int16_t)(2 + dice_int(15));
    P(MOVE) = (int16_t)dice_int(5);
    P(SWELL) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    chop_t *s = ST(chop_t);
    if (!t->playing) {
        s->left = 0;
        s->lit = -1;
    }
}

static void F(on_step)(toy_t *t)
{
    chop_t *s = ST(chop_t);
    int k;
    if (s->left || t->steps % 8)
        return;
    s->lit = -1;
    for (k = 0; k < 2; k++) {       /* between bursts: swap a few slices around */
        int a = dice_int(CH_N), b = dice_int(CH_N);
        if (a != b) {
            uint8_t o = s->order[a];
            s->order[a] = s->order[b];
            s->order[b] = o;
        }
    }
    s->ntones = tune_tones(toy_root(t, 0), 0, 14, s->tones);
    s->from = dice_int(CH_N);
    s->left = P(BURST);
    s->count = 0;
    s->wait = 0;
}

static void F(frame)(toy_t *t, int ms)
{
    chop_t *s = ST(chop_t);
    int gap;
    if (!t->playing)
        return;
    if (s->fade) {                  /* the repeat reaches full strength */
        s->fade = imax(0, s->fade - ms);
        if (!s->fade)
            s->full = 1;
    }
    if (!s->left)
        return;
    gap = t->step_ms / (P(RATE) ? 2 : 1);   /* 32nds are half a step */
    s->wait -= ms;
    while (s->left && s->wait <= 0) {
        int k = s->count;
        toy_note(t, s->tones[(k * P(MOVE)) % s->ntones], 0, 0);
        s->fade = gap * P(SWELL) / 12;
        s->full = !s->fade;
        s->lit = (s->from + k) % CH_N;
        s->count++;
        s->left--;
        s->wait += gap;
    }
}

#ifdef WB_PICTURES
/* the picture: twelve slices of a waveform, shuffled a few at a time; the slice being fired jumps up,
 * in colour, paler until its repeat reaches full strength */
static void F(draw)(toy_t *t)
{
    chop_t *s = ST(chop_t);
    static const uint8_t WAVE[12] = {22, 46, 72, 93, 78, 52, 33, 61, 87, 67, 39, 20};
    int i;
    for (i = 0; i < CH_N; i++) {
        int x = 12 + i * 18, h = WAVE[s->order[i]];
        if (i == s->lit)
            cv_rect(x, 78 - h / 2 - 10, 12, h, s->full ? pic_hue : K_SOFT);
        else
            cv_rect(x, 78 - h / 2, 12, h, K_TEXT);
    }
}
#endif

TOY(toy_chop, "chop", "Stutter Chop", "slices", ROLE_VOICE, HUE_ORANGE, 20, 1, "One voice note fired again and again in a fast burst, stepping through the chord.", chop_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef RATE
#undef BURST
#undef MOVE
#undef SWELL
