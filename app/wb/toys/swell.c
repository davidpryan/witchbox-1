/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 67 (Witchbox, chords): Swell. Chords that fade in like a sound played backwards and cut dead
 * on the downbeat, each swelling the chord that is about to arrive; Tail adds short echoes. */
#include "toy_util.h"
#undef F
#define F(n) sw_##n

static const uint8_t SW_LEN[3] = {4, 8, 16};
static const uint8_t SW_EVERY[3] = {1, 2, 4};
static const char *const SW_SIZE_N[3] = {"1/4 bar", "1/2 bar", "1 bar"};
static const char *const SW_EVERY_N[3] = {"1 bar", "2 bars", "4 bars"};
typedef struct { int all, tt, chord, echo, wait, blip; } swell_t;
#define SIZE 0
#define EVERY 1
#define CURVE 2
#define TAIL 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Length", 3, 1, SW_SIZE_N, "How long each swell takes");
    add_pick(t, "Every", 3, 0, SW_EVERY_N, "How often a swell comes");
    add(t, "Curve", 0, 10, 4, "From a steady rise to a late rush");
    add(t, "Tail", 0, 10, 3, "From a dead cut to a short echo");
}

static void F(randomise)(toy_t *t)
{
    P(SIZE) = (int16_t)dice_int(3);
    P(EVERY) = (int16_t)dice_int(3);
    P(CURVE) = (int16_t)dice_int(11);
    P(TAIL) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    swell_t *s = ST(swell_t);
    int period = SW_EVERY[P(EVERY)] * BAR;
    int at = (toy_bar(t) * BAR + t->steps % BAR) % period;
    if (at == 0 && s->all) {        /* the downbeat: cut dead */
        s->all = 0;
        s->blip = 0;
        s->echo = (P(TAIL) + 2) / 4;
        s->wait = t->step_ms * 3 / 2;
    }
    if (at == period - SW_LEN[P(SIZE)]) {
        int v;
        s->chord = toy_root(t, 1);  /* the chord that is coming */
        s->all = SW_LEN[P(SIZE)] * t->step_ms;
        s->tt = s->echo = 0;
        s->blip = 0;
        for (v = 0; v < 3; v++)
            toy_note(t, s->chord + 2 * v, v, 0);
    }
}

static void F(frame)(toy_t *t, int ms)
{
    swell_t *s = ST(swell_t);
    if (!t->playing)
        return;
    if (s->all) {
        s->tt = imin(s->all, s->tt + ms);
    } else if (s->echo || s->blip) {
        s->wait -= ms;
        if (s->wait <= 0) {
            s->blip = 0;
            if (s->echo) {          /* each echo is smaller than the last */
                s->echo--;
                toy_note(t, s->chord, 3, 0);
                s->blip = 78 * (s->echo + 1) / 4;
                s->wait += t->step_ms * 3 / 2;
            }
        }
    }
}

static void F(play_changed)(toy_t *t)
{
    swell_t *s = ST(swell_t);
    s->all = s->tt = s->echo = 0;
    s->blip = 0;
}

TOY(toy_swell, "swell", "Swell", "wedge", ROLE_CHORDS, HUE_PINK, 40, 1,
    "Chords that fade in like a sound played backwards, then cut dead on the downbeat.",
    swell_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef SIZE
#undef EVERY
#undef CURVE
#undef TAIL
