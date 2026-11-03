/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 98 (Witchbox III, texture): Toll Bell. One deep bell, struck every few bars and left to
 * ring, on the root, the fifth or the octave; now and then it wanders to another note of the chord. */
#include "toy_util.h"
#undef F
#define F(n) tol_##n

static const uint8_t TL_EVERY[3] = {32, 64, 128};
static const uint8_t TL_NOTE[3] = {0, 4, 7};
static const char *const TL_EVERY_N[3] = {"2 bars", "4 bars", "8 bars"};
static const char *const TL_NOTE_N[3] = {"Root", "Fifth", "Octave"};
typedef struct { int left, deg; } toll_t;
#define EVERY 0
#define STRIKES 1
#define NOTE 2
#define WANDER 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Every", 3, 1, TL_EVERY_N, "How often the bell is struck");
    add(t, "Strikes", 1, 4, 1, "How many times it is struck each time");
    add_pick(t, "Note", 3, 0, TL_NOTE_N, "Which note of the chord it tolls");
    add(t, "Wander", 0, 10, 2, "How often it tolls another note of the chord");
}

static void F(randomise)(toy_t *t)
{
    P(EVERY) = (int16_t)dice_int(3);
    P(STRIKES) = (int16_t)(1 + dice_int(4));
    P(NOTE) = (int16_t)dice_int(3);
    P(WANDER) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t) { ST(toll_t)->left = 0; }

static void F(on_step)(toy_t *t)
{
    toll_t *s = ST(toll_t);
    int now = toy_now(t);
    if (now % TL_EVERY[P(EVERY)] == 0) {
        s->deg = toy_root(t, 0) - 7 + TL_NOTE[P(NOTE)];
        if (dice_lt(P(WANDER), 11))
            s->deg += dice_lt_q(32768) ? 2 : 4;
        s->left = P(STRIKES) * 2;
    }
    if (s->left > 0) {
        if (s->left % 2 == 0)
            toy_note(t, s->deg, 0, 0);
        s->left--;
    }
}

#ifdef WB_PICTURES
/* the picture: a bell that swings when it is struck, with rings spreading from it while it sounds */
static void F(draw)(toy_t *t)
{
    uint32_t ago = pic_ago(t, 0);
    int swing = ago < 1200u ? PIC_SIN[(ago / 40u) & 31] * (int)(1200u - ago) / 100 / 120 : 0, cx = 120 + swing, i;
    if (ago < 3000u)
        for (i = 0; i < 3; i++) {
            int r = 36 + (int)(ago / 25u) + i * 14;
            if (r < 120)
                pic_ring(120, 76, r, r - 2, i == 0 ? K_SOFT : K_SAND);
        }
    cv_rect(118, 14, 4, 10, K_SOFT);
    for (i = 0; i < 44; i++)
        cv_rect(cx - 8 - i * i / 60, 24 + i, 16 + i * i / 30, 1, ago < 120u ? pic_hue : K_TEXT);
    cv_rect(cx - 30, 68, 60, 5, ago < 120u ? pic_hue : K_TEXT);
    pic_disc(cx, 80, 5, K_SOFT);
}
#endif

TOY(toy_toll, "toll", "Toll Bell", "bell", ROLE_TEXTURE, HUE_PURPLE, 0, 2, "One deep bell, struck every few bars and left to ring.", toll_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef EVERY
#undef STRIKES
#undef NOTE
#undef WANDER
