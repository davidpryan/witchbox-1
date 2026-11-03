/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 105 (Witchbox IV, bass): Sleepwalk. A random walk on the chord's notes: each step up or down
 * by a little, with momentum (it keeps going the way it went) and a pull back to the root. */
#include "toy_util.h"
#undef F
#define F(n) slw_##n

static const uint8_t SLW_EVERY[3] = {4, 2, 1};
static const char *const SLW_RATE_N[3] = {"Quarters", "Eighths", "Sixteenths"};
typedef struct { int deg, dir; } sleepwalk_t;
#define STEP 0
#define MOMENTUM 1
#define RATE 2
#define RETURN 3

static void F(make)(toy_t *t)
{
    add(t, "Step", 1, 4, 1, "How far one step of the walk can go");
    add(t, "Momentum", 0, 10, 5, "How much it keeps going the same way");
    add_pick(t, "Rate", 3, 1, SLW_RATE_N, "How often it steps");
    add(t, "Return", 0, 10, 4, "How hard the root pulls it back");
    ST(sleepwalk_t)->deg = -7;
    ST(sleepwalk_t)->dir = 1;
}

static void F(randomise)(toy_t *t)
{
    P(STEP) = (int16_t)(1 + dice_int(4));
    P(MOMENTUM) = (int16_t)dice_int(11);
    P(RATE) = (int16_t)dice_int(3);
    P(RETURN) = (int16_t)dice_int(11);
}

static void F(on_step)(toy_t *t)
{
    sleepwalk_t *s = ST(sleepwalk_t);
    int root = toy_root(t, 0) - 7, dir, deg;
    if (t->steps % SLW_EVERY[P(RATE)] != 0)
        return;
    if (t->steps % BAR == 0 && dice_lt(P(RETURN), 11)) {
        deg = root;                             /* pulled home on the bar */
    } else {
        dir = dice_lt(P(MOMENTUM), 12) ? s->dir : (dice_lt_q(32768) ? 1 : -1);
        if (s->deg >= root + 7 && dir > 0)
            dir = -1;
        if (s->deg <= root - 5 && dir < 0)
            dir = 1;
        s->dir = dir;
        deg = tune_nearest(s->deg + dir * (1 + dice_int(P(STEP))), root);
    }
    s->deg = deg;
    toy_note(t, deg, 0, 0);
}

#ifdef WB_PICTURES
/* the picture: the chord's notes as a wheel of thirteen points, the chord tones cream, the rest sand;
 * the walker a disc in the hue on its note, the way it came as a trail of fading points behind it
 * and a thread of chords; the rune at the centre points the way it is going */
static void F(draw)(toy_t *t)
{
    sleepwalk_t *s = ST(sleepwalk_t);
    uint32_t now = plat_ms(), i;
    int root = toy_root(t, 0) - 7, k, cx = PIC_CX, cy = PIC_CY, prev = -1;
    pic_seal(cx, cy, 64, 13, 0, K_SAND);
    for (k = -5; k <= 7; k++) {
        int tone = (k + 70) % 7 == 0 || (k + 70) % 7 == 2 || (k + 70) % 7 == 4;
        pic_pt(cx, cy, 48, k + 5, 13, tone ? 2 : 0, tone ? K_TEXT : K_SAND);
    }
    for (i = 0; i < NOTE_LOG; i++) {
        const note_log_t *l = &note_log[i];
        int d;
        if (!l->at || l->seat != (uint8_t)t->seat || now - l->at > 4000u)
            continue;
        d = l->degree - root + 5;
        if (d < 0 || d > 12)
            continue;
        pic_pt(cx, cy, 48, d, 13, now - l->at < 1500u ? 3 : 2, now - l->at < 1500u ? K_SOFT : K_SAND);
        if (prev >= 0 && prev != d)
            pic_chord(cx, cy, 48, prev, d, 13, K_LINE);
        prev = d;
    }
    k = s->deg - root + 5;
    k = k < 0 ? 0 : k > 12 ? 12 : k;
    pic_needle(cx, cy, 44, k, 13, K_LINE);
    pic_pt(cx, cy, 48, k, 13, pic_ago(t, -1) < 120u ? 7 : 5, pic_hue);
    pic_disc(cx, cy, 10, K_BG);
    pic_rune(cx - 4, cy - 6, s->dir > 0 ? RUNE_UP : RUNE_DOWN, K_TEXT);
}
#endif

TOY(toy_sleepwalk, "sleepwalk", "Sleepwalk", "walk", ROLE_BASS, HUE_BLUE, 0, 3, "A random walk on the chord's notes, with momentum and a pull back to the root.", sleepwalk_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef STEP
#undef MOMENTUM
#undef RATE
#undef RETURN
