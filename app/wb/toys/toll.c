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
/* the picture: the bell as an orb that fills at the strike and drains as it rings out, rings spreading
 * from it through the seal; the chord's degrees round the rim with the one tolled in the hue, and the
 * strikes left as dots beneath (the ringing timed by the clock, the flash by the strike) */
static void F(draw)(toy_t *t)
{
    toll_t *s = ST(toll_t);
    uint32_t ago = pic_ago(t, 0), since = t->playing ? (uint32_t)((toy_now(t) % TL_EVERY[P(EVERY)]) * t->step_ms) : 100000u;
    int i, deg = imax(0, imin(7, s->deg - (toy_root(t, 0) - 7))), fill = since < 3000u ? (int)(3000u - since) / 3 : 0;
    pic_circle(PIC_CX, PIC_CY, 64, K_SAND);
    if (since < 3000u)
        for (i = 0; i < 3; i++) {
            int r = 26 + (int)(since / 30u) + i * 12;
            if (r < 62)
                pic_circle(PIC_CX, PIC_CY, r, i == 0 ? K_SOFT : K_SAND);
        }
    pic_orb(PIC_CX, PIC_CY, 24, fill, K_TEXT, ago < 150u ? K_TEXT : pic_hue);
    for (i = 0; i < 8; i++)
        pic_pt(PIC_CX, PIC_CY, 58, i, 8, i == deg ? 5 : i == 0 || i == 4 || i == 7 ? 2 : 1, i == deg ? pic_hue : K_SAND);
    pic_needle(PIC_CX, PIC_CY, 30, deg, 8, since < 3000u ? K_SOFT : K_LINE);
    for (i = 0; i < P(STRIKES); i++)
        pic_disc(PIC_CX - (P(STRIKES) - 1) * 4 + i * 8, 134, 2, i >= P(STRIKES) - (s->left + 1) / 2 ? pic_hue : K_SAND);
}
#endif

TOY(toy_toll, "toll", "Toll Bell", "bell", ROLE_TEXTURE, HUE_PURPLE, 0, 2, "One deep bell, struck every few bars and left to ring.", toll_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef EVERY
#undef STRIKES
#undef NOTE
#undef WANDER
