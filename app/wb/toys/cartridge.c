/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 94 (Witchbox III, lead): Cursed Cartridge. An old video game tune that sticks on a note,
 * repeats itself, and leaps where it should not. */
#include "toy_util.h"
#undef F
#define F(n) cc_##n

static const uint8_t CC_EVERY[2] = {2, 1};
static const uint8_t CC_LEN[2] = {16, 32};
static const char *const CC_SPEED_N[2] = {"Eighths", "Sixteenths"};
static const char *const CC_LEN_N[2] = {"1 bar", "2 bars"};
typedef struct { int8_t tune[32]; int stuck, stuck_deg; } cartridge_t;
#define STICKS 0
#define LEAPS 1
#define SPEED 2
#define LENGTH 3

static void F(deal)(toy_t *t)
{
    cartridge_t *s = ST(cartridge_t);
    int i, deg = 7;
    for (i = 0; i < 32; i++) {
        s->tune[i] = (int8_t)deg;
        if (i % 8 == 7)
            deg = 7 + dice_int(3) - 1;          /* each phrase starts near home */
        else
            deg += dice_int(5) - 2;
        deg = imax(2, imin(14, deg));
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Sticks", 0, 10, 3, "How often the tune sticks on a note");
    add(t, "Leaps", 0, 10, 3, "How often a note leaps where it should not");
    add_pick(t, "Speed", 2, 0, CC_SPEED_N, "How fast the notes come");
    add_pick(t, "Length", 2, 1, CC_LEN_N, "How long the tune is before it comes round");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(STICKS) = (int16_t)dice_int(11);
    P(LEAPS) = (int16_t)dice_int(11);
    P(SPEED) = (int16_t)dice_int(2);
    P(LENGTH) = (int16_t)dice_int(2);
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    cartridge_t *s = ST(cartridge_t);
    int every = CC_EVERY[P(SPEED)], len = CC_LEN[P(LENGTH)], i, deg;
    if (t->steps % every != 0)
        return;
    i = (t->steps / every) % len;
    deg = toy_root(t, 0) + s->tune[i];
    if (i % 4 == 0)
        deg = tune_nearest(deg, toy_root(t, 0));
    if (s->stuck > 0) {
        s->stuck--;
        deg = s->stuck_deg;
    } else if (dice_lt(P(STICKS), 30)) {
        s->stuck = 2 + dice_int(4);
        s->stuck_deg = deg;
    } else if (dice_lt(P(LEAPS), 30)) {
        deg += dice_lt_q(32768) ? 5 + dice_int(5) : -5 - dice_int(5);
    }
    toy_note(t, deg, 0, t->step_ms * every * 3 / 4);
}

#ifdef WB_PICTURES
/* the picture: the tune as a wheel of sixteen or thirty-two points, each out from the centre by its
 * height and joined to the next by a chord, so the tune is a jagged seal; the needle is the playhead;
 * where it sticks the hold rune burns in the hue at the centre and the stuck note's ring glows */
static void F(draw)(toy_t *t)
{
    cartridge_t *s = ST(cartridge_t);
    static const uint8_t EVERY[2] = {2, 1}, LEN[2] = {16, 32};
    int len = LEN[P(LENGTH)], i, now = (t->steps / EVERY[P(SPEED)]) % len;
    pic_seal(PIC_CX, PIC_CY, 64, len, 0, K_SAND);
    for (i = 0; i < len; i++) {
        int a = pic_ang(i, len), b = pic_ang(i + 1, len), r0 = 22 + s->tune[i] * 2, r1 = 22 + s->tune[(i + 1) % len] * 2;
        cv_line(pic_px(PIC_CX, r0, a), pic_py(PIC_CY, r0, a), pic_px(PIC_CX, r1, b), pic_py(PIC_CY, r1, b), K_LINE);
    }
    pic_needle(PIC_CX, PIC_CY, 58, now, len, t->playing ? pic_hue : K_LINE);
    for (i = 0; i < len; i++) {
        int cur = i == now && t->playing;
        pic_pt(PIC_CX, PIC_CY, 22 + s->tune[i] * 2, i, len, cur ? 4 : i % 4 == 0 ? 3 : 2, cur ? pic_hue : K_TEXT);
    }
    if (s->stuck && t->playing) {
        int r = 22 + imax(2, imin(14, s->stuck_deg - toy_root(t, 0))) * 2;
        pic_circle(PIC_CX, PIC_CY, r, pic_hue);
        pic_rune(PIC_CX - 4, PIC_CY - 6, RUNE_HOLD, pic_hue);
    } else
        pic_rune(PIC_CX - 4, PIC_CY - 6, RUNE_LEAD, K_SOFT);
}
#endif

TOY(toy_cartridge, "cartridge", "Cursed Cartridge", "chip", ROLE_LEAD, HUE_GO, 0, 2, "An old video game tune that sticks, repeats and leaps where it should not.", cartridge_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef STICKS
#undef LEAPS
#undef SPEED
#undef LENGTH
