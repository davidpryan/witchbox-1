/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 120 (Witchbox IV, conductor): Moon. Changes the key under the whole band, phase by phase: every
 * so many bars it moves to the next dark scale, and may change the chord progression, then comes
 * back home at the end of its cycle. */
#include "toy_util.h"
#undef F
#define F(n) mn_##n

static const uint16_t MN_EVERY[3] = {64, 128, 256};
static const char *const MN_EVERY_N[3] = {"4 bars", "8 bars", "16 bars"};
static const char *const MN_SCALES_N[3] = {"Two", "Three", "All five"};
typedef struct { int home_scale, home_chords, phase; } moon_t;
#define EVERY 0
#define SCALES 1
#define CHORDS 2
#define HOME 3

static void F(remember)(toy_t *t)
{
    moon_t *s = ST(moon_t);
    s->home_scale = S.scale.value;
    s->home_chords = S.chords;
    s->phase = 0;
}

static void F(make)(toy_t *t)
{
    add_pick(t, "Every", 3, 1, MN_EVERY_N, "How long each phase lasts");
    add_pick(t, "Scales", 3, 1, MN_SCALES_N, "How many of the dark scales it moves through");
    add(t, "Chords", 0, 10, 3, "How often a phase brings new chords too");
    add(t, "Home", 1, 8, 4, "Phases before it comes home");
    F(remember)(t);
}

static void F(randomise)(toy_t *t)
{
    P(EVERY) = (int16_t)dice_int(3);
    P(SCALES) = (int16_t)dice_int(3);
    P(CHORDS) = (int16_t)dice_int(11);
    P(HOME) = (int16_t)(1 + dice_int(8));
}

static void F(go_home)(toy_t *t)
{
    moon_t *s = ST(moon_t);
    S.scale.value = (int16_t)s->home_scale;
    S.chords = s->home_chords;
    s->phase = 0;
}

static void F(close)(toy_t *t) { F(go_home)(t); }
static void F(play_changed)(toy_t *t) { if (t->playing) F(remember)(t); else F(go_home)(t); }

static void F(on_step)(toy_t *t)
{
    moon_t *s = ST(moon_t);
    int now = toy_now(t), n = P(SCALES) == 0 ? 2 : P(SCALES) == 1 ? 3 : NDARK, i, at = 0;
    if (now == 0 || now % MN_EVERY[P(EVERY)] != 0)
        return;
    if (++s->phase > P(HOME)) {
        F(go_home)(t);
        return;
    }
    for (i = 0; i < NDARK; i++)
        if (S.scale.value == DARK_SCALES[i])
            at = i;
    S.scale.value = DARK_SCALES[(at + 1) % n];
    if (dice_lt(P(CHORDS), 11))
        S.chords = 1 + dice_int(NPROG);
}

#ifdef WB_PICTURES
/* the picture: a moon whose phase is the conductor's, full at home and waning as it moves off; the
 * scale's name under it */
static void F(draw)(toy_t *t)
{
    moon_t *s = ST(moon_t);
    int ph = s->phase, cx = 120, cy = 62, dy, home = s->phase == 0;
    char b[16];
    pic_disc(cx, cy, 36, home ? K_TEXT : K_SOFT);
    if (!home) {
        int off = imin(8, ph) * 9;
        for (dy = -36; dy <= 36; dy++) {
            int dx = 0;
            while (dx < 36 && (dx + 1) * (dx + 1) + dy * dy <= 36 * 36) dx++;
            cv_rect(cx - dx + off / 4, cy + dy, imax(0, dx * 2 - off), 1, K_PANEL);
        }
    }
    caps(b, SCALE_NAME[S.scale.value], 16);
    cv_text(cx - 30, 110, &FONT_XS, b, pic_hue);
    cv_text(cx - 30, 126, &FONT_XS, home ? "HOME" : "AWAY", K_DIM);
}
#endif

TOY(toy_moon, "moon", "Moon", "moon", ROLE_CONDUCTOR, HUE_BLUE, 0, 3, "A conductor. Moves the whole band to the next dark scale every phase, sometimes with new chords, then comes home.", moon_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), F(close));
#undef EVERY
#undef SCALES
#undef CHORDS
#undef HOME
