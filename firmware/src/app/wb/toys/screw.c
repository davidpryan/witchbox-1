/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 79 (Witchbox, conductor): Screw. Slows the whole band and drops its pitch (Drag), stops the
 * tape dead every so many bars, and wavers its speed. */
#include "toy_util.h"
#include "screw_waver.h"
#undef F
#define F(n) sr_##n

static const uint8_t SR_STOPS[4] = {0, 16, 8, 4};
static const uint8_t SR_TIME[3] = {4, 8, 16};
static const char *const SR_STOPS_N[4] = {"Never", "16 bars", "8 bars", "4 bars"};
static const char *const SR_TIME_N[3] = {"1 beat", "2 beats", "1 bar"};
typedef struct { int state, tt, len, armed, drift; } screw_t;
#define DRAG 0
#define STOPS 1
#define TIME 2
#define WAVER 3

static int F(base)(toy_t *t) { return 100 - P(DRAG) * 5 / 2; }

static void F(free)(toy_t *t)      /* out of any tape stop, back to the steady speed */
{
    screw_t *s = ST(screw_t);
    s->state = 0;
    S.speed = t->playing ? F(base)(t) : 100;
    S.drop = t->playing ? P(DRAG) : 0;
}

static void F(make)(toy_t *t)
{
    screw_t *s = ST(screw_t);
    add(t, "Drag", 0, 10, 3, "A steady slow-down and drop in pitch");
    add_pick(t, "Stops", 4, 3, SR_STOPS_N, "How often the tape stops dead");
    add_pick(t, "Stop time", 3, 0, SR_TIME_N, "How long the slow-down takes");
    add(t, "Waver", 0, 10, 2, "A slow unsteady drift in speed");
    s->len = 1;
    s->armed = 1;
}

static void F(randomise)(toy_t *t)
{
    P(DRAG) = (int16_t)dice_int(11);
    P(STOPS) = (int16_t)dice_int(4);
    P(TIME) = (int16_t)dice_int(3);
    P(WAVER) = (int16_t)dice_int(11);
}

static void F(close)(toy_t *t)
{
    (void)t;
    S.speed = 100;
    S.drop = 0;
}

static void F(changed)(toy_t *t)
{
    if (ST(screw_t)->state == 0 || !t->playing)
        F(free)(t);
}

static void F(play_changed)(toy_t *t)
{
    ST(screw_t)->armed = 1;
    F(free)(t);
}

static void F(on_step)(toy_t *t)
{
    screw_t *s = ST(screw_t);
    int bars = SR_STOPS[P(STOPS)], period, early, late;
    if (!bars)
        return;
    period = bars * BAR;
    early = SR_TIME[P(TIME)] / 2;   /* the clock covers half its ground while it slows */
    late = toy_now(t) % period >= period - early;
    if (late && s->armed && s->state == 0) {
        s->state = 1;
        s->tt = 0;
        s->len = SR_TIME[P(TIME)] * 1500000 / (S.tempo.value * F(base)(t));
    }
    s->armed = !late;
}

static int F(waver)(int drift, int w)
{
    int i, v = 0;
    for (i = 0; i < SR_TRANS && SR_WAVER[w][i][0] <= drift; i++)
        v = SR_WAVER[w][i][1];
    return v;
}

static void F(frame)(toy_t *t, int ms)
{
    screw_t *s = ST(screw_t);
    if (t->playing) {
        int base = F(base)(t), speed;
        if (s->state == 1) {        /* slowing down */
            s->tt += ms;
            speed = fdiv(base * (s->len - s->tt), s->len);
            if (speed <= 0) {
                speed = 0;
                s->state = 2;
                s->tt = 0;
            }
        } else if (s->state == 2) { /* standing still */
            s->tt += ms;
            speed = 0;
            if (s->tt >= 300) {
                s->state = 0;
                speed = base;       /* snap back */
            }
        } else {
            s->drift = (s->drift + ms) % 6000;
            speed = base + F(waver)(s->drift, P(WAVER));
        }
        S.speed = imax(0, speed);
    }
}

TOY(toy_screw, "screw", "Screw", "reel", ROLE_CONDUCTOR, HUE_PURPLE, 40, 1,
    "Slows the whole band and drops its pitch, and stops the tape dead every so often.",
    screw_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, F(close));
#undef DRAG
#undef STOPS
#undef TIME
#undef WAVER
