/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 68 (Witchbox, chords): Tape Loop. A loop of chords 3, 5, 6 or 7 beats long that drifts
 * against the bar, wearing a little each pass, renewed after so many passes. */
#include "toy_util.h"
#undef F
#define F(n) tl_##n

static const uint8_t TL_BEATS[4] = {3, 5, 6, 7};
static const char *const TL_LOOP_N[4] = {"3 beats", "5 beats", "6 beats", "7 beats"};
#define TL_NONE (-99)
typedef struct { int hits[7], age, look_valid, look_loop, lit; } tapeloop_t;
#define LOOP 0
#define WEAR 1
#define WOBBLE 2
#define RENEW 3

static void F(record)(toy_t *t)    /* a fresh loop: the chord now, then the next one, on some beats */
{
    tapeloop_t *s = ST(tapeloop_t);
    int n = TL_BEATS[P(LOOP)], b;
    for (b = 0; b < 7; b++) {
        int r = b * 2 < n ? toy_root(t, 0) : toy_root(t, 1);
        int on = b < n && (b == 0 || dice_lt_q(36045u /* rand() < 0.55 */));
        s->hits[b] = on ? r : TL_NONE;
    }
    s->age = 0;
}

static void F(make)(toy_t *t)
{
    tapeloop_t *s = ST(tapeloop_t);
    int b;
    add_pick(t, "Loop", 4, 1, TL_LOOP_N, "How long the loop is");
    add(t, "Wear", 1, 10, 5, "How fast the loop wears out");
    add(t, "Wobble", 0, 10, 5, "How much the pitch wavers");
    add(t, "Renew", 4, 32, 8, "Passes before a fresh loop");
    for (b = 0; b < 7; b++)
        s->hits[b] = (b == 0 || b == 2) ? 0 : TL_NONE;
    s->lit = -1;
}

static void F(randomise)(toy_t *t)
{
    P(LOOP) = (int16_t)dice_int(4);
    P(WEAR) = (int16_t)(1 + dice_int(10));
    P(WOBBLE) = (int16_t)dice_int(11);
    P(RENEW) = (int16_t)(4 + dice_int(13));
}

static void F(changed)(toy_t *t)
{
    tapeloop_t *s = ST(tapeloop_t);
    if (s->look_valid && P(LOOP) != s->look_loop)
        F(record)(t);               /* a different length: a new loop */
    s->look_valid = 1;
    s->look_loop = P(LOOP);
}

static void F(on_step)(toy_t *t)
{
    tapeloop_t *s = ST(tapeloop_t);
    int n = TL_BEATS[P(LOOP)] * 4, at = t->steps % n;
    if (at == 0) {                  /* a new pass: older, or renewed */
        s->age++;
        if (t->steps == 0 || s->age >= P(RENEW))
            F(record)(t);
        s->look_valid = 1;
        s->look_loop = P(LOOP);
    }
    s->lit = -1;
    if (at % 4 == 0 && s->hits[at / 4] != TL_NONE) {
        int v;
        s->lit = at / 4;
        for (v = 0; v < 3; v++)
            toy_note(t, s->hits[s->lit] + 2 * v, v, 0);
    }
}

static void F(play_changed)(toy_t *t)
{
    if (!t->playing)
        ST(tapeloop_t)->lit = -1;
}

#ifdef WB_PICTURES
/* the picture: the loop as a seal of its beats, joined round in a ring that wears from cream to
 * sand and jitters with Wobble as the tape ages; a beat that plays a big point, in the hue as it
 * sounds; a needle for the step; the passes till the loop is renewed as points on the outer ring */
static void F(draw)(toy_t *t)
{
    tapeloop_t *s = ST(tapeloop_t);
    int level = imin(2, s->age * P(WEAR) / 10), wave = level * P(WOBBLE) * 3 / 10, b, n = TL_BEATS[P(LOOP)];
    int cx = PIC_CX, cy = PIC_CY, steps = n * 4, at = t->steps % steps, renew = P(RENEW), pass = s->age % renew;
    int jit = t->playing && wave ? (int)((plat_ms() / 120u) & 1u) : 0;
    uint16_t ink = level == 0 ? K_TEXT : level == 1 ? K_SOFT : K_SAND;
    pic_circle(cx, cy, 64, K_SAND);
    for (b = 0; b < renew; b++)
        pic_pt(cx, cy, 64, b, renew, b == pass ? 2 : 1, b <= pass ? K_SOFT : K_SAND);
    for (b = 0; b < n; b++) {
        int a = pic_ang(b, n), a2 = pic_ang(b + 1, n);
        int r = 46 + ((b + jit) & 1 ? wave : -wave), r2 = 46 + ((b + 1 + jit) & 1 ? wave : -wave);
        cv_line(pic_px(cx, r, a), pic_py(cy, r, a), pic_px(cx, r2, a2), pic_py(cy, r2, a2), ink);
    }
    pic_needle(cx, cy, 36, at, steps, K_LINE);
    for (b = 0; b < n; b++) {
        int r = 46 + ((b + jit) & 1 ? wave : -wave);
        if (s->hits[b] != TL_NONE)
            pic_pt(cx, cy, r, b, n, 4, b == s->lit ? pic_hue : ink);
        else
            pic_pt(cx, cy, r, b, n, 0, K_SAND);
    }
    pic_disc(cx, cy, 3, ink);
}
#endif

TOY(toy_tapeloop, "tapeloop", "Tape Loop", "reels", ROLE_CHORDS, HUE_CYAN, 0, 1, "A short loop of chords on worn tape that drifts against the bar.", tapeloop_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef LOOP
#undef WEAR
#undef WOBBLE
#undef RENEW
