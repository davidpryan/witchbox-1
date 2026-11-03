/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 49 (Witchbox, bass): Reese. Two buzzing notes a hair apart, in slow phrases of one to
 * three chord notes across a bar, then a wait. */
#include "toy_util.h"
#undef F
#define F(n) re_##n

static const uint8_t RE_WAIT[4] = {0, 1, 2, 4};
static const char *const RE_REST_N[4] = {"None", "1 bar", "2 bars", "4 bars"};

typedef struct { int on, ph, turn; } reese_t;

#define DETUNE 0
#define PHRASE 1
#define DARK 2
#define REST 3

static void F(make)(toy_t *t)
{
    add(t, "Detune", 0, 10, 4, "From a slow swirl to a rough growl");
    add(t, "Phrase", 1, 3, 2, "How many notes in each phrase");
    add(t, "Dark", 0, 10, 5, "How muffled the tone is");
    add_pick(t, "Rest", 4, 1, RE_REST_N, "How long it waits between phrases");
}

static void F(randomise)(toy_t *t)
{
    P(DETUNE) = (int16_t)dice_int(11);
    P(PHRASE) = (int16_t)(1 + dice_int(3));
    P(DARK) = (int16_t)dice_int(11);
    P(REST) = (int16_t)dice_int(4);
}

static void F(play_changed)(toy_t *t) { ST(reese_t)->on = 0; }

static void F(on_step)(toy_t *t)
{
    reese_t *s = ST(reese_t);
    int i = t->steps % BAR;
    static const uint16_t starts[3] = {0x1, 0x101, 0x1041};
    if (toy_bar(t) % (1 + RE_WAIT[P(REST)])) {
        s->on = 0;                  /* a bar of waiting */
    } else if (starts[P(PHRASE) - 1] & (1u << i)) {
        int deg;
        if (i == 0) {
            deg = tune_nearest(toy_root(t, 0), toy_root(t, 0));
        } else {
            int8_t tones[8];
            int n = tune_tones(toy_root(t, 0), -3, 4, tones);
            deg = tones[dice_int(n)];
        }
        toy_note(t, deg, 0, 0);
        s->on = 1;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    reese_t *s = ST(reese_t);
    if (t->playing)
        s->ph += ms * (1 + P(DETUNE));
    s->turn ^= 1;
}

#ifdef WB_PICTURES
/* the picture: two lines, the same curve one way up and the other, crossing as they drift apart
 * with Detune; in colour while a note sounds */
static void F(draw)(toy_t *t)
{
    reese_t *s = ST(reese_t);
    static const int8_t SIN[16] = {0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24};
    int i, k, amp = 33 - P(DARK) * 2, ph = s->ph / 50;
    uint16_t c = s->on && t->playing ? pic_hue : K_TEXT;
    for (k = 0; k < 2; k++)
        for (i = 0; i < 32; i++) {
            int a = k ? -amp : amp, y = 72 + a * SIN[((i * 3 + ph) / 4) & 15] / 64;
            cv_rect(i * 8 - 1, y - 2, 8, 4, c);
        }
}
#endif

TOY(toy_reese, "reese", "Reese", "twist", ROLE_BASS, HUE_BLUE, 30, 0, "A growling bass made of two notes rubbing together, in slow short phrases.", reese_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef DETUNE
#undef PHRASE
#undef DARK
#undef REST
