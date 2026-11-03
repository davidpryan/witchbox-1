/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 65 (Witchbox, texture): Riser. Build-up and release: before a phrase ends a sweep of noise
 * or a climbing note rises into the bar line, and an impact lands on it. */
#include "toy_util.h"
#undef F
#define F(n) ri_##n

static const uint8_t RI_EVERY[3] = {4, 8, 16};
static const uint8_t RI_STEPS[5] = {4, 8, 16, 32, 64};
static const char *const RI_EVERY_N[3] = {"4 bars", "8 bars", "16 bars"};
static const char *const RI_SPAN_N[5] = {"1 beat", "2 beats", "1 bar", "2 bars", "4 bars"};
static const char *const RI_KIND_N[3] = {"Noise", "Pitch", "Both"};
#define RI_N 60
typedef struct { int full, ms, drain, burst, last; } riser_t;
#define EVERY 0
#define SPAN 1
#define KIND 2
#define IMPACT 3
#define RI_NONE (-999)

static void F(reset)(riser_t *s)
{
    s->ms = -1;
    s->drain = s->burst = 0;
    s->full = 0;
}

static void F(make)(toy_t *t)
{
    riser_t *s = ST(riser_t);
    add_pick(t, "Every", 3, 0, RI_EVERY_N, "How long a phrase is");
    add_pick(t, "Length", 5, 2, RI_SPAN_N, "How long the riser lasts");
    add_pick(t, "Type", 3, 2, RI_KIND_N, "What rises: noise, a note or both");
    add(t, "Impact", 0, 10, 7, "How hard it lands; 10 adds a falling sweep");
    s->ms = -1;
    s->last = RI_NONE;
}

static void F(randomise)(toy_t *t)
{
    P(EVERY) = (int16_t)dice_int(3);
    P(SPAN) = (int16_t)dice_int(5);
    P(KIND) = (int16_t)dice_int(3);
    P(IMPACT) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t) { F(reset)(ST(riser_t)); }
static void F(play_changed)(toy_t *t) { F(reset)(ST(riser_t)); }

static void F(on_step)(toy_t *t)
{
    riser_t *s = ST(riser_t);
    int every = RI_EVERY[P(EVERY)], phrase = every * BAR;
    int at = toy_bar(t) % every * BAR + toy_now(t) % BAR;
    int length = RI_STEPS[P(SPAN)];
    if (at == 0 && s->ms >= 0) {    /* the phrase ends: impact */
        int hit = P(IMPACT);
        s->ms = -1;
        if (hit) {
            toy_note(t, toy_root(t, 0), 2, 0);
            s->burst = 1;
        }
        if (hit == 10)
            toy_note(t, toy_root(t, 0) + 14, 3, 0);    /* the falling sweep */
        s->drain = hit == 10 ? 3 : 12;
    }
    if (at == phrase - length) {    /* time to start climbing */
        s->ms = 0;
        s->drain = 0;
        s->last = RI_NONE;
        if (P(KIND) != 1)
            toy_note(t, 0, 0, length * t->step_ms);     /* the noise sweep, for the whole climb */
    }
    if (s->ms >= 0 && P(KIND)) {    /* the note climbs two octaves */
        int rise = toy_root(t, 1) + floordiv(14 * (length - fmod_(phrase - at, phrase)), length);
        if (rise != s->last) {
            s->last = rise;
            toy_note(t, rise, 1, 0);
        }
    }
}

static void F(frame)(toy_t *t, int ms)
{
    riser_t *s = ST(riser_t);
    if (!t->playing)
        return;
    if (s->ms >= 0) {
        int upto;
        s->ms += ms;
        upto = imin(RI_N, RI_N * s->ms / (RI_STEPS[P(SPAN)] * t->step_ms));
        if (s->full < upto)
            s->full = upto;
    } else if (s->drain) {
        s->full = imax(0, s->full - s->drain);
        if (!s->full)
            s->drain = 0;
    }
    if (s->burst) {
        s->burst += ms;
        if (s->burst / (60 + P(IMPACT) * 8) >= 4)
            s->burst = 0;
    }
}

TOY(toy_riser, "riser", "Riser", "ramp", ROLE_TEXTURE, HUE_ORANGE, 40, 1,
    "Build-up and release: a sweep rises into the end of each phrase and an impact lands.",
    riser_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef EVERY
#undef SPAN
#undef KIND
#undef IMPACT
