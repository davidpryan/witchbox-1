/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 90 (Witchbox III, chords): Vespers. A low choir chanting long, slow notes that barely move,
 * on the chord's lowest notes; a second line of singers can answer above. */
#include "toy_util.h"
#undef F
#define F(n) vp_##n

static const uint8_t VP_HOLD[3] = {16, 32, 64};
static const char *const VP_HOLD_N[3] = {"1 bar", "2 bars", "4 bars"};
static const char *const VP_SECOND_N[2] = {"Off", "On"};
typedef struct { int deg, up; } vespers_t;
#define NOTES 0
#define SECOND 1
#define HOLD 2
#define HOPS 3

static void F(make)(toy_t *t)
{
    add(t, "Notes", 1, 4, 2, "How many different notes the chant uses");
    add_pick(t, "Second", 2, 0, VP_SECOND_N, "A second line of singers above the first");
    add_pick(t, "Hold", 3, 1, VP_HOLD_N, "How long each note lasts");
    add(t, "Hops", 0, 10, 3, "How often a singer hops to another note");
    ST(vespers_t)->deg = -7;
}

static void F(randomise)(toy_t *t)
{
    P(NOTES) = (int16_t)(1 + dice_int(4));
    P(SECOND) = (int16_t)dice_int(2);
    P(HOLD) = (int16_t)dice_int(3);
    P(HOPS) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    vespers_t *s = ST(vespers_t);
    int every = VP_HOLD[P(HOLD)], root, hold, deg, n;
    int8_t tones[8];
    if (toy_now(t) % every != 0)
        return;
    root = toy_root(t, 0) - 7;
    n = tune_tones(root, root, root + 6, tones);
    n = imin(n, P(NOTES));
    deg = s->deg;
    if (deg < root || deg > root + 6 || dice_lt(P(HOPS), 11))
        deg = tones[dice_int(n)];
    else
        deg = tune_nearest(deg, root);
    s->deg = deg;
    hold = every * t->step_ms * 19 / 20;
    toy_note(t, deg, 0, hold);
    if (P(SECOND)) {
        s->up = deg + (dice_lt_q(32768) ? 2 : 4);
        toy_note(t, s->up, 1, hold);
    }
}

#ifdef WB_PICTURES
/* the picture: the chant as a long low bar at the note's height, and a second above it when the second
 * line sings; both breathe in colour while they sound */
static void F(draw)(toy_t *t)
{
    vespers_t *s = ST(vespers_t);
    int root = toy_root(t, 0) - 7, y = 112 - (s->deg - root) * 6, y2 = 112 - (s->up - root) * 6;
    uint32_t ago = pic_ago(t, 0);
    int k;
    for (k = 0; k < 7; k++)
        cv_rect(16, 112 - k * 6, 208, 1, K_LINE);
    cv_rect(16, y - 4, 208, 8, ago < 60000u && t->playing ? pic_hue : K_TEXT);
    if (P(SECOND))
        cv_rect(40, y2 - 3, 160, 6, ago < 60000u && t->playing ? K_SOFT : K_SAND);
    cv_text(16, 124, &FONT_XS, "CHANT", K_DIM);
}
#endif

TOY(toy_vespers, "vespers", "Vespers", "choir", ROLE_CHORDS, HUE_PURPLE, 0, 2, "A low choir chanting long, slow notes that barely move.", vespers_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef NOTES
#undef SECOND
#undef HOLD
#undef HOPS
