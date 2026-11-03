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
/* the picture: two reels, their holes turning, a tape between them with a block for each beat that
 * plays; the tape goes fainter and more crooked as the loop wears; the pass count written under */
static void F(draw)(toy_t *t)
{
    tapeloop_t *s = ST(tapeloop_t);
    static const uint8_t BEATS[4] = {3, 5, 6, 7};
    static const int8_t SPOKE[8][2] = {{0, -10}, {7, -7}, {10, 0}, {7, 7}, {0, 10}, {-7, 7}, {-10, 0}, {-7, -7}};
    int level = imin(2, s->age * P(WEAR) / 10), wave = level * P(WOBBLE) * 3 / 10, i, b, n = BEATS[P(LOOP)];
    uint16_t ink = level == 0 ? K_TEXT : level == 1 ? K_SOFT : K_SAND;
    int rx[2] = {44, 202}, ry = 44, turn = t->playing ? (int)(plat_ms() / 90u) & 7 : 0;
    char buf[20];
    for (i = 0; i < 2; i++) {
        pic_disc(rx[i], ry, 20, K_TEXT);
        pic_disc(rx[i], ry, 4, K_BG);
        cv_rect(rx[i] + SPOKE[turn][0] - 2, ry + SPOKE[turn][1] - 2, 5, 5, K_BG);
        cv_rect(rx[i] - SPOKE[turn][0] - 2, ry - SPOKE[turn][1] - 2, 5, 5, K_BG);
        cv_rect(rx[i] + (i ? 0 : -1), ry + 20, 2, 106 - ry - 20, K_SOFT);
    }
    for (i = 0; i < 21; i++)
        cv_rect(45 + i * 7, 106 + (i % 2 ? wave : -wave), 7, 2, ink);
    for (b = 0; b < 7; b++)
        if (s->hits[b] != TL_NONE && b < n)
            cv_rect(45 + b * 150 / n, 100, 5, 14, b == s->lit ? pic_hue : ink);
    if (s->lit >= 0)
        cv_rect(45 + s->lit * 150 / n, 90, 5, 8, pic_hue);
    itoa_s(s->age % P(RENEW) + 1, buf);
    cv_text(96, 124, &FONT_XS, "PASS", K_SOFT);
    cv_text(124, 124, &FONT_XS, buf, K_SOFT);
}
#endif

TOY(toy_tapeloop, "tapeloop", "Tape Loop", "reels", ROLE_CHORDS, HUE_CYAN, 0, 1, "A short loop of chords on worn tape that drifts against the bar.", tapeloop_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef LOOP
#undef WEAR
#undef WOBBLE
#undef RENEW
