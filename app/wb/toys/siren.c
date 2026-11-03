/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 72 (Witchbox, lead): Siren. One long note that glides between notes of the chord, resting
 * on each, with a slow wobble. */
#include "toy_util.h"
#undef F
#define F(n) si_##n

#define SI_LO 3
#define SI_HI 17
typedef struct { int a, b, tt, moving; } siren_t;
#define GLIDE 0
#define VIB 1
#define REACH 2
#define HOLD 3

static void F(aim)(toy_t *t, int live)    /* the next note: a chord note within reach, else the nearest */
{
    siren_t *s = ST(siren_t);
    int r = toy_root(t, 0), here = s->b, far = P(REACH), to, n, k;
    int8_t tones[12], near[12];
    n = tune_tones(r, imax(SI_LO, here - far), imin(SI_HI, here + far), tones);
    for (k = 0, to = 0; k < n; k++)
        if (tones[k] != here)
            near[to++] = tones[k];
    if (to)
        to = near[dice_int(to)];
    else
        to = imax(SI_LO, imin(SI_HI, tune_nearest(here, r)));
    s->tt = 0;
    if (to != here) {
        s->a = here;
        s->b = to;
        s->moving = 1;
        if (live)
            toy_note(t, to, NONE, 0);
    }
}

static void F(pen)(toy_t *t, int ms, int live)   /* move the pitch on by ms */
{
    siren_t *s = ST(siren_t);
    s->tt += ms;
    if (s->moving) {
        if (s->tt >= P(GLIDE) * 2 * t->step_ms) {
            s->moving = 0;
            s->tt = 0;
        }
    } else if (s->tt >= P(HOLD) * 2 * t->step_ms) {
        F(aim)(t, live);
    }
}

static void F(make)(toy_t *t)
{
    siren_t *s = ST(siren_t);
    int k;
    add(t, "Glide", 1, 10, 5, "How slow each glide is");
    add(t, "Vibrato", 0, 10, 4, "From none to deep");
    add(t, "Reach", 1, 7, 4, "How far each move goes, in notes");
    add(t, "Hold", 1, 10, 4, "How long it rests on each note");
    s->a = s->b = 10;
    for (k = 0; k < 160 - 5; k++)   /* a line to show before anything has played */
        F(pen)(t, 30, 0);
}

static void F(randomise)(toy_t *t)
{
    P(GLIDE) = (int16_t)(1 + dice_int(10));
    P(VIB) = (int16_t)dice_int(11);
    P(REACH) = (int16_t)(1 + dice_int(7));
    P(HOLD) = (int16_t)(1 + dice_int(10));
}

static void F(frame)(toy_t *t, int ms)
{
    if (t->playing)
        F(pen)(t, ms, 1);
}

static void F(play_changed)(toy_t *t)
{
    siren_t *s = ST(siren_t);
    if (t->playing && !s->moving)
        toy_note(t, s->b, NONE, 0);     /* the note it is resting on */
}

TOY(toy_siren, "siren", "Siren", "siren", ROLE_LEAD, HUE_PURPLE, 30, 1,
    "One long note that glides between notes of the chord with a slow wobble.",
    siren_t, F(make), 0, 0, F(randomise), 0, F(play_changed), F(frame), 0, 0);
#undef GLIDE
#undef VIB
#undef REACH
#undef HOLD
