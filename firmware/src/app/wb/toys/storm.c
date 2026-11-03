/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 96 (Witchbox III, texture): Storm. Wind and rain that never stop, gusts now and then, and
 * thunder every so often. */
#include "toy_util.h"
#undef F
#define F(n) sm_##n

typedef struct { int rain_left, wind_left; } storm_t;
#define WIND 0
#define RAIN 1
#define THUNDER 2
#define GUSTS 3

static void F(make)(toy_t *t)
{
    add(t, "Wind", 0, 10, 6, "How much wind there is");
    add(t, "Rain", 0, 10, 6, "How hard it rains");
    add(t, "Thunder", 0, 10, 3, "How often the thunder comes");
    add(t, "Gusts", 0, 10, 4, "How often a gust blows through");
}

static void F(randomise)(toy_t *t)
{
    P(WIND) = (int16_t)dice_int(11);
    P(RAIN) = (int16_t)dice_int(11);
    P(THUNDER) = (int16_t)dice_int(11);
    P(GUSTS) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    storm_t *s = ST(storm_t);
    s->rain_left = s->wind_left = 0;
}

static void F(on_step)(toy_t *t)
{
    storm_t *s = ST(storm_t);
    int i = t->steps % BAR, bar_ms = BAR * t->step_ms;
    if (s->rain_left > 0)
        s->rain_left--;
    if (s->wind_left > 0)
        s->wind_left--;
    if (i % 8 == 0 && s->rain_left <= 0 && dice_lt(P(RAIN), 10)) {   /* a sheet of rain, half a bar */
        toy_note(t, 0, 0, bar_ms / 2 + 100);
        s->rain_left = 8;
    }
    if (i == 0 && s->wind_left <= 0 && dice_lt(P(WIND), 10)) {
        toy_note(t, 0, 1, bar_ms + 200);
        s->wind_left = BAR;
    }
    if (i % 4 == 0 && dice_lt(P(THUNDER), 40))
        toy_note(t, dice_int(3) - 1, 2, 0);
    if (i % 4 == 2 && dice_lt(P(GUSTS), 24))
        toy_note(t, 0, 3, 0);
}

TOY(toy_storm, "storm", "Storm", "cloud", ROLE_TEXTURE, HUE_BLUE, 0, 2,
    "Wind and rain that never stop, and thunder now and then.",
    storm_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef WIND
#undef RAIN
#undef THUNDER
#undef GUSTS
