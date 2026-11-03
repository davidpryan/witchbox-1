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

#ifdef WB_PICTURES
/* the picture: rain streaks that fall harder with Rain, the wind as lines that lean with it, a flash
 * at the thunder, and a gust that sweeps across */
static void F(draw)(toy_t *t)
{
    uint32_t now = plat_ms(), ago_t = pic_ago(t, 2), ago_g = pic_ago(t, 3), i;
    int n = P(RAIN) * 6, lean = P(WIND);
    if (ago_t < 120u)
        cv_rect(0, 0, 240, 144, K_PANEL);
    for (i = 0; i < (uint32_t)n; i++) {
        uint32_t h = pic_hash(i, 0);
        int x = (int)(h % 240u), y = (int)((h / 240u + now / 6u) % 150u) - 6;
        cv_line(x, y, x - lean / 2, y + 8, K_SOFT);
    }
    for (i = 0; i < (uint32_t)lean; i++) {
        uint32_t h = pic_hash(i, 1);
        int y = (int)(h % 130u) + 6, x = (int)((h / 130u + now / 4u) % 260u) - 10;
        cv_rect(x, y, 10 + lean, 1, K_SAND);
    }
    if (ago_g < 600u) {
        int x = (int)(ago_g * 300u / 600u) - 30;
        cv_rect(x, 60, 30, 2, pic_hue);
        cv_rect(x + 8, 68, 22, 2, pic_hue);
    }
    if (ago_t < 120u)
        cv_line(120, 0, 100, 60, K_TEXT), cv_line(100, 60, 116, 70, K_TEXT), cv_line(116, 70, 98, 144, K_TEXT);
}
#endif

TOY(toy_storm, "storm", "Storm", "cloud", ROLE_TEXTURE, HUE_BLUE, 0, 2, "Wind and rain that never stop, and thunder now and then.", storm_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef WIND
#undef RAIN
#undef THUNDER
#undef GUSTS
