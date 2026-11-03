/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 77 (Witchbox, voice): Breath Stack. Up to four breathy voices coming in one after another
 * on notes above the chord, holding, and leaving in the same order. */
#include "toy_util.h"
#undef F
#define F(n) br_##n

static const uint8_t BR_APART[4] = {4, 8, 16, 32};
static const uint8_t BR_NOTES[4] = {7, 9, 11, 14};
static const char *const BR_STAGGER_N[4] = {"1 beat", "2 beats", "1 bar", "2 bars"};
typedef struct { int tt; uint8_t on[4]; int lvl[4], fade, rip, phase; } breath_t;
#define VOICES 0
#define STAGGER 1
#define AIR 2
#define HOLD 3

static void F(make)(toy_t *t)
{
    add(t, "Voices", 1, 4, 3, "How many voices join in");
    add_pick(t, "Stagger", 4, 1, BR_STAGGER_N, "How far apart the voices come in");
    add(t, "Air", 0, 10, 3, "From a clear tone to mostly breath");
    add(t, "Hold", 1, 8, 2, "How many bars the full stack holds");
}

static void F(randomise)(toy_t *t)
{
    P(VOICES) = (int16_t)(1 + dice_int(4));
    P(STAGGER) = (int16_t)dice_int(4);
    P(AIR) = (int16_t)dice_int(11);
    P(HOLD) = (int16_t)(1 + dice_int(8));
}

static void F(changed)(toy_t *t)
{
    breath_t *s = ST(breath_t);
    int v;
    for (v = P(VOICES); v < 4; v++)     /* voices switched off fall silent */
        s->on[v] = 0;
}

static void F(play_changed)(toy_t *t)
{
    breath_t *s = ST(breath_t);
    int v;
    s->tt = 0;
    if (!t->playing)
        for (v = 0; v < 4; v++)
            s->on[v] = 0;
}

static void F(on_step)(toy_t *t)
{
    breath_t *s = ST(breath_t);
    int n = P(VOICES), gap = BR_APART[P(STAGGER)], full = (n - 1) * gap + P(HOLD) * BAR, tt = s->tt, v;
    for (v = 0; v < n; v++) {
        if (tt == v * gap) {        /* voice v comes in */
            s->on[v] = 1;
            toy_note(t, toy_root(t, 0) + BR_NOTES[v], v, 0);
        } else if (tt == full + v * gap) {
            s->on[v] = 0;           /* and leaves, in the same order */
        } else if (s->on[v] && t->steps % BAR == 0) {
            toy_note(t, toy_root(t, 0) + BR_NOTES[v], v, 0);   /* held voices move with the chord */
        }
    }
    s->tt = tt + 1;
    if (s->tt >= full + n * gap + 4)    /* a beat's rest, then again */
        s->tt = 0;
}

static void F(frame)(toy_t *t, int ms)
{
    breath_t *s = ST(breath_t);
    int ripple, fade, v;
    s->fade -= ms;
    s->rip -= ms;
    ripple = s->rip <= 0 && t->playing;
    if (ripple) {
        s->rip = 200;
        s->phase = (s->phase + 1) % 8;
    }
    fade = s->fade <= 0;
    if (fade)
        s->fade = 160;
    for (v = 0; v < P(VOICES); v++) {
        int want = s->on[v] ? 3 : 0;
        if (fade && s->lvl[v] != want)
            s->lvl[v] += want ? 1 : -1;
    }
}

TOY(toy_breath, "breath", "Breath Stack", "ribbons", ROLE_VOICE, HUE_CYAN, 40, 1,
    "Breathy voices that come in one after another above the chord, hold, and leave.",
    breath_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef VOICES
#undef STAGGER
#undef AIR
#undef HOLD
