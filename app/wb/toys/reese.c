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
/* the picture: two rings the same size that drift apart and back through each other as the two
 * notes beat, further with Detune, in the hue while a note sounds; the phrase's starts as points
 * on the outer seal, the needle on the step, a darker eye in the middle the duller the tone */
static void F(draw)(toy_t *t)
{
    reese_t *s = ST(reese_t);
    static const uint16_t starts[3] = {0x1, 0x101, 0x1041};
    int cx = PIC_CX, cy = PIC_CY, ph = (s->ph / 50) & 31, d = (4 + P(DETUNE) * 2) * PIC_SIN[ph] / 100, i;
    int on = s->on && t->playing;
    uint16_t c = on ? pic_hue : K_TEXT;
    pic_circle(cx, cy, 64, K_SAND);
    pic_needle(cx, cy, 60, t->steps % BAR, BAR, K_LINE);
    for (i = 0; i < BAR; i++)
        if (starts[P(PHRASE) - 1] & (1u << i))
            pic_pt(cx, cy, 60, i, BAR, 3, pic_ago(t, 0) < 150u ? pic_hue : K_SOFT);
    pic_circle(cx - d, cy, 38, c);
    pic_circle(cx + d, cy, 38, c);
    pic_circle(cx - d, cy, 37, c);
    pic_circle(cx + d, cy, 37, c);
    pic_disc(cx, cy, 2 + (10 - P(DARK)) / 2, on ? K_TEXT : K_SOFT);
}
#endif

TOY(toy_reese, "reese", "Reese", "twist", ROLE_BASS, HUE_BLUE, 30, 0, "A growling bass made of two notes rubbing together, in slow short phrases.", reese_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef DETUNE
#undef PHRASE
#undef DARK
#undef REST
