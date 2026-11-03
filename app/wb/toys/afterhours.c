/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 89 (Witchbox III, chords): After Hours. Slow electric piano chords with extra notes on top
 * (sevenths, ninths), landing a step or two behind the beat. */
#include "toy_util.h"
#undef F
#define F(n) ah_##n

static const uint8_t AH_EVERY[3] = {16, 8, 4};
static const char *const AH_RATE_N[3] = {"Each bar", "Half bar", "Each beat"};
typedef struct { int last; } afterhours_t;
#define LATE 0
#define EXTRA 1
#define RATE 2
#define SPREAD 3

static void F(make)(toy_t *t)
{
    add(t, "Late", 0, 3, 1, "How many steps behind the beat it lands");
    add(t, "Extra", 0, 3, 1, "How many extra notes sit on top of the chord");
    add_pick(t, "Rate", 3, 1, AH_RATE_N, "How often the chord is played");
    add(t, "Spread", 0, 10, 4, "How far apart the chord's notes sit");
}

static void F(randomise)(toy_t *t)
{
    P(LATE) = (int16_t)dice_int(4);
    P(EXTRA) = (int16_t)dice_int(4);
    P(RATE) = (int16_t)dice_int(3);
    P(SPREAD) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    int every = AH_EVERY[P(RATE)], i = t->steps % BAR, k, n, root, hold;
    if ((i - P(LATE)) % every != 0 || i < P(LATE))
        return;
    root = toy_root(t, -P(LATE));
    n = 3 + P(EXTRA);
    hold = every * t->step_ms * 9 / 10;
    for (k = 0; k < n; k++) {
        int deg = root + 2 * k;
        if (k > 0 && dice_lt(P(SPREAD), 20))
            deg += 7;                           /* a note lifted an octave */
        if (k == 0 && P(SPREAD) > 5)
            deg -= 7;
        toy_note(t, deg, 0, hold);
    }
}

#ifdef WB_PICTURES
/* the picture: the seven degrees as the points of a seal and the chord as the chords joining the ones
 * it uses, extras in soft; the bar's steps round the rim with the beats it lands on in the hue, a
 * step or two past the beat marks; the chord lights while it rings and the needle is the playhead */
static void F(draw)(toy_t *t)
{
    static const uint8_t EVERY[3] = {16, 8, 4};
    int k, root = toy_root(t, 0), n = 3 + P(EXTRA), every = EVERY[P(RATE)], now = t->steps % BAR;
    uint16_t c = pic_ago(t, -1) < 500u ? pic_hue : K_TEXT;
    pic_circle(PIC_CX, PIC_CY, 64, K_SAND);
    pic_circle(PIC_CX, PIC_CY, 44, K_SAND);
    pic_needle(PIC_CX, PIC_CY, 60, now, BAR, t->playing ? K_SOFT : K_LINE);
    for (k = 0; k < BAR; k++) {
        int beat = k % every == 0, land = (k - P(LATE)) % every == 0 && k >= P(LATE);
        pic_pt(PIC_CX, PIC_CY, 60, k, BAR, land ? 3 : beat ? 2 : 0, land ? pic_hue : beat ? K_TEXT : K_SAND);
    }
    for (k = 0; k < 7; k++)
        pic_pt(PIC_CX, PIC_CY, 38, k, 7, 1, K_SAND);
    for (k = 0; k + 1 < n; k++)
        pic_chord(PIC_CX, PIC_CY, 38, (root + 2 * k) % 7, (root + 2 * k + 2) % 7, 7, k < 2 ? c : K_SOFT);
    for (k = 0; k < n; k++)
        pic_pt(PIC_CX, PIC_CY, 38, (root + 2 * k) % 7, 7, k < 3 ? 4 : 3, k < 3 ? c : K_SOFT);
    pic_rune(PIC_CX - 4, PIC_CY - 6, RUNE_CHORD, K_TEXT);
}
#endif

TOY(toy_afterhours, "afterhours", "After Hours", "keys", ROLE_CHORDS, HUE_BLUE, 0, 2, "Slow electric piano chords with extra notes on top, played a little late.", afterhours_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef LATE
#undef EXTRA
#undef RATE
#undef SPREAD
