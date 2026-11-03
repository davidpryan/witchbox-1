/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 63 (Witchbox, drums): Broken Clock. A simple beat underneath; now and then a slice gets
 * stuck and stutters, and now and then a step is cut to silence. */
#include "toy_util.h"
#undef F
#define F(n) ck_##n

static const uint16_t CK_SLICE[3] = {2000, 1000, 500};
static const char *const CK_SLICE_N[3] = {"8th", "16th", "32nd"};
typedef struct { uint8_t gone[BAR]; int togo, wait, voice, from; } clock_t_;
#define STUTTER 0
#define SLICE 1
#define REPS 2
#define HOLES 3

static void F(make)(toy_t *t)
{
    add(t, "Stutter", 0, 10, 4, "How often a slice gets stuck");
    add_pick(t, "Slice", 3, 1, CK_SLICE_N, "How long the stuck slice is");
    add(t, "Repeats", 2, 8, 4, "How many times the slice plays");
    add(t, "Holes", 0, 10, 3, "How often a step is cut to silence");
    ST(clock_t_)->voice = 2;
}

static void F(randomise)(toy_t *t)
{
    P(STUTTER) = (int16_t)dice_int(11);
    P(SLICE) = (int16_t)dice_int(3);
    P(REPS) = (int16_t)(2 + dice_int(7));
    P(HOLES) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    clock_t_ *s = ST(clock_t_);
    int i;
    s->togo = 0;
    for (i = 0; i < BAR; i++)
        s->gone[i] = 0;
}

static void F(sound)(toy_t *t, int voice)
{
    if (voice)
        toy_note(t, 0, voice, 0);
    else
        toy_kick(t);
}

static void F(on_step)(toy_t *t)
{
    clock_t_ *s = ST(clock_t_);
    int i = t->steps % BAR;
    if (s->togo)                    /* stuck: the beat underneath is not heard */
        return;
    if (s->gone[i])                 /* the hand is back: mend last bar's hole */
        s->gone[i] = 0;
    if (i && dice_lt(P(HOLES), 40)) {
        s->gone[i] = 1;             /* a hole: this step is silent */
        return;
    }
    if (i % 2)
        return;
    s->voice = i == 0 ? 0 : i % 8 == 4 ? 1 : 2;     /* kick, snap, click */
    F(sound)(t, s->voice);
    if (dice_lt(P(STUTTER), 30)) {
        s->from = i;                /* this slice sticks; frame() plays the repeats */
        s->togo = P(REPS) - 1;
        s->wait = CK_SLICE[P(SLICE)] * t->step_ms / 1000;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    clock_t_ *s = ST(clock_t_);
    if (!t->playing || !s->togo)
        return;
    s->wait -= ms;
    if (s->wait <= 0) {
        s->togo--;
        s->wait += CK_SLICE[P(SLICE)] * t->step_ms / 1000;
        F(sound)(t, s->voice ? s->voice : 2);       /* a stuck kick repeats as a click */
    }
}

#ifdef WB_PICTURES
/* the picture: the clock as a seal: a rune at every step that can sound (kick, snare, hat), a sand
 * dot at the rest between, a hollow ring where a step is cut to silence; the hand a needle that
 * sweeps the bar and judders in the hue while a slice is stuck */
static void F(draw)(toy_t *t)
{
    clock_t_ *s = ST(clock_t_);
    int i, cx = PIC_CX, cy = PIC_CY, at = t->steps % BAR;
    int half = s->togo ? 2 * s->from + s->togo % 2 : 2 * at;
    pic_seal(cx, cy, 64, BAR, 0, K_SAND);
    for (i = 0; i < BAR; i++) {
        int k = i == 0 ? RUNE_KICK : i % 8 == 4 ? RUNE_SNARE : RUNE_HAT, a = pic_ang(i, BAR);
        uint16_t c = (s->togo ? i == s->from : i == at) ? pic_hue : K_TEXT;
        if (i % 2)
            pic_pt(cx, cy, 50, i, BAR, 0, K_SAND);
        else if (s->gone[i])
            pic_circle(pic_px(cx, 50, a), pic_py(cy, 50, a), 4, K_SAND);
        else
            pic_rune_at(cx, cy, 50, i, BAR, k, c);
    }
    {
        int x = pic_px(cx, 34, half), y = pic_py(cy, 34, half), k;
        for (k = -1; k <= 1; k++)
            cv_line(cx + k, cy, x + k, y, s->togo ? pic_hue : K_TEXT);
    }
    pic_disc(cx, cy, 4, K_TEXT);
}
#endif

TOY(toy_clock, "clock", "Broken Clock", "clock", ROLE_DRUMS, HUE_CYAN, 20, 1, "A simple beat that gets stuck and stutters, with steps cut out to silence.", clock_t_, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef STUTTER
#undef SLICE
#undef REPS
#undef HOLES
