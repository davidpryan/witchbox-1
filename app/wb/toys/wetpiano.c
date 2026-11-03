/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 54 (Witchbox, chords): Wet Piano. Sparse five-note chords on a dull piano, a touch late,
 * spread like a slow hand, often left out. Wow and Dull are for the engine. */
#include "toy_util.h"
#undef F
#define F(n) wp_##n

#define WP_DROPS 10
#define WP_KEYS 15
typedef struct { int key[WP_DROPS], ms[WP_DROPS], n; int ring[WP_KEYS]; } wetpiano_t;
#define STRUM 0
#define SPARSE 1
#define WOW 2
#define DULL 3

static void F(make)(toy_t *t)
{
    add(t, "Strum", 0, 10, 4, "From all at once to a slow spread");
    add(t, "Sparse", 0, 10, 4, "How often a chord is left out");
    add(t, "Wow", 0, 10, 3, "How much the pitch wavers");
    add(t, "Dull", 0, 10, 5, "How muffled the tone is");
}

static void F(randomise)(toy_t *t)
{
    P(STRUM) = (int16_t)dice_int(11);
    P(SPARSE) = (int16_t)dice_int(9);
    P(WOW) = (int16_t)dice_int(11);
    P(DULL) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    wetpiano_t *s = ST(wetpiano_t);
    int k;
    if (!t->playing) {
        s->n = 0;
        for (k = 0; k < WP_KEYS; k++)
            s->ring[k] = 0;
    }
}

static void F(on_step)(toy_t *t)
{
    wetpiano_t *s = ST(wetpiano_t);
    int i = t->steps % BAR, r, beat, k, keep, j;
    /* one beat before each chord's place (steps 0 and 8), decide whether it comes */
    if (!(i == 4 || i == 12) || dice_lt(P(SPARSE), 11))
        return;
    r = toy_root(t, i == 12 ? 1 : 0);
    beat = 4 * t->step_ms;
    keep = imin(s->n, 5);           /* drops[-5:] */
    for (j = 0; j < keep; j++) {
        s->key[j] = s->key[s->n - keep + j];
        s->ms[j] = s->ms[s->n - keep + j];
    }
    s->n = keep;
    for (k = 0; k < 5; k++) {
        s->key[s->n] = r + 2 * k;
        s->ms[s->n++] = beat + 30 + k * P(STRUM) * 14;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    wetpiano_t *s = ST(wetpiano_t);
    int k, i, landed = 0;
    for (k = 0; k < WP_KEYS; k++)
        if (s->ring[k])
            s->ring[k] = imax(0, s->ring[k] - ms);
    if (!s->n)
        return;
    for (i = 0; i < s->n; i++) {
        s->ms[i] -= ms;
        k = s->key[i];
        if (s->ms[i] <= 0) {        /* it lands: the note sounds */
            landed = 1;
            toy_note(t, k, fmod_(k, 5), 0);
            if (k >= 0 && k < WP_KEYS)
                s->ring[k] = 450;
        }
    }
    if (landed) {
        int j = 0;
        for (i = 0; i < s->n; i++)
            if (s->ms[i] > 0) {
                s->key[j] = s->key[i];
                s->ms[j++] = s->ms[i];
            }
        s->n = j;
    }
}

#ifdef WB_PICTURES
/* the picture: fifteen keys; drops fall on them, swaying with Wow, and a key struck goes dark for a
 * moment, darker the duller the tone */
static void F(draw)(toy_t *t)
{
    wetpiano_t *s = ST(wetpiano_t);
    static const int8_t SWAY[8] = {0, 1, 2, 1, 0, -1, -2, -1};
    int k, beat = 4 * t->step_ms;
    for (k = 0; k < WP_KEYS; k++) {
        uint16_t c = K_TEXT;
        if (s->ring[k])
            c = P(DULL) > 7 ? K_SAND : pic_hue;
        cv_rect(8 + k * 15, 112, 14, 26, c);
    }
    for (k = 0; k < s->n; k++) {
        int key = s->key[k], y = 108 - s->ms[k] * 82 / beat, x;
        if (y < 4 || s->ms[k] <= 0)
            continue;
        x = 8 + key * 15 + 7 + P(WOW) * SWAY[(y >> 3) & 7] / 4;
        pic_disc(x, y, 2, pic_hue);
    }
}
#endif

TOY(toy_wetpiano, "wetpiano", "Wet Piano", "pkeys", ROLE_CHORDS, HUE_BLUE, 25, 0, "Sparse, dull piano chords, strummed a little late like a worn tape.", wetpiano_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef STRUM
#undef SPARSE
#undef WOW
#undef DULL
