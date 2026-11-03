/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 52 (Witchbox, chords): Coven Saws. Thick detuned chords (root, fifth, root above) held for
 * whole bars while the tone opens; now and then the chord slips up a semitone (slipped = the
 * engine should pitch it a semitone above the root it was given). */
#include "toy_util.h"
#undef F
#define F(n) cv_##n

static const uint8_t CV_HOLD[3] = {1, 2, 4};
static const char *const CV_HOLD_N[3] = {"1 bar", "2 bars", "4 bars"};
typedef struct { int slipped, opened, age, jolt; } coven_t;
#define DETUNE 0
#define OPEN 1
#define SLIP 2
#define HOLD 3

static void F(make)(toy_t *t)
{
    add(t, "Detune", 0, 10, 4, "How far apart the voices are tuned");
    add(t, "Open", 0, 10, 7, "How far the tone opens over each chord");
    add(t, "Slip", 0, 10, 2, "How often a chord slips up a semitone");
    add_pick(t, "Hold", 3, 0, CV_HOLD_N, "How long each chord is held");
}

static void F(randomise)(toy_t *t)
{
    P(DETUNE) = (int16_t)dice_int(11);
    P(OPEN) = (int16_t)dice_int(11);
    P(SLIP) = (int16_t)dice_int(6);
    P(HOLD) = (int16_t)dice_int(3);
}

static void F(play_changed)(toy_t *t)
{
    coven_t *s = ST(coven_t);
    s->age = s->jolt = s->opened = 0;
    s->slipped = 0;
}

static void F(on_step)(toy_t *t)
{
    coven_t *s = ST(coven_t);
    int r, v;
    if (t->steps % BAR || toy_bar(t) % CV_HOLD[P(HOLD)])
        return;
    r = toy_root(t, 0);
    s->slipped = dice_lt(P(SLIP), 20);
    if (s->slipped) {
        r += 1;                     /* stands for a semitone up; see the top */
        s->jolt = 320;
    }
    for (v = 0; v < 3; v++)         /* root, fifth, root */
        toy_note(t, v == 0 ? r - 7 : v == 1 ? r - 3 : r, v, 0);
    s->age = 0;
}

static void F(frame)(toy_t *t, int ms)
{
    coven_t *s = ST(coven_t);
    int whole;
    if (!t->playing)
        return;
    s->age += ms;
    s->jolt = imax(0, s->jolt - ms);
    whole = CV_HOLD[P(HOLD)] * BAR * t->step_ms;
    s->opened = P(OPEN) * 10 * imin(s->age, whole) / whole;
}

#ifdef WB_PICTURES
/* the picture: three triangles stacked, further apart with Detune, shading towards the colour as the
 * tone opens, thrown sideways for a moment when the chord slips */
static void F(draw)(toy_t *t)
{
    coven_t *s = ST(coven_t);
    int gap = 28 + P(DETUNE) * 3 / 2, dx = s->jolt ? 20 : 0, k, i, shade = s->opened * 6 / 100;
    uint16_t c = shade >= 4 ? pic_hue : shade >= 2 ? K_SOFT : K_TEXT;
    for (k = -1; k <= 1; k++) {
        int top = 72 + k * gap - 12;
        for (i = 0; i < 8; i++) {
            int half = 3 + i * 3;
            cv_rect(120 + dx - half, top + i * 3, 2 * half, 3, c);
        }
    }
}
#endif

TOY(toy_coven, "coven", "Coven Saws", "triangle", ROLE_CHORDS, HUE_PURPLE, 40, 0, "Thick detuned chords held for whole bars. Sometimes they slip up a step.", coven_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef DETUNE
#undef OPEN
#undef SLIP
#undef HOLD
