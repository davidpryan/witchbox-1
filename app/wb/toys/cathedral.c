/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 55 (Witchbox, chords): Cathedral. Slow choir and organ chords, the notes fading in one
 * after another, held most of the bar, ringing on after. fade_ms, ring_ms, Blend for the engine. */
#include "toy_util.h"
#undef F
#define F(n) ca_##n

static const int8_t CA_NOTES[5] = {0, 2, 4, 7, 9};
static const char *const CA_HEIGHT_N[3] = {"None", "One", "Two"};
typedef struct { int lv[5], wait[5], held, fade_ms, ring_ms; } cathedral_t;
#define BLEND 0
#define FADE 1
#define RING 2
#define HEIGHT 3

static void F(make)(toy_t *t)
{
    add(t, "Blend", 0, 10, 5, "From all organ to all choir");
    add(t, "Fade", 1, 10, 4, "How slowly chords fade in");
    add(t, "Ring", 1, 10, 6, "How long they ring on");
    add_pick(t, "Height", 3, 1, CA_HEIGHT_N, "Adds notes an octave above");
}

static void F(randomise)(toy_t *t)
{
    P(BLEND) = (int16_t)dice_int(11);
    P(FADE) = (int16_t)(1 + dice_int(10));
    P(RING) = (int16_t)(1 + dice_int(10));
    P(HEIGHT) = (int16_t)dice_int(3);
}

static void F(play_changed)(toy_t *t) { ST(cathedral_t)->held = 0; }

static void F(on_step)(toy_t *t)
{
    cathedral_t *s = ST(cathedral_t);
    int i = t->steps % BAR;
    if (i == 0) {
        int n = 3 + P(HEIGHT), r = toy_root(t, 0), k;
        s->fade_ms = P(FADE) * 150;
        s->ring_ms = P(RING) * 350;
        for (k = 0; k < n; k++) {   /* the notes enter one after another */
            toy_note(t, r + CA_NOTES[k], k, 0);
            s->wait[k] = k * s->fade_ms / 4;
        }
        s->held = 1;
    } else if (i == 10) {
        s->held = 0;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    cathedral_t *s = ST(cathedral_t);
    int n = 3 + P(HEIGHT), k;
    for (k = 0; k < n; k++) {
        if (s->held && s->wait[k] <= 0)
            s->lv[k] = imin(1000, s->lv[k] + ms * 1000 / (P(FADE) * 150));
        else {
            s->wait[k] -= ms;
            s->lv[k] = imax(0, s->lv[k] - ms * 1000 / (P(RING) * 350));
        }
    }
}

#ifdef WB_PICTURES
/* the picture: a spire a voice, each a hollow glyph that fills from its base as its chord fades in
 * and drains as it rings out, the voice's rune beneath on a ground line */
static void F(draw)(toy_t *t)
{
    cathedral_t *s = ST(cathedral_t);
    int n = 3 + P(HEIGHT), k, w = 192 / n;
    for (k = 0; k < n; k++) {
        int x = 24 + k * w + w / 2;
        pic_spire(x, 110, w - 10, 84, s->lv[k], K_SAND, pic_hue);
        pic_rune(x - 4, 118, RUNE_CHORD + (k & 1) * (RUNE_VOICE - RUNE_CHORD), K_TEXT);
    }
    pic_hline(16, PW - 16, 112, K_TEXT);
    pic_hline(16, PW - 16, 113, K_TEXT);
}
#endif

TOY(toy_cathedral, "cathedral", "Cathedral", "arch", ROLE_CHORDS, HUE_PURPLE, 40, 0, "Slow choir and organ chords that fade in and ring on over each other.", cathedral_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef BLEND
#undef FADE
#undef RING
#undef HEIGHT
